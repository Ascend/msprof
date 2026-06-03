/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
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
#include "analysis/csrc/domain/data_process/ai_task/unified_pmu_processor.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;

// Original Sample Timeline Format:aicore/ai_vector_core中的AICoreOriginalData
// timestamp, task_cyc, coreid
using OSTFormat = std::vector<std::tuple<uint64_t, std::string, uint32_t>>;
// Original Sample Summary Format:aicore/ai_vector_core中的MetricSummary
// metric, value, coreid
using OSSFormat = std::vector<std::tuple<std::string, double, uint32_t>>;
// Original Task Format: 只取id + 对应字段的value
// stream_id, task_id, subtask_id, batch_id, value
using OTFormat = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, double>>;
// 手动设置task-based pmu表Format
// aic_total_time, aic_total_cycles, aic_mac_time, aic_mac_ratio_extra,
// aiv_total_time, aiv_total_cycles, aiv_vec_time, aiv_vec_ratio, task_id, stream_id, subtask_id,
// start_time, end_time, batch_id
using TempTaskFormat = std::vector<std::tuple<double, double, double, double, double, double, double, double,
        uint32_t, uint32_t, uint32_t, double, double, uint32_t>>;

const std::string UNIFIED_PMU_DIR = "./PMU";
const std::string MSPROF_DB = "msprof.db";
const std::string DB_PATH = File::PathJoin({UNIFIED_PMU_DIR, MSPROF_DB});
const std::string PROF_PATH = File::PathJoin({UNIFIED_PMU_DIR, "PROF_114514"});
const std::string TASK_BASED = "task-based";
const std::string SAMPLE_BASED = "sample-based";

const std::vector<std::tuple<uint64_t, uint32_t, uint64_t, uint32_t, std::string, std::string, std::string,
    std::string, std::string, std::string, std::string, std::string, std::string>> SAMPLE_TIMELINE = {
    {1, 0, 88885812174450, 0, "0", "0", "0", "0", "0", "0", "0", "0", "0"},
    {1, 0, 88885812182000, 1, "0", "0", "0", "0", "0", "0", "0", "0", "0"},
    {1, 0, 88885812187440, 2, "0", "0", "0", "0", "0", "0", "0", "0", "0"},
    {1, 0, 88885822175760, 0, "303770", "42452", "116329", "27651", "86395", "162419", "35857", "13793", "347"},
    {1, 0, 88885822182560, 1, "306253", "42140", "115151", "29956", "86321", "155588", "35627", "14004", "304"},
    {1, 0, 88885822187760, 2, "249423", "36166", "74243", "24100", "61766", "116532", "31727", "10212", "251"},
};

const OSSFormat SAMPLE_SUMMARY = {
    {"total_time(ms)", 734.951, 0},
    {"vec_ratio", 0.12857727930161, 0},
    {"mac_ratio", 0.36937700608612, 0},
    {"total_time(ms)", 735.67, 1},
    {"vec_ratio", 0.12561746435222, 1},
    {"mac_ratio", 0.36563948509522, 1},
    {"ub_read_bw(GB/s)", 0.123, 1},
    {"ub_write_bw(GB/s)", 0.12561746435222, 1},
    {"l1_read_bw(GB/s)", 0.36563948509522, 1},
};

const TempTaskFormat TASK_DATA = {
    {318.36, 14135332, 197.9849315375118, 0.62189009780598, 75.1, 6668981, 0.87503179271316, 0.01165155516263,
     22, 68, 2344, 21203781495130.004, 21203812838010.004, 0},
    {327.19, 14527061, 197.98020025179216, 0.60509245469541, 76.41, 6785309, 0.87503201991243, 0.01145179976328,
     22, 68, 2356, 21203781495130.004, 21203813146290.004, 0},
    {313.94, 13939056, 197.98647121440646, 0.63065066960058, 61.91, 5497563, 0.8750522076782, 0.01413426276334,
     22, 68, 2357, 21203781495130.004, 21203813447710.004, 0},
    {315.17, 13993558, 197.98357473417414, 0.62818026694855, 61.88, 5495101, 0.87502004421757, 0.01414059541399,
     22, 68, 2358, 21203781495130.004, 21203813750090.004, 0},
};

