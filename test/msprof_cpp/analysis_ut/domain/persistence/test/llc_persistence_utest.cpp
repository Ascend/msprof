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

#include <array>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_llc_pcie.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/persistence/device/llc_persistence.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "gtest/gtest.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const std::string LLC_ROOT = "./llc_persistence";
const std::string LLC_DEVICE_DIR = "./llc_persistence/device_0";
const std::string LLC_SQLITE_DIR = "./llc_persistence/device_0/sqlite";
const std::string LLC_DB_PATH = "./llc_persistence/device_0/sqlite/llc.db";

const std::array<uint32_t, 8> READ_EVENTS{{0x00, 0x01, 0x02, 0x13, 0x20, 0x22, 0x34, 0x36}};
const std::array<uint32_t, 8> WRITE_EVENTS{{0x00, 0x01, 0x03, 0x14, 0x21, 0x23, 0x35, 0x37}};

using OriginalRow = std::tuple<uint32_t, double, uint64_t, uint32_t, uint32_t>;
using EventRow =
    std::tuple<uint32_t, uint32_t, double, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
               uint64_t>;
using EventNullFlags =
    std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>;
using MetricRow = std::tuple<uint32_t, uint32_t, double, double, double>;

const std::string QUERY_LLC_EVENTS =
    "SELECT device_id,l3tid,timestamp,COALESCE(event0,0),COALESCE(event1,0),COALESCE(event2,0),"
    "COALESCE(event3,0),COALESCE(event4,0),COALESCE(event5,0),COALESCE(event6,0),COALESCE(event7,0) "
    "FROM LLCEvents ";

void AppendCompleteEventSet(std::vector<HalLlcData> &records, uint32_t deviceId, uint64_t timestamp, uint32_t l3tid,
                            const std::array<uint64_t, 8> &counts, const std::array<uint32_t, 8> &events)
{
    for (std::size_t index = 0; index < events.size(); ++index)
    {
        records.push_back({deviceId, timestamp, counts[index], events[index], l3tid});
    }
}

}  // namespace

