/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "analysis/csrc/application/deliverable_consumer.h"
#include "analysis/csrc/application/json_event_sink.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/domain/entities/json_trace/include/trace_event.h"
#include "analysis/csrc/infrastructure/db/include/connection.h"
#include "analysis/csrc/infrastructure/dump_tools/include/sync_utils.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace
{
const std::string BASE_PATH = "./deliverable_consumer_utest";

std::shared_ptr<TraceEvent> MakeEvent(uint32_t id)
{
    return std::make_shared<DurationEvent>(id, 1, 1.5, "1.0", "op");
}

void PrepareDir(const std::string &dir)
{
    if (File::Exist(dir))
    {
        ASSERT_TRUE(File::RemoveDir(dir, 0));
    }
    ASSERT_TRUE(File::CreateDir(dir));
}

nlohmann::json ReadJsonFile(const std::string &path)
{
    FileReader reader(path);
    std::vector<std::string> lines;
    EXPECT_EQ(reader.ReadText(lines), Analysis::ANALYSIS_OK);
    std::string text;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (i != 0)
        {
            text.append("\n");
        }
        text.append(lines[i]);
    }
    return nlohmann::json::parse(text);
}

std::vector<std::string> FilesNamed(const std::string &dir, const std::string &prefix)
{
    auto files = File::GetOriginData(dir, {prefix}, {});
    std::sort(files.begin(), files.end());
    return files;
}
}  // namespace

class DeliverableConsumerUTest : public testing::Test
{
   protected:
    static void SetUpTestCase() { PrepareDir(BASE_PATH); }
    static void TearDownTestCase()
    {
        DeliverableConsumer::Instance().FinishTimeline();
        DeliverableConsumer::Instance().CloseAllDb();
        DeliverableConsumer::Instance().ResetOk();
        if (File::Exist(BASE_PATH))
        {
            File::RemoveDir(BASE_PATH, 0);
        }
    }
    void SetUp() override
    {
        DeliverableConsumer::Instance().SetBeforeWriteForTest({});
        DeliverableConsumer::Instance().ResetOk();
    }
    void TearDown() override
    {
        DeliverableConsumer::Instance().SetBeforeWriteForTest({});
        DeliverableConsumer::Instance().FinishTimeline();
        DeliverableConsumer::Instance().CloseAllDb();
        DeliverableConsumer::Instance().ResetOk();
    }
};

TEST_F(DeliverableConsumerUTest, ShouldNotCreateJsonWhenNoEvents)
{
    const std::string prof = File::PathJoin({BASE_PATH, "empty_json"});
    const std::string output = File::PathJoin({prof, Analysis::Common::OUTPUT_PATH});
    PrepareDir(prof);
    PrepareDir(output);
    auto &consumer = DeliverableConsumer::Instance();
    consumer.BeginTimeline();
    consumer.AcceptJson({"msprof"}, prof, {});
    consumer.AcceptJson({"msprof"}, prof, {nullptr});
    consumer.AcceptJson({}, prof, {MakeEvent(1)});
    consumer.FinishTimeline();
    EXPECT_TRUE(FilesNamed(output, "msprof").empty());
    EXPECT_TRUE(consumer.Ok());
}