const TableColumns TASK_METRIC_SUMMARY = {
    {"aic_total_time", SQL_NUMERIC_TYPE},
    {"aic_total_cycles", SQL_NUMERIC_TYPE},
    {"aic_mac_time", SQL_NUMERIC_TYPE},
    {"aic_mac_ratio_extra", SQL_NUMERIC_TYPE},
    {"aiv_total_time", SQL_NUMERIC_TYPE},
    {"aiv_total_cycles", SQL_NUMERIC_TYPE},
    {"aiv_vec_time", SQL_NUMERIC_TYPE},
    {"aiv_vec_ratio", SQL_NUMERIC_TYPE},
    {"task_id", SQL_INTEGER_TYPE},
    {"stream_id", SQL_INTEGER_TYPE},
    {"subtask_id", SQL_INTEGER_TYPE},
    {"start_time", SQL_NUMERIC_TYPE},
    {"end_time", SQL_NUMERIC_TYPE},
    {"batch_id", SQL_INTEGER_TYPE},
};

class UnifiedPmuProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(UNIFIED_PMU_DIR));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, DEVICE_PREFIX + "0"})));
        EXPECT_TRUE(CreatePmuDB(File::PathJoin({PROF_PATH, DEVICE_PREFIX + "0", SQLITE})));
    }

    static bool CreatePmuDB(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<AicoreDB> aicDB;
        MAKE_SHARED0_RETURN_VALUE(aicDB, AicoreDB, false);
        std::shared_ptr<DBRunner> aicRunner;
        MAKE_SHARED_RETURN_VALUE(aicRunner, DBRunner, false, File::PathJoin({sqlitePath, "aicore.db"}));
        EXPECT_TRUE(aicRunner->CreateTable("AICoreOriginalData", aicDB->GetTableCols("AICoreOriginalData")));
        EXPECT_TRUE(aicRunner->InsertData("AICoreOriginalData", SAMPLE_TIMELINE));
        EXPECT_TRUE(aicRunner->CreateTable("MetricSummary", aicDB->GetTableCols("MetricSummary")));
        EXPECT_TRUE(aicRunner->InsertData("MetricSummary", SAMPLE_SUMMARY));

        std::shared_ptr<AiVectorCoreDB> aivDB;
        MAKE_SHARED0_RETURN_VALUE(aivDB, AiVectorCoreDB, false);
        std::shared_ptr<DBRunner> aivRunner;
        MAKE_SHARED_RETURN_VALUE(aivRunner, DBRunner, false, File::PathJoin({sqlitePath, "ai_vector_core.db"}));
        EXPECT_TRUE(aivRunner->CreateTable("AICoreOriginalData", aivDB->GetTableCols("AICoreOriginalData")));
        EXPECT_TRUE(aivRunner->InsertData("AICoreOriginalData", SAMPLE_TIMELINE));
        EXPECT_TRUE(aivRunner->CreateTable("MetricSummary", aivDB->GetTableCols("MetricSummary")));
        EXPECT_TRUE(aivRunner->InsertData("MetricSummary", SAMPLE_SUMMARY));

        std::shared_ptr<DBRunner> metricRunner;
        MAKE_SHARED_RETURN_VALUE(metricRunner, DBRunner, false, File::PathJoin({sqlitePath, "metric_summary.db"}));
        EXPECT_TRUE(metricRunner->CreateTable("MetricSummary", TASK_METRIC_SUMMARY));
        EXPECT_TRUE(metricRunner->InsertData("MetricSummary", TASK_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(UNIFIED_PMU_DIR, 0));
    }

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
    }
};

