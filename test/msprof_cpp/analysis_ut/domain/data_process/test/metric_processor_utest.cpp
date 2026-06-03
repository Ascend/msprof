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

#include <algorithm>
#include <vector>
#include <set>

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/data_process/ai_task/metric_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/metric_summary.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Domain::Environment;


const std::string METRIC_DIR = "./metric";
const std::string METRIC_SUMMARY_DB = "metric_summary.db";
const std::string PROF_PATH_A1 = File::PathJoin({METRIC_DIR, "PROF_PLATFORM_1"});
const std::string PROF_PATH_A2 = File::PathJoin({METRIC_DIR, "PROF_PLATFORM_5"});
const std::string TASK_BASED = "task-based";
const std::string SAMPLE_BASED = "sample-based";

using TempTaskFormatA1 = std::vector<std::tuple<double, double, double, double, double, double, double, double, double,
    double, uint32_t, uint32_t, double, double, uint32_t>>;

using TempTaskFormat = std::vector<std::tuple<double, double, double, double, double, double, double, double, double,
    double, double, uint32_t, uint32_t, uint32_t, double, double, uint32_t>>;

const TempTaskFormatA1 TASK_DATA_A1 = {
    {318.36, 14135332, 160, 0.5025757004648825, 180, 0.5653976630229928, 200, 0.6282196255811032,
        300, 0.9423294383716547, 20, 68, 21203781495130.004, 21203812838010.004, 0},
    {327.19, 14527061, 160, 0.489012500382041, 180, 0.5501390629297961, 200, 0.6112656254775513,
        300, 0.9168984382163269, 21, 68, 21203781495130.004, 21203813146290.004, 0},
    {313.94, 13939056, 160, 0.5096515257692553, 180, 0.5733579664904122, 200, 0.6370644072115691,
        300, 0.9555966108173536, 22, 68, 21203781495130.004, 21203813447710.004, 0},
    {315.17, 13993558, 160, 0.5076625313322969, 180, 0.571120347748834, 200, 0.6345781641653711,
        300, 0.9518672462480566, 23, 68, 21203781495130.004, 21203813750090.004, 0},
};

// 非stars的表头 当前C化代码不适用,仅用于检验memoryBound
const TableColumns TASK_METRIC_SUMMARY_A1 = {
    {"total_time", SQL_NUMERIC_TYPE},
    {"total_cycles", SQL_NUMERIC_TYPE},
    {"mac_time", SQL_NUMERIC_TYPE},
    {"mac_ratio", SQL_NUMERIC_TYPE},
    {"vec_time", SQL_NUMERIC_TYPE},
    {"vec_ratio", SQL_NUMERIC_TYPE},
    {"mte2_time", SQL_NUMERIC_TYPE},
    {"mte2_ratio", SQL_NUMERIC_TYPE},
    {"mte3_time", SQL_NUMERIC_TYPE},
    {"mte3_ratio", SQL_NUMERIC_TYPE},
    {"task_id", SQL_INTEGER_TYPE},
    {"stream_id", SQL_INTEGER_TYPE},
    {"start_time", SQL_NUMERIC_TYPE},
    {"end_time", SQL_NUMERIC_TYPE},
    {"batch_id", SQL_INTEGER_TYPE},
};


const TempTaskFormat TASK_DATA_A2 = {
    {318.36, 14135332, 197.9849315375118, 0.62189009780598, 160, 0.5025757004648825, 75.1, 6668981, 0.87503179271316,
        0.01165155516263, 0, 22, 68, 2344, 21203781495130.004, 21203812838010.004, 0},
    {327.19, 14527061, 197.98020025179216, 0.60509245469541, 160, 0.489012500382041, 76.41, 6785309, 0.87503201991243,
        0.01145179976328, 0, 22, 68, 2356, 21203781495130.004, 21203813146290.004, 0},
    {313.94, 13939056, 197.98647121440646, 0.63065066960058, 160, 0.5096515257692553, 61.91, 5497563, 0.8750522076782,
        0.01413426276334, 0, 22, 68, 2357, 21203781495130.004, 21203813447710.004, 0},
    {315.17, 13993558, 197.98357473417414, 0.62818026694855, 160, 0.5076625313322969, 61.88, 5495101, 0.87502004421757,
        0.01414059541399, 0, 22, 68, 2358, 21203781495130.004, 21203813750090.004, 0},
};