TEST_F(DeliverableConsumerUTest, ShouldWriteOneJsonArrayForSeveralBatchesAndPrefixes)
{
    const std::string prof = File::PathJoin({BASE_PATH, "json_ok"});
    const std::string output = File::PathJoin({prof, Analysis::Common::OUTPUT_PATH});
    PrepareDir(prof);
    PrepareDir(output);
    auto &consumer = DeliverableConsumer::Instance();
    consumer.BeginTimeline();
    std::vector<std::shared_ptr<TraceEvent>> first{MakeEvent(1)};
    std::vector<std::shared_ptr<TraceEvent>> second{MakeEvent(2), MakeEvent(3)};
    consumer.AcceptJson({"msprof", "step_trace"}, prof, first);
    consumer.AcceptJson({"msprof", "step_trace"}, prof, second);
    consumer.FinishTimeline();
    const auto msprofFiles = FilesNamed(output, "msprof_");
    const auto stepFiles = FilesNamed(output, "step_trace_");
    ASSERT_EQ(msprofFiles.size(), 1UL);
    ASSERT_EQ(stepFiles.size(), 1UL);
    const auto left = ReadJsonFile(msprofFiles.front());
    const auto right = ReadJsonFile(stepFiles.front());
    ASSERT_TRUE(left.is_array());
    ASSERT_EQ(left.size(), 3UL);
    EXPECT_EQ(left, right);
    EXPECT_TRUE(consumer.Ok());
}

TEST_F(DeliverableConsumerUTest, ShouldKeepJsonValidWhenAssemblersWriteTogether)
{
    const std::string prof = File::PathJoin({BASE_PATH, "json_parallel"});
    const std::string output = File::PathJoin({prof, Analysis::Common::OUTPUT_PATH});
    PrepareDir(prof);
    PrepareDir(output);
    auto &consumer = DeliverableConsumer::Instance();
    consumer.BeginTimeline();
    std::thread left([&]()
                     {
                         std::vector<std::shared_ptr<TraceEvent>> events;
                         for (uint32_t i = 0; i < 32; ++i)
                         {
                             events.emplace_back(MakeEvent(i));
                         }
                         consumer.AcceptJson({"msprof"}, prof, events);
                     });
    std::thread right([&]()
                      {
                          std::vector<std::shared_ptr<TraceEvent>> events;
                          for (uint32_t i = 100; i < 140; ++i)
                          {
                              events.emplace_back(MakeEvent(i));
                          }
                          consumer.AcceptJson({"msprof"}, prof, events);
                      });
    left.join();
    right.join();
    consumer.FinishTimeline();
    const auto files = FilesNamed(output, "msprof_");
    ASSERT_EQ(files.size(), 1UL);
    const auto content = ReadJsonFile(files.front());
    EXPECT_EQ(content.size(), 72UL);
    EXPECT_TRUE(consumer.Ok());
}

TEST_F(DeliverableConsumerUTest, ShouldFlushJsonSinkAtBatchAndDropDiscardedEvents)
{
    const std::string prof = File::PathJoin({BASE_PATH, "json_sink"});
    const std::string output = File::PathJoin({prof, Analysis::Common::OUTPUT_PATH});
    PrepareDir(prof);
    PrepareDir(output);
    JsonEventSink discarded;
    discarded.Open({{"msprof", FileCategory::MSPROF}}, prof);
    discarded.push_back(MakeEvent(1));
    EXPECT_FALSE(discarded.empty());
    discarded.Discard();
    EXPECT_TRUE(discarded.empty());
    DeliverableConsumer::Instance().FinishTimeline();
    EXPECT_TRUE(FilesNamed(output, "msprof").empty());

    JsonEventSink sink;
    sink.Open({{"msprof", FileCategory::MSPROF}}, prof);
    const size_t count = 4096 + 3;
    for (size_t i = 0; i < count; ++i)
    {
        sink.push_back(MakeEvent(static_cast<uint32_t>(i)));
    }
    EXPECT_FALSE(sink.empty());
    sink.Flush();
    DeliverableConsumer::Instance().FinishTimeline();
    const auto files = FilesNamed(output, "msprof_");
    ASSERT_EQ(files.size(), 1UL);
    EXPECT_EQ(ReadJsonFile(files.front()).size(), count);
    EXPECT_TRUE(FilesNamed(output, ".deliverable_spill_").empty());
}