TEST_F(UnifiedPmuProcessorUTest, TestTaskRunShouldReturnTrueWhenRunSuccess)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    DataInventory dataInventory;
    auto processor = UnifiedPmuProcessor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_UNIFIED_PMU));
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(UnifiedPmuProcessorUTest, TestTaskRunShouldReturnFalseWhenCheckColumnsFailed)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));

    // GetAndCheckTableColumns false
    std::vector<std::string> emptyTableColumns;
    MOCKER_CPP(&UnifiedPmuProcessor::GetAndCheckTableColumns).stubs().will(returnValue(emptyTableColumns));
    DataInventory dataInventory1;
    auto processor1 = UnifiedPmuProcessor(PROF_PATH);
    EXPECT_FALSE(processor1.Run(dataInventory1, PROCESSOR_NAME_UNIFIED_PMU));
    MOCKER_CPP(&UnifiedPmuProcessor::GetAndCheckTableColumns).reset();
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(UnifiedPmuProcessorUTest, TestTaskRunShouldReturnFalseWhenFormatDataFailed)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };

    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    // dataFormat empty
    MOCKER_CPP(&OTFormat::empty).stubs().will(returnValue(true));
    DataInventory dataInventory1;
    auto processor1 = UnifiedPmuProcessor(PROF_PATH);
    EXPECT_FALSE(processor1.Run(dataInventory1, PROCESSOR_NAME_UNIFIED_PMU));
    MOCKER_CPP(&OTFormat::empty).reset();

    // Reserve failed
    Analysis::Test::StubReserveFailureForVector<std::vector<UnifiedTaskPmu>>();
    DataInventory dataInventory2;
    auto processor2 = UnifiedPmuProcessor(PROF_PATH);
    EXPECT_FALSE(processor1.Run(dataInventory2, PROCESSOR_NAME_UNIFIED_PMU));
    Analysis::Test::ResetReserveFailureForVector<std::vector<UnifiedTaskPmu>>();

    // ProcessedDataFormat empty
    MOCKER_CPP(&std::vector<UnifiedTaskPmu>::empty).stubs().will(returnValue(true));
    DataInventory dataInventory3;
    auto processor3 = UnifiedPmuProcessor(PROF_PATH);
    EXPECT_FALSE(processor3.Run(dataInventory1, PROCESSOR_NAME_UNIFIED_PMU));
    MOCKER_CPP(&std::vector<UnifiedTaskPmu>::empty).reset();

    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(UnifiedPmuProcessorUTest, TestSampleRunShouldReturnTrueWhenRunSuccess)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", SAMPLE_BASED},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    DataInventory dataInventory;
    auto processor = UnifiedPmuProcessor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_UNIFIED_PMU));
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(UnifiedPmuProcessorUTest, TestSampleRunShouldReturnFalseWhenTimelineOrSummaryFailed)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", SAMPLE_BASED},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));

    // timeline GetProfTimeRecordInfo false + summary FormatSampleBasedSummaryData false
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    MOCKER_CPP(&UnifiedPmuProcessor::FormatSampleBasedSummaryData).stubs().will(returnValue(false));
    DataInventory dataInventory1;
    auto processor1 = UnifiedPmuProcessor(PROF_PATH);
    EXPECT_FALSE(processor1.Run(dataInventory1, PROCESSOR_NAME_UNIFIED_PMU));
    MOCKER_CPP(&UnifiedPmuProcessor::FormatSampleBasedSummaryData).reset();
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    // timeline GetPmuFreq false
    MOCKER_CPP(&Context::GetPmuFreq).stubs().will(returnValue(false));
    DataInventory dataInventory2;
    auto processor2 = UnifiedPmuProcessor(PROF_PATH);
    EXPECT_FALSE(processor2.Run(dataInventory2, PROCESSOR_NAME_UNIFIED_PMU));
    MOCKER_CPP(&Context::GetPmuFreq).reset();

    // timeline FormatSampleBasedTimelineData false
    MOCKER_CPP(&UnifiedPmuProcessor::FormatSampleBasedTimelineData).stubs().will(returnValue(false));
    DataInventory dataInventory3;
    auto processor3 = UnifiedPmuProcessor(PROF_PATH);
    EXPECT_FALSE(processor3.Run(dataInventory3, PROCESSOR_NAME_UNIFIED_PMU));
    MOCKER_CPP(&UnifiedPmuProcessor::FormatSampleBasedTimelineData).reset();

    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(UnifiedPmuProcessorUTest, TestSampleRunShouldReturnFalseWhenFormatDataFailed)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", SAMPLE_BASED},
    };

    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    // dataFormat empty
    MOCKER_CPP(&OSSFormat::empty).stubs().will(returnValue(true));
    MOCKER_CPP(&OSTFormat::empty).stubs().will(returnValue(true));
    DataInventory dataInventory1;
    auto processor1 = UnifiedPmuProcessor(PROF_PATH);
    EXPECT_FALSE(processor1.Run(dataInventory1, PROCESSOR_NAME_UNIFIED_PMU));
    MOCKER_CPP(&OSTFormat::empty).reset();
    MOCKER_CPP(&OSSFormat::empty).reset();

    // Reserve failed
    Analysis::Test::StubReserveFailureForVector<std::vector<UnifiedSampleTimelinePmu>>();
    Analysis::Test::StubReserveFailureForVector<std::vector<UnifiedSampleSummaryPmu>>();
    DataInventory dataInventory2;
    auto processor2 = UnifiedPmuProcessor(PROF_PATH);
    EXPECT_FALSE(processor2.Run(dataInventory2, PROCESSOR_NAME_UNIFIED_PMU));
    Analysis::Test::ResetReserveFailureForVector<std::vector<UnifiedSampleTimelinePmu>>();
    Analysis::Test::ResetReserveFailureForVector<std::vector<UnifiedSampleSummaryPmu>>();

    // ProcessedDataFormat empty
    MOCKER_CPP(&std::vector<UnifiedTaskPmu>::empty).stubs().will(returnValue(true));
    MOCKER_CPP(&std::vector<UnifiedSampleSummaryPmu>::empty).stubs().will(returnValue(true));
    DataInventory dataInventory3;
    auto processor3 = UnifiedPmuProcessor(PROF_PATH);
    EXPECT_FALSE(processor3.Run(dataInventory1, PROCESSOR_NAME_UNIFIED_PMU));
    MOCKER_CPP(&std::vector<UnifiedSampleSummaryPmu>::empty).reset();
    MOCKER_CPP(&std::vector<UnifiedTaskPmu>::empty).reset();

    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(UnifiedPmuProcessorUTest, TestWhenNoDBShouldReturnTrue)
{
    nlohmann::json record = {
        {"ai_core_profiling_mode", SAMPLE_BASED},
        {"platform_version", "5"},
    };
    std::string profTest = File::PathJoin({UNIFIED_PMU_DIR, "PROF_TEST"});
    EXPECT_TRUE(File::CreateDir(profTest));
    EXPECT_TRUE(File::CreateDir(File::PathJoin({profTest, DEVICE_PREFIX + "0"})));
    EXPECT_TRUE(File::CreateDir(File::PathJoin({profTest, DEVICE_PREFIX + "0", SQLITE})));

    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    auto processor1 = UnifiedPmuProcessor(profTest);
    DataInventory dataInventory1;
    EXPECT_TRUE(processor1.Run(dataInventory1, PROCESSOR_NAME_UNIFIED_PMU));
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();

    record = {
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    auto processor2 = UnifiedPmuProcessor(profTest);
    DataInventory dataInventory2;
    EXPECT_TRUE(processor2.Run(dataInventory1, PROCESSOR_NAME_UNIFIED_PMU));
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();

    EXPECT_TRUE(File::RemoveDir(profTest, 0));
}