const TableColumns TASK_METRIC_SUMMARY_A2 = {
    {"aic_total_time", SQL_NUMERIC_TYPE},
    {"aic_total_cycles", SQL_NUMERIC_TYPE},
    {"aic_mac_time", SQL_NUMERIC_TYPE},
    {"aic_mac_ratio_extra", SQL_NUMERIC_TYPE},
    {"aic_mte1_time", SQL_NUMERIC_TYPE},
    {"aic_mte1_ratio", SQL_NUMERIC_TYPE},
    {"aiv_total_time", SQL_NUMERIC_TYPE},
    {"aiv_total_cycles", SQL_NUMERIC_TYPE},
    {"aiv_vec_time", SQL_NUMERIC_TYPE},
    {"aiv_vec_ratio", SQL_NUMERIC_TYPE},
    {"aiv_icache_miss_rate", SQL_NUMERIC_TYPE},
    {"task_id", SQL_INTEGER_TYPE},
    {"stream_id", SQL_INTEGER_TYPE},
    {"subtask_id", SQL_INTEGER_TYPE},
    {"start_time", SQL_NUMERIC_TYPE},
    {"end_time", SQL_NUMERIC_TYPE},
    {"batch_id", SQL_INTEGER_TYPE},
};

class MetricProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(METRIC_DIR));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A1));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A1, DEVICE_PREFIX + "0"})));
        EXPECT_TRUE(CreateMetricSummaryDBA1(File::PathJoin({PROF_PATH_A1, DEVICE_PREFIX + "0", SQLITE})));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A2));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A2, DEVICE_PREFIX + "0"})));
        EXPECT_TRUE(CreateMetricSummaryDBA2(File::PathJoin({PROF_PATH_A2, DEVICE_PREFIX + "0", SQLITE})));
    }

    // 两张表的表结构不一样 没法复用create函数
    static bool CreateMetricSummaryDBA1(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<DBRunner> metricRunner;
        MAKE_SHARED_RETURN_VALUE(metricRunner, DBRunner, false, File::PathJoin({sqlitePath, "metric_summary.db"}));
        EXPECT_TRUE(metricRunner->CreateTable("MetricSummary", TASK_METRIC_SUMMARY_A1));
        EXPECT_TRUE(metricRunner->InsertData("MetricSummary", TASK_DATA_A1));
        return true;
    }

    static bool CreateMetricSummaryDBA2(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<DBRunner> metricRunner;
        MAKE_SHARED_RETURN_VALUE(metricRunner, DBRunner, false, File::PathJoin({sqlitePath, "metric_summary.db"}));
        EXPECT_TRUE(metricRunner->CreateTable("MetricSummary", TASK_METRIC_SUMMARY_A2));
        EXPECT_TRUE(metricRunner->InsertData("MetricSummary", TASK_DATA_A2));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(METRIC_DIR, 0));
    }

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
};

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnTrueWhenRunA1SuccessToOnlyCheckMemoryBound)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}}, // 未设置aicore num, cube usage 流程中会被置NA
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "1"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH_A1);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    auto metricSummary = dataInventory.GetPtr<MetricSummary>();
    EXPECT_NE(metricSummary, nullptr);
    std::vector<std::string> headers;
    std::vector<std::string> expectedHeaders {"aicore_time(us)", "total_cycles", "mac_time(us)", "mac_ratio",
                                              "vec_time(us)", "vec_ratio", "mte2_time(us)", "mte2_ratio",
                                              "mte3_time(us)", "mte3_ratio", "memory_bound", "cube_utilization(%)"};
    headers.insert(headers.end(), metricSummary->labels.begin(), metricSummary->labels.end());
    EXPECT_EQ(expectedHeaders, headers);
    std::vector<std::vector<std::string>> metricData;
    std::vector<std::vector<std::string>> expectedMetricData {
        { "318.36", "14135332", "160.0", "0.503", "180.0", "0.565", "200.0", "0.628", "300.0",
          "0.942", "1.111504", "N/A" },
        { "327.19", "14527061", "160.0", "0.489", "180.0", "0.55", "200.0", "0.611", "300.0",
          "0.917", "1.110909", "N/A" },
        { "313.94", "13939056", "160.0", "0.51", "180.0", "0.573", "200.0", "0.637", "300.0",
          "0.956", "1.111693", "N/A" },
        { "315.17", "13993558", "160.0", "0.508", "180.0", "0.571", "200.0", "0.635", "300.0",
          "0.952", "1.112084", "N/A" }
    };
    for (auto processedDatum : metricSummary->data) {
        for (auto lineData : processedDatum.second) {
            metricData.emplace_back(lineData);
        }
    }
    EXPECT_EQ(expectedMetricData, metricData);
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnTrueWhenRunA2Success)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}, {"ai_core_num", 24}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH_A2);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    auto metricSummary = dataInventory.GetPtr<MetricSummary>();
    EXPECT_NE(metricSummary, nullptr);
    std::vector<std::string> headers;
    std::vector<std::string> expectedHeaders { "aicore_time(us)", "aic_total_cycles", "aic_mac_time(us)",
                                               "aic_mac_ratio", "aic_mte1_time(us)", "aic_mte1_ratio",
                                               "aiv_time(us)", "aiv_total_cycles", "aiv_vec_time(us)",
                                               "aiv_vec_ratio", "aiv_icache_miss_rate", "cube_utilization(%)"};
    headers.insert(headers.end(), metricSummary->labels.begin(), metricSummary->labels.end());
    EXPECT_EQ(expectedHeaders, headers);
    std::vector<std::vector<std::string>> metricData;
    std::vector<std::vector<std::string>> expectedMetricData {
        { "318.36", "14135332", "197.985", "0.622", "160.0", "0.503",
          "75.1", "6668981", "0.875", "0.012", "0.0", "58897.216667" },
        { "327.19", "14527061", "197.98", "0.605", "160.0", "0.489",
          "76.41", "6785309", "0.875", "0.011", "0.0", "60529.420833" },
        { "313.94", "13939056", "197.986", "0.631", "160.0", "0.51",
          "61.91", "5497563", "0.875", "0.014", "0.0", "58079.400000" },
        { "315.17", "13993558", "197.984", "0.628", "160.0", "0.508",
          "61.88", "5495101", "0.875", "0.014", "0.0", "58306.491667" }
    };
    for (auto processedDatum : metricSummary->data) {
        for (auto lineData : processedDatum.second) {
            metricData.emplace_back(lineData);
        }
    }
    EXPECT_EQ(expectedMetricData, metricData);
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnTrueWhenSampleBased)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", SAMPLE_BASED},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH_A2);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnFALSEWhenGetMetricModeFailed)
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
    MOCKER_CPP(&Context::GetMetricMode).stubs().will(returnValue(false));
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH_A2);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));

    MOCKER_CPP(&Context::GetMetricMode).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnFalseWhenCheckColumnsFailed)
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
    std::vector<std::string> emptyHeaders;
    MOCKER_CPP(&MetricProcessor::GetAndCheckTableColumns).stubs().will(returnValue(emptyHeaders));
    auto processor1 = MetricProcessor(PROF_PATH_A2);
    DataInventory dataInventory;
    EXPECT_FALSE(processor1.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));

    MOCKER_CPP(&MetricProcessor::GetAndCheckTableColumns).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnFalseWhenUpdateColumnsFailed)
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

    // ModifySummaryHeaders failed
    std::vector<std::string> emptyHeaders;
    MOCKER_CPP(&MetricProcessor::ModifySummaryHeaders).stubs().will(returnValue(emptyHeaders));
    auto processor1 = MetricProcessor(PROF_PATH_A2);
    DataInventory dataInventory;
    EXPECT_FALSE(processor1.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));

    MOCKER_CPP(&MetricProcessor::ModifySummaryHeaders).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnFalseWhenDeviceIDInvalid)
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

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(UINT16_MAX)));
    auto processor1 = MetricProcessor(PROF_PATH_A2);
    DataInventory dataInventory;
    EXPECT_FALSE(processor1.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}
TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnFalseWhenCheckPathAndTableNotExist)
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
    MOCKER_CPP(&MetricProcessor::CheckPathAndTable).stubs().will(returnValue(1));

    auto processor1 = MetricProcessor(PROF_PATH_A2);
    EXPECT_TRUE(processor1.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));

    MOCKER_CPP(&MetricProcessor::CheckPathAndTable).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnFalseWhenCheckPathAndTableFailed)
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
    const uint8_t CHECK_FAILED = 2;
    MOCKER_CPP(&MetricProcessor::CheckPathAndTable).stubs().will(returnValue(CHECK_FAILED));

    auto processor1 = MetricProcessor(PROF_PATH_A2);
    EXPECT_FALSE(processor1.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));

    MOCKER_CPP(&MetricProcessor::CheckPathAndTable).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnTrueWhenNoMetricSummaryDB)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    EXPECT_TRUE(File::RemoveDir(File::PathJoin({PROF_PATH_A2, DEVICE_PREFIX + "0"}), 0));

    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH_A2);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

const TableColumns MEMORY_ACCESS_METRIC_SUMMARY = {
    {"aicore_time", SQL_NUMERIC_TYPE},
    {"aic_total_cycles", SQL_NUMERIC_TYPE},
    {"aic_read_main_memory_datas", SQL_NUMERIC_TYPE},
    {"aic_write_main_memory_datas", SQL_NUMERIC_TYPE},
    {"aic_GM_to_L1_datas", SQL_NUMERIC_TYPE},
    {"aic_L0C_to_L1_datas", SQL_NUMERIC_TYPE},
    {"aic_L0C_to_GM_datas", SQL_NUMERIC_TYPE},
    {"aic_GM_to_UB_datas", SQL_NUMERIC_TYPE},
    {"aic_UB_to_GM_datas", SQL_NUMERIC_TYPE},
    {"aiv_total_time", SQL_NUMERIC_TYPE},
    {"aiv_total_cycles", SQL_NUMERIC_TYPE},
    {"aiv_read_main_memory_datas", SQL_NUMERIC_TYPE},
    {"aiv_write_main_memory_datas", SQL_NUMERIC_TYPE},
    {"aiv_GM_to_L1_datas", SQL_NUMERIC_TYPE},
    {"aiv_L0C_to_L1_datas", SQL_NUMERIC_TYPE},
    {"aiv_L0C_to_GM_datas", SQL_NUMERIC_TYPE},
    {"aiv_GM_to_UB_datas", SQL_NUMERIC_TYPE},
    {"aiv_UB_to_GM_datas", SQL_NUMERIC_TYPE},
    {"task_id", SQL_INTEGER_TYPE},
    {"stream_id", SQL_INTEGER_TYPE},
    {"subtask_id", SQL_INTEGER_TYPE},
    {"start_time", SQL_NUMERIC_TYPE},
    {"end_time", SQL_NUMERIC_TYPE},
    {"batch_id", SQL_INTEGER_TYPE}
};