TEST_F(DeliverableConsumerUTest, ShouldDropBatchedEventsWhenSinkIsDiscarded)
{
    const std::string prof = File::PathJoin({BASE_PATH, "json_discard_batch"});
    const std::string output = File::PathJoin({prof, Analysis::Common::OUTPUT_PATH});
    PrepareDir(prof);
    PrepareDir(output);
    JsonEventSink sink;
    sink.Open({{"msprof", FileCategory::MSPROF}}, prof);
    for (size_t i = 0; i < 4096 + 3; ++i)
    {
        sink.push_back(MakeEvent(static_cast<uint32_t>(i)));
    }
    EXPECT_FALSE(sink.empty());
    sink.Discard();
    EXPECT_TRUE(sink.empty());
    DeliverableConsumer::Instance().FinishTimeline();
    EXPECT_TRUE(FilesNamed(output, "msprof").empty());
    EXPECT_TRUE(FilesNamed(output, ".deliverable_spill_").empty());
}

// Assemblers share the msprof prefix. One sink already published; a later sink that
// spills a full batch and then Discard must not remove or replace that file.
TEST_F(DeliverableConsumerUTest, ShouldKeepFlushedEventsWhenAnotherSinkDiscardsTheSamePrefix)
{
    const std::string prof = File::PathJoin({BASE_PATH, "json_shared_prefix"});
    const std::string output = File::PathJoin({prof, Analysis::Common::OUTPUT_PATH});
    PrepareDir(prof);
    PrepareDir(output);
    JsonEventSink published;
    published.Open({{"msprof", FileCategory::MSPROF}}, prof);
    published.push_back(MakeEvent(7));
    published.Flush();

    JsonEventSink discarded;
    discarded.Open({{"msprof", FileCategory::MSPROF}}, prof);
    const size_t dropped = 4096 + 3;
    for (size_t i = 0; i < dropped; ++i)
    {
        discarded.push_back(MakeEvent(static_cast<uint32_t>(i + 100)));
    }
    EXPECT_FALSE(discarded.empty());
    discarded.Discard();
    EXPECT_TRUE(discarded.empty());
    DeliverableConsumer::Instance().FinishTimeline();
    const auto files = FilesNamed(output, "msprof_");
    ASSERT_EQ(files.size(), 1UL);
    const auto content = ReadJsonFile(files.front());
    ASSERT_TRUE(content.is_array());
    ASSERT_EQ(content.size(), 1UL);
    EXPECT_EQ(content[0]["pid"].get<unsigned>(), 7U);
    EXPECT_TRUE(FilesNamed(output, ".deliverable_spill_").empty());
    EXPECT_TRUE(DeliverableConsumer::Instance().Ok());
}

TEST_F(DeliverableConsumerUTest, ShouldReplaceJsonWhenTheSameProcessExportsAgain)
{
    const std::string prof = File::PathJoin({BASE_PATH, "json_replace"});
    const std::string output = File::PathJoin({prof, Analysis::Common::OUTPUT_PATH});
    PrepareDir(prof);
    PrepareDir(output);
    auto &consumer = DeliverableConsumer::Instance();
    consumer.BeginTimeline();
    consumer.AcceptJson({"msprof"}, prof, {MakeEvent(1)});
    consumer.FinishTimeline();
    consumer.BeginTimeline();
    consumer.AcceptJson({"msprof"}, prof, {MakeEvent(2), MakeEvent(3)});
    consumer.FinishTimeline();
    const auto files = FilesNamed(output, "msprof_");
    ASSERT_EQ(files.size(), 1UL);
    const auto content = ReadJsonFile(files.front());
    ASSERT_TRUE(content.is_array());
    EXPECT_EQ(content.size(), 2UL);
    EXPECT_TRUE(consumer.Ok());
}

