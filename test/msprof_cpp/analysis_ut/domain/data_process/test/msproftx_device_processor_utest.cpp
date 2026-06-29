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
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/ai_task/msproftx_device_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Test;
namespace {
const int DEPTH = 0;
const std::string DEVICE_TX_PATH = "./msprof_tx_device";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "step_trace.db";
const std::string PROF_PATH_A = File::PathJoin({DEVICE_TX_PATH, "./PROF_0"});
const std::string TABLE_NAME = "StepTrace";

using DbDataType = std::vector<std::tuple<uint32_t, uint32_t, uint64_t, uint32_t, uint32_t, uint32_t>>;

DbDataType DATA_A{{0, 4294967295, 26248923229230, 2, 10, 11},
                  {0, 4294967295, 26248923229240, 2, 10, 11},  // mark data in aclgraph replay scene with index id 0
                  {1, 4294967295, 26248923229340, 2, 14, 12},
                  {1, 4294967295, 26248923229440, 2, 15, 12},
                  {2, 4294967295, 26248923229540, 2, 16, 12},
                  {2, 4294967295, 26248923229640, 2, 17, 12},
                  {1, 4294967295, 26248923229740, 2, 14, 12},  // range data in aclgraph replay scene with index id 1
                  {1, 4294967295, 26248923229840, 2, 15, 12}};
}

class MsprofTxDeviceProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(DEVICE_TX_PATH)) {
            File::RemoveDir(DEVICE_TX_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(DEVICE_TX_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE})));
        CreateStepTrace(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_A);
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(DEVICE_TX_PATH, DEPTH));
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
    static void CreateStepTrace(const std::string& dbPath, DbDataType& data)
    {
        std::shared_ptr<StepTraceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, StepTraceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(MsprofTxDeviceProcessorUTest, ShouldReturnTrueWhenProcessorRunSuccessWhenCannVersionIsNewerThan900)
{
    DataInventory dataInventory;
    auto processor = MsprofTxDeviceProcessor(PROF_PATH_A);
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069324370978"},
        {"endCollectionTimeEnd", "1701069338159976"},
        {"startClockMonotonicRaw", "10071129942580"},
        {"pid", "10"},
        {"hostCntvct", "65177261204177"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
        {"hostMonotonic", "651599377155020"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetCannVersion).stubs().will(returnValue(std::vector<uint16_t>{9, 1, 0}));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    auto res = dataInventory.GetPtr<std::vector<MsprofTxDeviceData>>();
    EXPECT_EQ(5ul, res->size());
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();
    MOCKER_CPP(&Context::GetCannVersion).reset();
}

TEST_F(MsprofTxDeviceProcessorUTest, ShouldReturnTrueWhenProcessorRunSuccessWhenCannVersionIsOlderThan900)
{
    DataInventory dataInventory;
    auto processor = MsprofTxDeviceProcessor(PROF_PATH_A);
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069324370978"},
        {"endCollectionTimeEnd", "1701069338159976"},
        {"startClockMonotonicRaw", "10071129942580"},
        {"pid", "10"},
        {"hostCntvct", "65177261204177"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
        {"hostMonotonic", "651599377155020"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetCannVersion).stubs().will(returnValue(std::vector<uint16_t>{}));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    auto res = dataInventory.GetPtr<std::vector<MsprofTxDeviceData>>();
    EXPECT_EQ(3ul, res->size());
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();
    MOCKER_CPP(&Context::GetCannVersion).reset();
}

TEST_F(MsprofTxDeviceProcessorUTest, ShouldReturnFalseWhenCheckFailed)
{
    DataInventory dataInventory;
    auto processor = MsprofTxDeviceProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(HOST_ID)));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Utils::FileReader::Check).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Utils::FileReader::Check).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();
}

TEST_F(MsprofTxDeviceProcessorUTest, ShouldReturnFalseWhenReserveException)
{
    DataInventory dataInventory;
    auto processor = MsprofTxDeviceProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    StubReserveFailureForVector<std::vector<MsprofTxDeviceData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    ResetReserveFailureForVector<std::vector<MsprofTxDeviceData>>();
}