using MS_TYPE = std::vector<std::tuple<double, double, double, double, double, double, double, double, double, double,
    double, double, double, double, double, double, double, double, uint32_t, uint32_t, uint32_t, double, double,
    uint16_t>>;

const MS_TYPE MS_DATA = {
    {3.515, 2812, 1.125, 0.125, 0.5, 0, 0.125, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, UINT32_MAX, 21203781495130.004,
     21203812838010.004, 0},
    {3.515, 2812, 1.125, 0.125, 0.5, 0, 0.125, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, UINT32_MAX, 21203781497130.004,
     21203812738010.004, 0}
};

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnTrueWhenRunMemoryAccessSuccess)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    auto path = File::PathJoin({PROF_PATH_A2, DEVICE_PREFIX + "0", SQLITE});
    std::shared_ptr<DBRunner> metricRunner;
    MAKE_SHARED_NO_OPERATION(metricRunner, DBRunner, File::PathJoin({path, "metric_summary.db"}));
    if (File::Check(METRIC_DIR)) {
        EXPECT_TRUE(File::RemoveDir(METRIC_DIR, 0));
    }
    EXPECT_TRUE(File::CreateDir(METRIC_DIR));
    EXPECT_TRUE(File::CreateDir(PROF_PATH_A2));
    EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A2, DEVICE_PREFIX + "0"})));
    EXPECT_TRUE(File::CreateDir(path));
    EXPECT_TRUE(metricRunner->CreateTable("MetricSummary", MEMORY_ACCESS_METRIC_SUMMARY));
    EXPECT_TRUE(metricRunner->InsertData("MetricSummary", MS_DATA));
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH_A2);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    MOCKER_CPP(&Context::IsChipV1).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));
    MOCKER_CPP(&Context::IsChipV1).reset();
}

TEST_F(MetricProcessorUTest, TestRunShouldReturnFalseWhenConstructDBRunnerFailed)
{
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH_A2);
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(MetricProcessorUTest, TestRemoveNeedlessColumnsShouldReturnEmptyWhenDataIsEmpty)
{
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH_A2);
    std::vector<TableColumn> tableColumns;
    ASSERT_EQ(processor.RemoveNeedlessColumns(tableColumns).size(), 0ul);
}

TEST_F(MetricProcessorUTest, TestFormatTaskBasedDataShouldReturnFalseWhenDataIsEmpty)
{
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH_A2);
    std::vector<std::vector<std::string>> oriData;
    std::map<TaskId, std::vector<std::vector<std::string>>> processedData;
    uint16_t deviceId;
    EXPECT_FALSE(processor.FormatTaskBasedData(oriData, processedData, deviceId));
}

TEST_F(MetricProcessorUTest, TestRunShouldReturnFalseWhenTaskBasedProcessByDeviceFailed)
{
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH_A2);
    MOCKER_CPP(&MetricProcessor::TaskBasedProcessByDevice).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));
    MOCKER_CPP(&MetricProcessor::TaskBasedProcessByDevice).reset();
}

TEST_F(MetricProcessorUTest, TestRunShouldReturnFalseWhenFormatTaskBasedDataFailed)
{
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH_A2);
    MOCKER_CPP(&MetricProcessor::FormatTaskBasedData).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));
    MOCKER_CPP(&MetricProcessor::FormatTaskBasedData).reset();
}