TEST_F(DeliverableConsumerUTest, ShouldReplaceCsvWhenTheSamePathIsExportedAgain)
{
    const std::string dir = File::PathJoin({BASE_PATH, "csv_replace"});
    PrepareDir(dir);
    const std::string base = File::PathJoin({dir, "task_time"});
    auto &consumer = DeliverableConsumer::Instance();
    consumer.WriteCsv(base, {"h"}, {{"old"}}, {}, "fixed");
    consumer.WriteCsv(base, {"h"}, {{"new"}}, {}, "fixed");
    const auto files = FilesNamed(dir, "task_time");
    ASSERT_EQ(files.size(), 1UL);
    FileReader reader(files.front());
    std::vector<std::string> lines;
    ASSERT_EQ(reader.ReadText(lines), Analysis::ANALYSIS_OK);
    ASSERT_EQ(lines.size(), 2UL);
    EXPECT_EQ(lines[0], "h");
    EXPECT_EQ(lines[1], "new");
}

TEST_F(DeliverableConsumerUTest, ShouldReturnFromFinishWhenWriterThrowsWithOpsStillQueued)
{
    const std::string prof = File::PathJoin({BASE_PATH, "json_throw"});
    const std::string output = File::PathJoin({prof, Analysis::Common::OUTPUT_PATH});
    PrepareDir(prof);
    PrepareDir(output);
    std::mutex mu;
    std::condition_variable cv;
    bool entered = false;
    bool release = false;
    auto &consumer = DeliverableConsumer::Instance();
    consumer.SetBeforeWriteForTest([&]()
                                   {
                                       std::unique_lock<std::mutex> lock(mu);
                                       entered = true;
                                       cv.notify_all();
                                       cv.wait_for(lock, std::chrono::seconds(5), [&] { return release; });
                                       throw std::runtime_error("deliverable write failed");
                                   });
    consumer.BeginTimeline();
    std::thread writer([&]() { consumer.AcceptJson({"msprof"}, prof, {MakeEvent(1)}); });
    struct JoinWriter
    {
        std::thread &writer;
        std::mutex &mu;
        std::condition_variable &cv;
        bool &release;
        ~JoinWriter()
        {
            {
                std::lock_guard<std::mutex> lock(mu);
                release = true;
            }
            cv.notify_all();
            if (writer.joinable())
            {
                writer.join();
            }
        }
    } joinWriter{writer, mu, cv, release};
    {
        std::unique_lock<std::mutex> lock(mu);
        ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(5), [&] { return entered; }));
    }
    consumer.AcceptJson({"msprof"}, prof, {MakeEvent(2)});
    {
        std::lock_guard<std::mutex> lock(mu);
        release = true;
    }
    cv.notify_all();
    writer.join();
    consumer.SetBeforeWriteForTest({});
    consumer.FinishTimeline();
    EXPECT_FALSE(consumer.Ok());
}

TEST_F(DeliverableConsumerUTest, ShouldMarkNotOkWhenJsonDirectoryIsMissing)
{
    const std::string prof = File::PathJoin({BASE_PATH, "json_missing"});
    PrepareDir(prof);
    auto &consumer = DeliverableConsumer::Instance();
    consumer.BeginTimeline();
    consumer.AcceptJson({"msprof"}, prof, {MakeEvent(1)});
    consumer.FinishTimeline();
    EXPECT_FALSE(consumer.Ok());
}

TEST_F(DeliverableConsumerUTest, ShouldNotCreateCsvWhenThereAreNoRows)
{
    const std::string dir = File::PathJoin({BASE_PATH, "csv_empty"});
    PrepareDir(dir);
    const std::string base = File::PathJoin({dir, "task_time"});
    DeliverableConsumer::Instance().WriteCsv(base, {"h0", "h1"}, {}, {}, "t0");
    EXPECT_TRUE(FilesNamed(dir, "task_time").empty());
    DeliverableConsumer::Instance().FinishCsv(base);
}

