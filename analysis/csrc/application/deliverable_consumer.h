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

#ifndef ANALYSIS_APPLICATION_DELIVERABLE_CONSUMER_H
#define ANALYSIS_APPLICATION_DELIVERABLE_CONSUMER_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/entities/json_trace/include/trace_event.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"

namespace Analysis
{
namespace Utils
{
class ThreadPool;
}  // namespace Utils

namespace Application
{
// One consumer owns JSON, CSV and DB delivery. Assemblers only submit assembled
// records. The consumer decides when to write, how much, file names, CSV slices
// and empty-file suppression. JsonWriter and CsvWriter perform the file write.
// One DB file keeps one writer thread. Table, index and insert go through DBRunner.
class DeliverableConsumer
{
   public:
    static DeliverableConsumer &Instance();

    void ResetOk();
    void BeginTimeline();
    void AcceptJson(const std::vector<std::string> &prefixes, const std::string &profPath,
                    const std::vector<std::shared_ptr<Domain::TraceEvent>> &events);
    // Wait only for the files these prefixes resolve to.
    void SyncJson(const std::vector<std::string> &prefixes, const std::string &profPath);
    void FinishTimeline();

    void WriteCsv(const std::string &basePath, const std::vector<std::string> &headers,
                  const std::vector<std::vector<std::string>> &rows, const std::set<int> &mask,
                  const std::string &timestamp);
    void StartCsv(const std::string &basePath, const std::vector<std::string> &headers, const std::set<int> &mask,
                  const std::string &timestamp, uint64_t totalHint);
    void AppendCsv(const std::string &basePath, const std::vector<std::vector<std::string>> &rows);
    void FinishCsv(const std::string &basePath);

    bool RunDb(const std::string &dbPath, const std::function<bool(Infra::DBRunner &)> &job);
    bool CreateIndex(const std::string &dbPath, const std::string &table, const std::string &indexName,
                     const std::vector<std::string> &colNames);
    void CloseAllDb();

    template <typename... Args>
    bool InsertRows(const std::string &dbPath, const std::string &table, const std::vector<Infra::TableColumn> &cols,
                    const std::vector<std::tuple<Args...>> &rows)
    {
        // Empty rows are already rejected by SaveData before reaching here; repeating the check in this
        // layer would give "empty data" a second, divergent meaning. This only validates the arguments
        // that belong to the consumer itself.
        if (dbPath.empty() || table.empty())
        {
            return false;
        }
        return RunDb(dbPath,
                     [&](Infra::DBRunner &runner) -> bool
                     {
                         if (cols.empty())
                         {
                             ERROR("Create table failed, table or columns is empty.");
                             return false;
                         }
                         if (!runner.CreateTable(table, cols))
                         {
                             return false;
                         }
                         if (rows.size() <= DB_INSERT_BATCH)
                         {
                             return runner.InsertData(table, rows);
                         }
                         for (size_t offset = 0; offset < rows.size(); offset += DB_INSERT_BATCH)
                         {
                             const size_t count = std::min(DB_INSERT_BATCH, rows.size() - offset);
                             std::vector<std::tuple<Args...>> part;
                             part.reserve(count);
                             for (size_t i = 0; i < count; ++i)
                             {
                                 part.emplace_back(rows[offset + i]);
                             }
                             if (!runner.InsertData(table, part))
                             {
                                 return false;
                             }
                         }
                         return true;
                     });
    }

    bool Ok() const;
    // Unit test only. The callback runs on the writer thread before each file write.
    // A throw is contained by Drain and must leave Wait able to return.
    void SetBeforeWriteForTest(std::function<void()> hook);
    // Unit test only. True while this path still has its DB writer thread.
    bool HasDbWriterForTest(const std::string &dbPath);

   private:
    DeliverableConsumer();
    ~DeliverableConsumer();
    DeliverableConsumer(const DeliverableConsumer &) = delete;
    DeliverableConsumer &operator=(const DeliverableConsumer &) = delete;

    struct ByteStream;
    struct CsvStream;
    struct DbState;
    struct IoOp;

    friend class JsonEventSink;

    Utils::ThreadPool *EnsurePool();
    std::shared_ptr<ByteStream> GetJsonStream(const std::string &path);
    std::shared_ptr<CsvStream> GetCsvStream(const std::string &basePath);
    void Enqueue(const std::shared_ptr<ByteStream> &stream, IoOp op);
    void Drain(ByteStream *stream);
    void FailStream(ByteStream &stream);
    void Wait(ByteStream &stream);
    void AcceptSerialized(const std::vector<std::string> &prefixes, const std::string &profPath,
                          const std::string &body);
    std::shared_ptr<DbState> EnsureDb(const std::string &path);
    static void DbLoop(const std::shared_ptr<DbState> &state, const std::string &path);
    static std::string SerializeEvents(const std::vector<std::shared_ptr<Domain::TraceEvent>> &events);
    static std::string MakeJsonPath(const std::string &profPath, const std::string &prefix);
    static std::string MakeCsvPath(const CsvStream &stream);
    void AppendJsonBody(const std::shared_ptr<ByteStream> &stream, const std::string &body);
    void AppendCsvRows(const std::shared_ptr<CsvStream> &stream, const std::vector<std::vector<std::string>> &rows);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_DELIVERABLE_CONSUMER_H
