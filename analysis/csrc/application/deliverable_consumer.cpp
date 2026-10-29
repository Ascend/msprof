/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/application/deliverable_consumer.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <fstream>
#include <future>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "analysis/csrc/application/json_event_sink.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/dump_tools/csv_tool/include/csv_writer.h"
#include "analysis/csrc/infrastructure/dump_tools/include/sync_utils.h"
#include "analysis/csrc/infrastructure/dump_tools/json_tool/include/json_writer.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"

namespace Analysis
{
namespace Application
{
namespace
{
void RemoveStaleCsv(const std::string &basePath, const std::string &timestamp)
{
    const std::string suffix = std::string("_") + timestamp + SUMMARY_SUFFIX;
    const std::string parent = Utils::File::ParentPath(basePath);
    const std::string name = Utils::File::BaseName(basePath);
    if (!parent.empty() && !name.empty())
    {
        const auto files = Utils::File::GetOriginData(parent, {name}, {});
        for (const auto &path : files)
        {
            if (path.size() >= suffix.size() && path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0)
            {
                Utils::File::DeleteFile(path);
            }
        }
    }
    Utils::File::DeleteFile(basePath + suffix);
}

constexpr size_t kCsvFlushBytes = 32U * 1024U * 1024U;
constexpr size_t kIoQueueCap = 64U * 1024U * 1024U;
constexpr uint64_t kMaxSpillChunk = 256ULL * 1024ULL * 1024ULL;
constexpr uint32_t kWriterThreads = 8;
}  // namespace

struct DeliverableConsumer::IoOp
{
    enum class Type
    {
        Write,
        Rename
    };
    Type type = Type::Write;
    std::string path;
    std::string data;
    std::string renameTo;
    size_t bytes = 0;
};

struct DeliverableConsumer::ByteStream
{
    std::mutex mu;
    std::condition_variable cv;
    std::deque<IoOp> queue;
    size_t queuedBytes = 0;
    bool ioScheduled = false;
    bool opened = false;
    std::string path;
};

struct DeliverableConsumer::CsvStream
{
    ByteStream io;
    std::string basePath;
    std::string timestamp;
    std::string headerLine;
    std::set<int> mask;
    std::string buffer;
    std::string currentPath;
    std::mutex sessionMu;
    std::unordered_set<std::string> preparedPaths;
    uint64_t generation = 0;
    uint64_t totalHint = 0;
    uint64_t total = 0;
    uint64_t rowsInSlice = 0;
    int sliceIndex = 0;
    bool sliceMode = false;
    bool started = false;
};

struct DeliverableConsumer::DbState
{
    struct Item
    {
        std::function<bool(Infra::DBRunner &)> fn;
        std::shared_ptr<std::promise<bool>> done;
    };
    std::mutex mu;
    std::condition_variable cv;
    std::deque<Item> jobs;
    std::thread thread;
    bool stop = false;
};

struct DeliverableConsumer::Impl
{
    std::mutex mapMu;
    std::unordered_map<std::string, std::shared_ptr<ByteStream>> jsonStreams;
    std::vector<std::string> jsonOrder;
    std::unordered_map<std::string, std::shared_ptr<CsvStream>> csvStreams;
    std::mutex dbMu;
    std::unordered_map<std::string, std::shared_ptr<DbState>> dbs;
    std::mutex poolMu;
    std::unique_ptr<Utils::ThreadPool> pool;
    std::function<void()> beforeWrite;
    std::atomic<bool> ok{true};
};

DeliverableConsumer &DeliverableConsumer::Instance()
{
    static DeliverableConsumer consumer;
    return consumer;
}

DeliverableConsumer::DeliverableConsumer() : impl_(new Impl()) {}

DeliverableConsumer::~DeliverableConsumer()
{
    // No file IO here: static destruction order is not ours to rely on. ExportManager::Run owns
    // FinishTimeline(), the destructor only joins the DB writer threads.
    CloseAllDb();
}

bool DeliverableConsumer::Ok() const { return impl_->ok.load(); }

void DeliverableConsumer::ResetOk() { impl_->ok = true; }

void DeliverableConsumer::SetBeforeWriteForTest(std::function<void()> hook)
{
    std::lock_guard<std::mutex> lock(impl_->poolMu);
    impl_->beforeWrite = std::move(hook);
}

Utils::ThreadPool *DeliverableConsumer::EnsurePool()
{
    std::lock_guard<std::mutex> lock(impl_->poolMu);
    if (impl_->pool != nullptr)
    {
        return impl_->pool.get();
    }
    uint32_t threads = std::thread::hardware_concurrency();
    if (threads == 0)
    {
        threads = 2;
    }
    threads = std::max(2U, std::min(kWriterThreads, threads));
    impl_->pool.reset(new Utils::ThreadPool(threads));
    if (!impl_->pool->Start())
    {
        ERROR("Deliverable writer pool start failed, write on the caller thread.");
        impl_->pool.reset();
        return nullptr;
    }
    return impl_->pool.get();
}

void DeliverableConsumer::Enqueue(const std::shared_ptr<ByteStream> &stream, IoOp op)
{
    auto pool = EnsurePool();
    bool schedule = false;
    {
        std::unique_lock<std::mutex> lock(stream->mu);
        stream->cv.wait(lock, [&] { return stream->queuedBytes < kIoQueueCap; });
        stream->queuedBytes += op.bytes;
        stream->queue.emplace_back(std::move(op));
        if (!stream->ioScheduled)
        {
            stream->ioScheduled = true;
            schedule = true;
        }
    }
    if (!schedule)
    {
        return;
    }
    if (pool != nullptr)
    {
        try
        {
            pool->AddTask([this, stream] { Drain(stream.get()); });
            return;
        }
        catch (...)
        {
            // No task was queued, so nothing would ever drain this stream and Wait would block.
            ERROR("Schedule deliverable write failed, give up on this stream.");
            FailStream(*stream);
            return;
        }
    }
    Drain(stream.get());
}

void DeliverableConsumer::Drain(ByteStream *stream)
{
    Utils::FileWriter file;
    std::string openPath;
    try
    {
        for (;;)
        {
            IoOp op;
            {
                std::lock_guard<std::mutex> lock(stream->mu);
                if (stream->queue.empty())
                {
                    stream->ioScheduled = false;
                    stream->cv.notify_all();
                    return;
                }
                op = std::move(stream->queue.front());
                stream->queue.pop_front();
                if (stream->queuedBytes >= op.bytes)
                {
                    stream->queuedBytes -= op.bytes;
                }
                else
                {
                    stream->queuedBytes = 0;
                }
                stream->cv.notify_all();
            }
            if (op.type == IoOp::Type::Rename)
            {
                file.Close();
                openPath.clear();
                if (std::rename(op.path.c_str(), op.renameTo.c_str()) != 0)
                {
                    ERROR("Rename csv slice failed, from % to %", op.path, op.renameTo);
                    impl_->ok = false;
                }
                continue;
            }
            std::function<void()> beforeWrite;
            {
                std::lock_guard<std::mutex> lock(impl_->poolMu);
                beforeWrite = impl_->beforeWrite;
            }
            if (beforeWrite)
            {
                beforeWrite();
            }
            if (openPath != op.path)
            {
                file.Close();
                file.Open(op.path, std::ios::out | std::ios::app);
                if (!file.IsOpen())
                {
                    openPath.clear();
                    ERROR("Open deliverable file failed: %", op.path);
                    impl_->ok = false;
                    continue;
                }
                openPath = op.path;
            }
            else if (!file.IsOpen())
            {
                ERROR("Open deliverable file failed: %", op.path);
                impl_->ok = false;
                continue;
            }
            file.WriteText(op.data);
        }
    }
    catch (const std::exception &ex)
    {
        // The pool swallows the exception. Drop the rest of the queue or Wait never returns.
        ERROR("Deliverable writer exception: %", ex.what());
        FailStream(*stream);
    }
    catch (...)
    {
        ERROR("Deliverable writer unknown exception");
        FailStream(*stream);
    }
}

void DeliverableConsumer::FailStream(ByteStream &stream)
{
    impl_->ok = false;
    std::lock_guard<std::mutex> lock(stream.mu);
    stream.queue.clear();
    stream.queuedBytes = 0;
    stream.ioScheduled = false;
    stream.cv.notify_all();
}

void DeliverableConsumer::Wait(ByteStream &stream)
{
    std::unique_lock<std::mutex> lock(stream.mu);
    stream.cv.wait(lock, [&] { return stream.queue.empty() && !stream.ioScheduled; });
}

std::shared_ptr<DeliverableConsumer::ByteStream> DeliverableConsumer::GetJsonStream(const std::string &path)
{
    std::lock_guard<std::mutex> lock(impl_->mapMu);
    auto &slot = impl_->jsonStreams[path];
    if (slot == nullptr)
    {
        slot = std::make_shared<ByteStream>();
        slot->path = path;
        impl_->jsonOrder.emplace_back(path);
        // Same-process export reuses the timestamp, so append would extend the previous file.
        Utils::File::DeleteFile(path);
    }
    return slot;
}

std::string DeliverableConsumer::MakeJsonPath(const std::string &profPath, const std::string &prefix)
{
    std::string fileName = prefix;
    fileName.append("_").append(Infra::GetTimeStampStr()).append(JSON_SUFFIX);
    return Utils::File::PathJoin({profPath, Common::OUTPUT_PATH, fileName});
}

std::string DeliverableConsumer::SerializeEvents(const std::vector<std::shared_ptr<Domain::TraceEvent>> &events)
{
    Infra::JsonWriter writer;
    writer.StartArray();
    for (const auto &event : events)
    {
        if (event != nullptr)
        {
            event->DumpJson(writer);
        }
    }
    writer.EndArray();
    const char *text = writer.GetString();
    const size_t size = writer.GetSize();
    if (text == nullptr || size < 2 || text[0] != '[' || text[size - 1] != ']')
    {
        return "";
    }
    std::string body(text + 1, size - 2);
    if (body.find('{') == std::string::npos)
    {
        return "";
    }
    return body;
}

void DeliverableConsumer::AppendJsonBody(const std::shared_ptr<ByteStream> &stream, const std::string &body)
{
    // The '[' / ',' prefix and the queue push share one lock so two assemblers
    // cannot publish chunks out of order.
    auto pool = EnsurePool();
    bool schedule = false;
    {
        std::unique_lock<std::mutex> lock(stream->mu);
        stream->cv.wait(lock, [&] { return stream->queuedBytes < kIoQueueCap; });
        IoOp op;
        op.type = IoOp::Type::Write;
        op.path = stream->path;
        if (!stream->opened)
        {
            op.data.push_back('[');
            stream->opened = true;
        }
        else
        {
            op.data.push_back(',');
        }
        op.data.append(body);
        op.bytes = op.data.size();
        stream->queuedBytes += op.bytes;
        stream->queue.emplace_back(std::move(op));
        if (!stream->ioScheduled)
        {
            stream->ioScheduled = true;
            schedule = true;
        }
    }
    if (!schedule)
    {
        return;
    }
    if (pool != nullptr)
    {
        try
        {
            pool->AddTask([this, stream] { Drain(stream.get()); });
            return;
        }
        catch (...)
        {
            ERROR("Schedule deliverable write failed, give up on this stream.");
            FailStream(*stream);
            return;
        }
    }
    Drain(stream.get());
}

void DeliverableConsumer::BeginTimeline() { (void)EnsurePool(); }

void DeliverableConsumer::SyncJson(const std::vector<std::string> &prefixes, const std::string &profPath)
{
    std::vector<std::shared_ptr<ByteStream>> streams;
    {
        std::lock_guard<std::mutex> lock(impl_->mapMu);
        streams.reserve(prefixes.size());
        for (const auto &prefix : prefixes)
        {
            auto it = impl_->jsonStreams.find(MakeJsonPath(profPath, prefix));
            if (it != impl_->jsonStreams.end() && it->second != nullptr)
            {
                streams.push_back(it->second);
            }
        }
    }
    for (const auto &stream : streams)
    {
        Wait(*stream);
    }
}

void DeliverableConsumer::AcceptSerialized(const std::vector<std::string> &prefixes, const std::string &profPath,
                                           const std::string &body)
{
    if (body.empty() || prefixes.empty())
    {
        return;
    }
    for (const auto &prefix : prefixes)
    {
        AppendJsonBody(GetJsonStream(MakeJsonPath(profPath, prefix)), body);
    }
}

void DeliverableConsumer::AcceptJson(const std::vector<std::string> &prefixes, const std::string &profPath,
                                     const std::vector<std::shared_ptr<Domain::TraceEvent>> &events)
{
    if (events.empty() || prefixes.empty())
    {
        return;
    }
    AcceptSerialized(prefixes, profPath, SerializeEvents(events));
}

void DeliverableConsumer::FinishTimeline()
{
    std::vector<std::shared_ptr<ByteStream>> streams;
    {
        std::lock_guard<std::mutex> lock(impl_->mapMu);
        streams.reserve(impl_->jsonOrder.size());
        for (const auto &path : impl_->jsonOrder)
        {
            auto it = impl_->jsonStreams.find(path);
            if (it != impl_->jsonStreams.end())
            {
                streams.push_back(it->second);
            }
        }
    }
    for (const auto &stream : streams)
    {
        bool writeSuffix = false;
        {
            std::lock_guard<std::mutex> lock(stream->mu);
            writeSuffix = stream->opened;
        }
        if (writeSuffix)
        {
            IoOp op;
            op.type = IoOp::Type::Write;
            op.path = stream->path;
            op.data = "]";
            op.bytes = 1;
            Enqueue(stream, std::move(op));
        }
        Wait(*stream);
    }
    std::lock_guard<std::mutex> lock(impl_->mapMu);
    impl_->jsonStreams.clear();
    impl_->jsonOrder.clear();
}

std::shared_ptr<DeliverableConsumer::CsvStream> DeliverableConsumer::GetCsvStream(const std::string &basePath)
{
    std::lock_guard<std::mutex> lock(impl_->mapMu);
    auto &slot = impl_->csvStreams[basePath];
    if (slot == nullptr)
    {
        slot = std::make_shared<CsvStream>();
        slot->basePath = basePath;
    }
    return slot;
}

std::string DeliverableConsumer::MakeCsvPath(const CsvStream &stream)
{
    std::string name = stream.basePath;
    if (stream.sliceMode)
    {
        name.append("_").append(SLICE).append("_").append(std::to_string(stream.sliceIndex));
    }
    name.append("_").append(stream.timestamp).append(SUMMARY_SUFFIX);
    return name;
}

void DeliverableConsumer::AppendCsvRows(const std::shared_ptr<CsvStream> &stream,
                                        const std::vector<std::vector<std::string>> &rows)
{
    auto io = std::shared_ptr<ByteStream>(stream, &stream->io);
    auto prepare = [&stream](const std::string &path)
    {
        if (stream->preparedPaths.insert(path).second)
        {
            Utils::File::DeleteFile(path);
        }
    };
    auto flushBuffer = [this, &stream, io]()
    {
        if (stream->buffer.empty())
        {
            return;
        }
        IoOp op;
        op.type = IoOp::Type::Write;
        op.path = stream->currentPath;
        op.bytes = stream->buffer.size();
        op.data.swap(stream->buffer);
        Enqueue(io, std::move(op));
    };
    for (const auto &row : rows)
    {
        if (!stream->started)
        {
            if (stream->totalHint > CSV_LIMIT)
            {
                stream->sliceMode = true;
                stream->sliceIndex = 0;
            }
            stream->currentPath = MakeCsvPath(*stream);
            prepare(stream->currentPath);
            stream->buffer = stream->headerLine;
            stream->started = true;
        }
        else if (stream->rowsInSlice == CSV_LIMIT)
        {
            const std::string oldPath = stream->currentPath;
            flushBuffer();
            if (!stream->sliceMode)
            {
                stream->sliceMode = true;
                stream->sliceIndex = 0;
                const std::string sliced = MakeCsvPath(*stream);
                prepare(sliced);
                IoOp rename;
                rename.type = IoOp::Type::Rename;
                rename.path = oldPath;
                rename.renameTo = sliced;
                Enqueue(io, std::move(rename));
                stream->sliceIndex = 1;
            }
            else
            {
                ++stream->sliceIndex;
            }
            stream->currentPath = MakeCsvPath(*stream);
            prepare(stream->currentPath);
            stream->buffer = stream->headerLine;
            stream->rowsInSlice = 0;
        }
        Infra::CsvWriter::AppendRow(stream->buffer, row, stream->mask);
        ++stream->rowsInSlice;
        ++stream->total;
        if (stream->buffer.size() >= kCsvFlushBytes)
        {
            flushBuffer();
        }
    }
}

void DeliverableConsumer::StartCsv(const std::string &basePath, const std::vector<std::string> &headers,
                                   const std::set<int> &mask, const std::string &timestamp, uint64_t totalHint)
{
    auto stream = GetCsvStream(basePath);
    Wait(stream->io);
    std::lock_guard<std::mutex> session(stream->sessionMu);
    ++stream->generation;
    stream->preparedPaths.clear();
    stream->timestamp = timestamp.empty() ? Infra::GetTimeStampStr() : timestamp;
    RemoveStaleCsv(basePath, stream->timestamp);
    stream->headerLine = Infra::CsvWriter::FormatHeader(headers, mask);
    stream->mask = mask;
    stream->totalHint = totalHint;
    stream->total = 0;
    stream->rowsInSlice = 0;
    stream->sliceIndex = 0;
    stream->sliceMode = false;
    stream->started = false;
    stream->buffer.clear();
    stream->currentPath.clear();
    INFO("Start dump %.csv, timestamp is %.", basePath, stream->timestamp);
}

void DeliverableConsumer::AppendCsv(const std::string &basePath, const std::vector<std::vector<std::string>> &rows)
{
    if (rows.empty())
    {
        return;
    }
    auto stream = GetCsvStream(basePath);
    std::lock_guard<std::mutex> session(stream->sessionMu);
    AppendCsvRows(stream, rows);
}

void DeliverableConsumer::FinishCsv(const std::string &basePath)
{
    std::shared_ptr<CsvStream> stream;
    {
        std::lock_guard<std::mutex> lock(impl_->mapMu);
        auto it = impl_->csvStreams.find(basePath);
        if (it == impl_->csvStreams.end())
        {
            return;
        }
        stream = it->second;
    }
    uint64_t generation = 0;
    uint64_t rows = 0;
    {
        std::lock_guard<std::mutex> session(stream->sessionMu);
        generation = stream->generation;
        rows = stream->total;
        if (stream->total != 0 && !stream->buffer.empty())
        {
            auto io = std::shared_ptr<ByteStream>(stream, &stream->io);
            IoOp op;
            op.type = IoOp::Type::Write;
            op.path = stream->currentPath;
            op.bytes = stream->buffer.size();
            op.data.swap(stream->buffer);
            Enqueue(io, std::move(op));
        }
        Wait(stream->io);
    }
    // Report the real row count: the caller's Ok() check is about the bytes, this line is about how
    // much data the file actually holds.
    INFO("Generate %.csv finished, % rows, timestamp is %.", basePath, rows, stream->timestamp);
    std::lock_guard<std::mutex> lock(impl_->mapMu);
    std::lock_guard<std::mutex> session(stream->sessionMu);
    auto it = impl_->csvStreams.find(basePath);
    if (it != impl_->csvStreams.end() && it->second == stream && stream->generation == generation)
    {
        impl_->csvStreams.erase(it);
    }
}

void DeliverableConsumer::WriteCsv(const std::string &basePath, const std::vector<std::string> &headers,
                                   const std::vector<std::vector<std::string>> &rows, const std::set<int> &mask,
                                   const std::string &timestamp)
{
    StartCsv(basePath, headers, mask, timestamp, rows.size());
    for (size_t offset = 0; offset < rows.size(); offset += CSV_ROW_BATCH)
    {
        const size_t count = std::min(CSV_ROW_BATCH, rows.size() - offset);
        std::vector<std::vector<std::string>> part;
        part.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            part.emplace_back(rows[offset + i]);
        }
        AppendCsv(basePath, part);
    }
    FinishCsv(basePath);
}

std::shared_ptr<DeliverableConsumer::DbState> DeliverableConsumer::EnsureDb(const std::string &path)
{
    std::lock_guard<std::mutex> lock(impl_->dbMu);
    auto it = impl_->dbs.find(path);
    if (it != impl_->dbs.end())
    {
        return it->second;
    }
    auto state = std::make_shared<DbState>();
    state->thread = std::thread([state, path] { DbLoop(state, path); });
    impl_->dbs.emplace(path, state);
    return state;
}

void DeliverableConsumer::DbLoop(const std::shared_ptr<DbState> &state, const std::string &path)
{
    // DBRunner opens sqlite inside each call. A path with no job does not create a file.
    Infra::DBRunner runner(path);
    for (;;)
    {
        DbState::Item job;
        {
            std::unique_lock<std::mutex> lock(state->mu);
            state->cv.wait(lock, [&] { return state->stop || !state->jobs.empty(); });
            if (state->jobs.empty())
            {
                if (state->stop)
                {
                    return;
                }
                continue;
            }
            job = std::move(state->jobs.front());
            state->jobs.pop_front();
        }
        bool ok = false;
        try
        {
            ok = job.fn(runner);
        }
        catch (const std::exception &ex)
        {
            ERROR("DB writer exception: %", ex.what());
            ok = false;
        }
        catch (...)
        {
            ERROR("DB writer unknown exception");
            ok = false;
        }
        if (!ok)
        {
            Instance().impl_->ok = false;
        }
        job.done->set_value(ok);
    }
}

bool DeliverableConsumer::RunDb(const std::string &dbPath, const std::function<bool(Infra::DBRunner &)> &job)
{
    if (dbPath.empty() || !job)
    {
        return false;
    }
    auto state = EnsureDb(dbPath);
    auto done = std::make_shared<std::promise<bool>>();
    auto future = done->get_future();
    {
        std::lock_guard<std::mutex> lock(state->mu);
        if (state->stop)
        {
            return false;
        }
        state->jobs.push_back(DbState::Item{job, done});
    }
    state->cv.notify_one();
    return future.get();
}

bool DeliverableConsumer::CreateIndex(const std::string &dbPath, const std::string &table, const std::string &indexName,
                                      const std::vector<std::string> &colNames)
{
    if (dbPath.empty() || table.empty() || indexName.empty() || colNames.empty())
    {
        ERROR("Create index failed, argument is empty.");
        return false;
    }
    return RunDb(dbPath, [table, indexName, colNames](Infra::DBRunner &runner) -> bool
                 { return runner.CreateIndex(table, indexName, colNames); });
}

bool DeliverableConsumer::HasDbWriterForTest(const std::string &dbPath)
{
    std::lock_guard<std::mutex> lock(impl_->dbMu);
    return impl_->dbs.find(dbPath) != impl_->dbs.end();
}

void DeliverableConsumer::CloseAllDb()
{
    // Hold dbMu for the whole shutdown: EnsureDb must not create a second writer for a path whose
    // first writer is still being joined.
    std::lock_guard<std::mutex> guard(impl_->dbMu);
    std::vector<std::shared_ptr<DbState>> states;
    for (auto &item : impl_->dbs)
    {
        states.emplace_back(item.second);
    }
    impl_->dbs.clear();
    for (auto &state : states)
    {
        {
            std::lock_guard<std::mutex> lock(state->mu);
            state->stop = true;
        }
        state->cv.notify_all();
        if (state->thread.joinable())
        {
            state->thread.join();
        }
    }
}

void JsonEventSink::Open(const std::unordered_map<std::string, Infra::FileCategory> &files, const std::string &profPath)
{
    DeleteSpill();
    spillDisabled_ = false;
    prefixes_.clear();
    prefixes_.reserve(files.size());
    for (const auto &file : files)
    {
        prefixes_.emplace_back(file.first);
    }
    profPath_ = profPath;
    pending_.clear();
    accepted_ = 0;
    open_ = true;
}

void JsonEventSink::push_back(std::shared_ptr<Domain::TraceEvent> event)
{
    if (!open_ || event == nullptr)
    {
        return;
    }
    pending_.emplace_back(std::move(event));
    ++accepted_;
    if (pending_.size() < JSON_EVENT_BATCH)
    {
        return;
    }
    const std::string body = DeliverableConsumer::SerializeEvents(pending_);
    // Release the batch before touching the file: a failed spill must not keep these events while
    // every later event keeps growing the buffer.
    pending_.clear();
    if (body.empty())
    {
        return;
    }
    if (!spillDisabled_ && AppendSpill(body))
    {
        return;
    }
    PublishDirect(body);
}

void JsonEventSink::PublishDirect(const std::string &body)
{
    // The spill file is unusable (missing output directory, disk full, chunk too large). Replay
    // whatever was already spilled and then hand the rest straight to the consumer: the order of
    // the events is kept and the memory stays bounded. These batches can no longer be retracted by
    // Discard, so this is only a fallback for an already failing output device.
    if (!spillDisabled_)
    {
        WARN("Json spill is unavailable, publish the remaining events directly.");
        spillDisabled_ = true;
    }
    ReplaySpill();
    DeleteSpill();
    if (!body.empty())
    {
        DeliverableConsumer::Instance().AcceptSerialized(prefixes_, profPath_, body);
    }
}

void JsonEventSink::emplace_back(std::shared_ptr<Domain::TraceEvent> event) { push_back(std::move(event)); }

void JsonEventSink::Flush()
{
    if (!open_)
    {
        return;
    }
    if (spillRecords_ == 0 && !spillBroken_)
    {
        if (!pending_.empty())
        {
            DeliverableConsumer::Instance().AcceptJson(prefixes_, profPath_, pending_);
            pending_.clear();
        }
        return;
    }
    std::string tail;
    if (!pending_.empty())
    {
        tail = DeliverableConsumer::SerializeEvents(pending_);
        pending_.clear();
        if (!tail.empty() && AppendSpill(tail))
        {
            tail.clear();
        }
    }
    ReplaySpill();
    DeleteSpill();
    if (!tail.empty())
    {
        DeliverableConsumer::Instance().AcceptSerialized(prefixes_, profPath_, tail);
    }
}

void JsonEventSink::Discard()
{
    pending_.clear();
    accepted_ = 0;
    DeleteSpill();
}

void JsonEventSink::Sync() { DeliverableConsumer::Instance().SyncJson(prefixes_, profPath_); }

bool JsonEventSink::AppendSpill(const std::string &body)
{
    if (body.empty() || spillBroken_)
    {
        return body.empty();
    }
    if (body.size() > kMaxSpillChunk)
    {
        ERROR("Json spill chunk is too large.");
        spillBroken_ = true;
        return false;
    }
    if (spillPath_.empty())
    {
        static std::atomic<uint64_t> seq{0};
        const uint64_t id = seq.fetch_add(1, std::memory_order_relaxed);
        const std::string dir = Utils::File::PathJoin({profPath_, Common::OUTPUT_PATH});
        spillPath_ = Utils::File::PathJoin({dir, ".deliverable_spill_" + std::to_string(id) + ".tmp"});
    }
    std::ofstream out(spillPath_, std::ios::binary | std::ios::app);
    if (!out.is_open())
    {
        ERROR("Open json spill failed: %", spillPath_);
        spillBroken_ = true;
        return false;
    }
    const uint64_t bytes = static_cast<uint64_t>(body.size());
    out.write(reinterpret_cast<const char *>(&bytes), static_cast<std::streamsize>(sizeof(bytes)));
    out.write(body.data(), static_cast<std::streamsize>(body.size()));
    out.close();
    if (!out)
    {
        ERROR("Write json spill failed: %", spillPath_);
        if (spillRecords_ == 0)
        {
            Utils::File::DeleteFile(spillPath_);
            spillPath_.clear();
        }
        spillBroken_ = true;
        return false;
    }
    ++spillRecords_;
    return true;
}

void JsonEventSink::ReplaySpill()
{
    if (spillRecords_ == 0 || spillPath_.empty())
    {
        return;
    }
    std::ifstream in(spillPath_, std::ios::binary);
    if (!in.is_open())
    {
        ERROR("Read json spill failed: %", spillPath_);
        DeliverableConsumer::Instance().impl_->ok = false;
        return;
    }
    for (uint64_t record = 0; record < spillRecords_; ++record)
    {
        uint64_t bytes = 0;
        in.read(reinterpret_cast<char *>(&bytes), static_cast<std::streamsize>(sizeof(bytes)));
        if (!in || bytes == 0 || bytes > kMaxSpillChunk)
        {
            ERROR("Json spill record is invalid: %", spillPath_);
            DeliverableConsumer::Instance().impl_->ok = false;
            return;
        }
        std::string body(static_cast<size_t>(bytes), '\0');
        in.read(&body[0], static_cast<std::streamsize>(bytes));
        if (!in)
        {
            ERROR("Json spill record is truncated: %", spillPath_);
            DeliverableConsumer::Instance().impl_->ok = false;
            return;
        }
        DeliverableConsumer::Instance().AcceptSerialized(prefixes_, profPath_, body);
    }
}

void JsonEventSink::DeleteSpill()
{
    if (!spillPath_.empty())
    {
        Utils::File::DeleteFile(spillPath_);
    }
    spillPath_.clear();
    spillRecords_ = 0;
    spillBroken_ = false;
}
}  // namespace Application
}  // namespace Analysis