TEST_F(DeliverableConsumerUTest, ShouldWriteMaskedCsvWithoutSliceWhenRowCountIsKnown)
{
    const std::string dir = File::PathJoin({BASE_PATH, "csv_small"});
    PrepareDir(dir);
    const std::string base = File::PathJoin({dir, "op_summary"});
    std::vector<std::vector<std::string>> rows{{"a", "b", "c", "d"}, {"e", "f", "g", "h"}};
    DeliverableConsumer::Instance().WriteCsv(base, {"c0", "c1", "c2", "c3"}, rows, {0, 2}, "t1");
    const auto files = FilesNamed(dir, "op_summary");
    ASSERT_EQ(files.size(), 1UL);
    EXPECT_EQ(files.front().find("slice"), std::string::npos);
    FileReader reader(files.front());
    std::vector<std::string> lines;
    ASSERT_EQ(reader.ReadText(lines), Analysis::ANALYSIS_OK);
    ASSERT_EQ(lines.size(), 3UL);
    // Masked columns 0 and 2 must not leave separators behind for the dropped leading column.
    EXPECT_EQ(lines[0], "c1,c3");
    EXPECT_EQ(lines[1], "b,d");
    EXPECT_EQ(lines[2], "f,h");
}

TEST_F(DeliverableConsumerUTest, ShouldStartInSliceModeWhenTotalExceedsLimit)
{
    const std::string dir = File::PathJoin({BASE_PATH, "csv_hint"});
    PrepareDir(dir);
    const std::string base = File::PathJoin({dir, "hint"});
    auto &consumer = DeliverableConsumer::Instance();
    consumer.StartCsv(base, {"h"}, {}, "t2", static_cast<uint64_t>(CSV_LIMIT) + 1);
    consumer.AppendCsv(base, {{"only"}});
    consumer.FinishCsv(base);
    const auto files = FilesNamed(dir, "hint");
    ASSERT_EQ(files.size(), 1UL);
    EXPECT_NE(files.front().find("_slice_0_"), std::string::npos);
}

TEST_F(DeliverableConsumerUTest, ShouldRenameFirstFileWhenRowsExceedTheUnslidedLimit)
{
    const std::string dir = File::PathJoin({BASE_PATH, "csv_rename"});
    PrepareDir(dir);
    const std::string base = File::PathJoin({dir, "rename"});
    auto &consumer = DeliverableConsumer::Instance();
    consumer.StartCsv(base, {"h"}, {}, "t3", 0);
    std::vector<std::vector<std::string>> batch(8192, {"x"});
    size_t left = static_cast<size_t>(CSV_LIMIT) + 1;
    while (left > 0)
    {
        if (left < batch.size())
        {
            batch.resize(left);
        }
        consumer.AppendCsv(base, batch);
        left -= batch.size();
    }
    consumer.FinishCsv(base);
    const auto files = FilesNamed(dir, "rename");
    ASSERT_EQ(files.size(), 2UL);
    EXPECT_NE(files[0].find("_slice_0_"), std::string::npos);
    EXPECT_NE(files[1].find("_slice_1_"), std::string::npos);
    FileReader first(files[0]);
    FileReader second(files[1]);
    std::vector<std::string> firstLines;
    std::vector<std::string> secondLines;
    ASSERT_EQ(first.ReadText(firstLines), Analysis::ANALYSIS_OK);
    ASSERT_EQ(second.ReadText(secondLines), Analysis::ANALYSIS_OK);
    EXPECT_EQ(firstLines.size(), static_cast<size_t>(CSV_LIMIT) + 1);
    EXPECT_EQ(secondLines.size(), 2UL);
    EXPECT_EQ(firstLines.front(), "h");
    EXPECT_EQ(secondLines.front(), "h");
}

TEST_F(DeliverableConsumerUTest, ShouldWriteDifferentCsvFilesInParallel)
{
    const std::string dir = File::PathJoin({BASE_PATH, "csv_parallel"});
    PrepareDir(dir);
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i)
    {
        threads.emplace_back([dir, i]()
                             {
                                 const std::string base = File::PathJoin({dir, "part" + std::to_string(i)});
                                 DeliverableConsumer::Instance().WriteCsv(base, {"h"}, {{std::to_string(i)}}, {}, "tp");
                             });
    }
    for (auto &thread : threads)
    {
        thread.join();
    }
    EXPECT_EQ(FilesNamed(dir, "part").size(), 4UL);
    EXPECT_TRUE(DeliverableConsumer::Instance().Ok());
}