class LlcPersistenceUtest : public testing::Test
{
   protected:
    void SetUp() override
    {
        if (Utils::File::Exist(LLC_ROOT))
        {
            ASSERT_TRUE(Utils::File::RemoveDir(LLC_ROOT, 0));
        }
        ASSERT_TRUE(Utils::File::CreateDir(LLC_ROOT));
        ASSERT_TRUE(Utils::File::CreateDir(LLC_DEVICE_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(LLC_SQLITE_DIR));
        records_ = std::make_shared<std::vector<HalLlcData>>();
        ASSERT_TRUE(dataInventory_.Inject(records_));
        context_.deviceContextInfo.deviceFilePath = LLC_DEVICE_DIR;
        context_.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
        context_.deviceContextInfo.sampleInfo.llcProfiling = "read";
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        if (Utils::File::Exist(LLC_ROOT))
        {
            ASSERT_TRUE(Utils::File::RemoveDir(LLC_ROOT, 0));
        }
    }

    uint32_t Run(const std::vector<HalLlcData> &records, const std::string &profiling = "read")
    {
        *records_ = records;
        context_.deviceContextInfo.sampleInfo.llcProfiling = profiling;
        LlcPersistence persistence;
        return persistence.Run(dataInventory_, context_);
    }

    Infra::DataInventory dataInventory_;
    DeviceContext context_;
    std::shared_ptr<std::vector<HalLlcData>> records_;
};

TEST_F(LlcPersistenceUtest, ShouldSaveReadEventsOriginalRowsAndThreeDecimalMetrics)
{
    std::vector<HalLlcData> records;
    AppendCompleteEventSet(records, 7, 1000, 0, {{10, 20, 3, 4, 6, 5, 6, 8}}, READ_EVENTS);
    AppendCompleteEventSet(records, 7, 1064, 0, {{30, 34, 3, 4, 6, 5, 6, 8}}, READ_EVENTS);

    ASSERT_EQ(ANALYSIS_OK, Run(records));

    Infra::DBRunner runner(LLC_DB_PATH);
    std::vector<OriginalRow> originalRows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM LLCOriginalData ORDER BY rowid", originalRows));
    ASSERT_EQ(16UL, originalRows.size());
    EXPECT_EQ(OriginalRow(7, 1000.0, 10, 0x00, 0), originalRows.front());
    EXPECT_EQ(OriginalRow(7, 1064.0, 8, 0x36, 0), originalRows.back());

    std::vector<EventRow> eventRows;
    ASSERT_TRUE(runner.QueryData(QUERY_LLC_EVENTS + "ORDER BY device_id,l3tid,timestamp", eventRows));
    ASSERT_EQ(2UL, eventRows.size());
    EXPECT_EQ(EventRow(7, 0, 1000.0, 10, 20, 3, 4, 6, 5, 6, 8), eventRows.front());
    EXPECT_EQ(EventRow(7, 0, 1064.0, 30, 34, 3, 4, 6, 5, 6, 8), eventRows.back());

    std::vector<MetricRow> metricRows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM LLCMetrics ORDER BY rowid", metricRows));
    ASSERT_EQ(2UL, metricRows.size());
    EXPECT_EQ(MetricRow(7, 0, 1000.0, 1.667, 0.0), metricRows.front());
    EXPECT_EQ(MetricRow(7, 0, 1064.0, 1.667, 3906250.0), metricRows.back());
}

TEST_F(LlcPersistenceUtest, ShouldUseWriteMapKeepLastDuplicateAndPersistNullableEvents)
{
    std::vector<HalLlcData> records;
    AppendCompleteEventSet(records, 0, 200, 0, {{1, 2, 3, 4, 5, 6, 7, 8}}, WRITE_EVENTS);
    records.push_back({0, 200, 99, 0x00, 0});
    records.push_back({0, 200, 123, 0x02, 0});
    records.push_back({0, 300, 7, 0x00, 0});

    ASSERT_EQ(ANALYSIS_OK, Run(records, "write"));

    Infra::DBRunner runner(LLC_DB_PATH);
    std::vector<OriginalRow> originalRows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM LLCOriginalData ORDER BY rowid", originalRows));
    EXPECT_EQ(11UL, originalRows.size());

    std::vector<EventRow> eventRows;
    ASSERT_TRUE(runner.QueryData(QUERY_LLC_EVENTS + "ORDER BY timestamp", eventRows));
    ASSERT_EQ(2UL, eventRows.size());
    EXPECT_EQ(EventRow(0, 0, 200.0, 99, 2, 3, 4, 5, 6, 7, 8), eventRows.front());
    EXPECT_EQ(7U, std::get<3>(eventRows.back()));

    std::vector<EventNullFlags> nullFlags;
    ASSERT_TRUE(runner.QueryData(
        "SELECT event0 IS NULL,event1 IS NULL,event2 IS NULL,event3 IS NULL,event4 IS NULL,event5 IS NULL,"
        "event6 IS NULL,event7 IS NULL FROM LLCEvents ORDER BY timestamp",
        nullFlags));
    ASSERT_EQ(2UL, nullFlags.size());
    EXPECT_EQ(EventNullFlags(0, 0, 0, 0, 0, 0, 0, 0), nullFlags.front());
    EXPECT_EQ(EventNullFlags(0, 1, 1, 1, 1, 1, 1, 1), nullFlags.back());
}

TEST_F(LlcPersistenceUtest, ShouldSortMetricsAndResetThroughputForEachDevice)
{
    const std::array<uint64_t, 8> counts{{1, 1, 1, 1, 1, 1, 1, 1}};
    std::vector<HalLlcData> records;
    AppendCompleteEventSet(records, 0, 200, 0, counts, READ_EVENTS);
    AppendCompleteEventSet(records, 1, 300, 0, counts, READ_EVENTS);
    AppendCompleteEventSet(records, 0, 100, 0, counts, READ_EVENTS);
    AppendCompleteEventSet(records, 0, 50, 1, counts, READ_EVENTS);
    AppendCompleteEventSet(records, 0, 50, 2, counts, READ_EVENTS);

    ASSERT_EQ(ANALYSIS_OK, Run(records));

    Infra::DBRunner runner(LLC_DB_PATH);
    std::vector<MetricRow> metricRows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM LLCMetrics ORDER BY rowid", metricRows));
    ASSERT_EQ(4UL, metricRows.size());
    EXPECT_EQ(MetricRow(0, 0, 100.0, 2.0, 0.0), metricRows[0]);
    EXPECT_EQ(MetricRow(0, 0, 200.0, 2.0, 78125.0), metricRows[1]);
    EXPECT_EQ(MetricRow(1, 0, 300.0, 2.0, 0.0), metricRows[2]);
    EXPECT_EQ(MetricRow(0, 1, 50.0, 2.0, 0.0), metricRows[3]);

    std::vector<EventRow> eventRows;
    ASSERT_TRUE(runner.QueryData(QUERY_LLC_EVENTS + "ORDER BY device_id,l3tid,timestamp", eventRows));
    EXPECT_EQ(5UL, eventRows.size());
}

TEST_F(LlcPersistenceUtest, ShouldReplaceExistingData)
{
    std::vector<HalLlcData> firstRecords;
    AppendCompleteEventSet(firstRecords, 0, 100, 0, {{1, 2, 3, 4, 5, 6, 7, 8}}, READ_EVENTS);
    ASSERT_EQ(ANALYSIS_OK, Run(firstRecords));

    std::vector<HalLlcData> secondRecords;
    AppendCompleteEventSet(secondRecords, 0, 200, 0, {{11, 12, 13, 14, 15, 16, 17, 18}}, READ_EVENTS);
    ASSERT_EQ(ANALYSIS_OK, Run(secondRecords));

    Infra::DBRunner runner(LLC_DB_PATH);
    std::vector<OriginalRow> originalRows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM LLCOriginalData ORDER BY rowid", originalRows));
    ASSERT_EQ(secondRecords.size(), originalRows.size());
    EXPECT_EQ(200.0, std::get<1>(originalRows.front()));

    std::vector<EventRow> eventRows;
    ASSERT_TRUE(runner.QueryData(QUERY_LLC_EVENTS + "ORDER BY rowid", eventRows));
    ASSERT_EQ(1UL, eventRows.size());
    EXPECT_EQ(200.0, std::get<2>(eventRows.front()));

    std::vector<MetricRow> metricRows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM LLCMetrics ORDER BY rowid", metricRows));
    ASSERT_EQ(1UL, metricRows.size());
    EXPECT_EQ(200.0, std::get<2>(metricRows.front()));
}

TEST_F(LlcPersistenceUtest, ShouldUseFourL3IdsForNonStarsChip)
{
    context_.deviceContextInfo.deviceInfo.chipID = CHIP_V3_1_0;
    ASSERT_EQ(ANALYSIS_OK, Run({{0, 100, 1, READ_EVENTS[0], 2}}));

    Infra::DBRunner runner(LLC_DB_PATH);
    std::vector<MetricRow> metricRows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM LLCMetrics ORDER BY rowid", metricRows));
    ASSERT_EQ(1UL, metricRows.size());
    EXPECT_EQ(MetricRow(0, 2, 100.0, 0.0, 0.0), metricRows.front());
}

TEST_F(LlcPersistenceUtest, ShouldNotCreateDatabaseForEmptyInput)
{
    ASSERT_EQ(ANALYSIS_OK, Run({}));
    EXPECT_FALSE(Utils::File::Exist(LLC_DB_PATH));
}

TEST_F(LlcPersistenceUtest, ShouldReturnErrorForMissingData)
{
    dataInventory_.RemoveRestData({});
    LlcPersistence persistence;
    EXPECT_EQ(ANALYSIS_ERROR, persistence.Run(dataInventory_, context_));
    EXPECT_FALSE(Utils::File::Exist(LLC_DB_PATH));
}

TEST_F(LlcPersistenceUtest, ShouldReturnErrorWhenSqliteDirectoryIsMissing)
{
    ASSERT_TRUE(Utils::File::RemoveDir(LLC_SQLITE_DIR, 0));
    EXPECT_EQ(ANALYSIS_ERROR, Run({{0, 100, 1, 0x00, 0}}));
    EXPECT_FALSE(Utils::File::Exist(LLC_DB_PATH));
}

TEST_F(LlcPersistenceUtest, ShouldReturnErrorWhenDatabasePathIsDirectory)
{
    ASSERT_TRUE(Utils::File::CreateDir(LLC_DB_PATH));
    EXPECT_EQ(ANALYSIS_ERROR, Run({{0, 100, 1, 0x00, 0}}));
}

}  // namespace Domain
}  // namespace Analysis