TEST_F(DeliverableConsumerUTest, ShouldNotCreateDbWhenIndexArgumentsAreEmpty)
{
    const std::string dir = File::PathJoin({BASE_PATH, "db_empty"});
    PrepareDir(dir);
    const std::string dbPath = File::PathJoin({dir, "msprof_empty.db"});
    // Empty-rows rejection lives in SaveData, not in InsertRows, so this test only covers the
    // argument validation that the consumer itself owns.
    EXPECT_FALSE(DeliverableConsumer::Instance().CreateIndex("", "task", "idx", {"id"}));
    EXPECT_FALSE(File::Exist(dbPath));
}

TEST_F(DeliverableConsumerUTest, ShouldInsertAndIndexThroughOneConnection)
{
    const std::string dir = File::PathJoin({BASE_PATH, "db_ok"});
    PrepareDir(dir);
    const std::string dbPath = File::PathJoin({dir, "msprof_ok.db"});
    auto &consumer = DeliverableConsumer::Instance();
    std::vector<TableColumn> cols{{"id", "INTEGER", true}, {"name", "TEXT", false}};
    std::vector<std::tuple<int64_t, std::string>> first{{1, "a"}, {2, "b"}};
    std::vector<std::tuple<int64_t, std::string>> second{{3, "c"}};
    ASSERT_TRUE(consumer.InsertRows(dbPath, "task", cols, first));
    ASSERT_TRUE(consumer.InsertRows(dbPath, "task", cols, second));
    ASSERT_TRUE(consumer.CreateIndex(dbPath, "task", "task_id_idx", {"id"}));
    EXPECT_FALSE(consumer.InsertRows(dbPath, "task", {}, first));
    EXPECT_FALSE(consumer.Ok());
    consumer.ResetOk();
    consumer.CloseAllDb();

    Connection check(dbPath);
    ASSERT_TRUE(check.IsDBOpened());
    std::vector<std::tuple<int64_t, std::string>> result;
    ASSERT_TRUE(check.ExecuteQuery("SELECT id, name FROM task ORDER BY id;", result));
    ASSERT_EQ(result.size(), 3UL);
    EXPECT_EQ(std::get<0>(result[0]), 1);
    EXPECT_EQ(std::get<1>(result[2]), "c");
    std::vector<std::tuple<std::string>> indexes;
    ASSERT_TRUE(check.ExecuteQuery("SELECT name FROM sqlite_master WHERE type='index' AND name='task_id_idx';", indexes));
    EXPECT_EQ(indexes.size(), 1UL);
}

TEST_F(DeliverableConsumerUTest, ShouldSplitLargeDbInsertIntoBatches)
{
    const std::string dir = File::PathJoin({BASE_PATH, "db_batch"});
    PrepareDir(dir);
    const std::string dbPath = File::PathJoin({dir, "msprof_batch.db"});
    std::vector<TableColumn> cols{{"id", "INTEGER", true}};
    std::vector<std::tuple<int64_t>> rows;
    rows.reserve(200001);
    for (int64_t i = 0; i < 200001; ++i)
    {
        rows.emplace_back(i);
    }
    ASSERT_TRUE(DeliverableConsumer::Instance().InsertRows(dbPath, "batch_task", cols, rows));
    DeliverableConsumer::Instance().CloseAllDb();
    Connection check(dbPath);
    std::vector<std::tuple<int64_t>> result;
    ASSERT_TRUE(check.ExecuteQuery("SELECT id FROM batch_task WHERE id=200000;", result));
    ASSERT_EQ(result.size(), 1UL);
    EXPECT_EQ(std::get<0>(result[0]), 200000);
}
