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
#include "analysis/csrc/domain/data_process/system/pcie_processor.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
const std::string PCIE_DIR = "./pcie";
const std::string MSPROF = "msprof.db";
const std::string DB_PATH = File::PathJoin({PCIE_DIR, MSPROF});
const std::string PROF = File::PathJoin({PCIE_DIR, "PROF"});
using ProcessedDataFormat = std::vector<PCIeData>;
// 一条异常的数据
const PCIeDataFormat PCIE_INVALID_DATA = {
    {88698127877400, 0, 1048575, 0, 0, 1048575, 0, 0, 1048575, 0, 0, 1048575, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

const PCIeDataFormat PCIE_DATA = {
    {88698147865670, 0, 0, 0, 1236, 67, 0, 104, 1, 0, 0, 0, 318, 426, 237, 0, 28, 0, 0, 0, 0, 160, 0},
    {88698167865210, 0, 0, 0, 1236, 67, 0, 104, 0, 0, 2, 0, 318, 431, 288, 0, 96, 0, 0, 4, 0, 160, 0},
    {88698187865220, 0, 0, 0, 1236, 67, 0, 104, 0, 0, 22, 0, 318, 431, 323, 0, 96, 0, 0, 12, 0, 160, 0},
    {88698207898080, 0, 0, 0, 1236, 67, 0, 104, 0, 0, 22, 0, 318, 461, 353, 0, 96, 0, 0, 12, 0, 160, 0},
    {88698227865150, 0, 0, 0, 1236, 67, 0, 104, 0, 0, 22, 0, 318, 465, 380, 0, 96, 0, 0, 12, 0, 160, 0},
};

class PCIeProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(PCIE_DIR));
        EXPECT_TRUE(File::CreateDir(PROF));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF, DEVICE_PREFIX + "0"})));
        EXPECT_TRUE(CreatePCIeDB(File::PathJoin({PROF, DEVICE_PREFIX + "0", SQLITE})));
    }

    static bool CreatePCIeDB(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<PCIeDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, PCIeDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        EXPECT_TRUE(dbRunner->CreateTable("PcieOriginalData", database->GetTableCols("PcieOriginalData")));
        EXPECT_TRUE(dbRunner->InsertData("PcieOriginalData", PCIE_INVALID_DATA));
        EXPECT_TRUE(dbRunner->InsertData("PcieOriginalData", PCIE_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(PCIE_DIR, 0));
    }

    virtual void SetUp()
    {
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
            {"hostMonotonic", "80471130547330"},
            {"devMonotonic", "30471130547330"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    virtual void TearDown()
    {
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
        GlobalMockObject::verify();
    }
};

TEST_F(PCIeProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = PCIeProcessor(PROF);
    DataInventory dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_PCIE));
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    uint16_t expectNum = PCIE_DATA.size();
    auto res = dataInventory.GetPtr<std::vector<PCIeData>>();
    EXPECT_EQ(expectNum, res->size());
}

TEST_F(PCIeProcessorUTest, TestProcessShouldReturnFalseWhenGetRecordOrCheckPathFailed)
{
    DataInventory dataInventory = DataInventory();
    // CheckPathAndTable failed
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).stubs().will(returnValue(CHECK_FAILED));
    auto processor1 = PCIeProcessor(PROF);
    EXPECT_FALSE(processor1.Run(dataInventory, PROCESSOR_NAME_PCIE));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();

    // GetProfTimeRecordInfo failed
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    auto processor2 = PCIeProcessor(PROF);
    EXPECT_FALSE(processor2.Run(dataInventory, PROCESSOR_NAME_PCIE));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
}

TEST_F(PCIeProcessorUTest, TestFormatDataShouldReturnFalseWhenFormatDataFailed)
{
    DataInventory dataInventory = DataInventory();
    // dataFormat empty
    MOCKER_CPP(&PCIeDataFormat::empty).stubs().will(returnValue(true));
    auto processor1 = PCIeProcessor(PROF);
    EXPECT_FALSE(processor1.Run(dataInventory, PROCESSOR_NAME_PCIE));
    MOCKER_CPP(&PCIeDataFormat::empty).reset();

    // Reserve failed
    Analysis::Test::StubReserveFailureForVector<ProcessedDataFormat>();
    auto processor2 = PCIeProcessor(PROF);
    EXPECT_FALSE(processor2.Run(dataInventory, PROCESSOR_NAME_PCIE));
    Analysis::Test::ResetReserveFailureForVector<ProcessedDataFormat>();

    // ProcessedDataFormat empty
    MOCKER_CPP(&ProcessedDataFormat::empty).stubs().will(returnValue(true));
    auto processor3 = PCIeProcessor(PROF);
    EXPECT_FALSE(processor3.Run(dataInventory, PROCESSOR_NAME_PCIE));
    MOCKER_CPP(&ProcessedDataFormat::empty).reset();
}

TEST_F(PCIeProcessorUTest, TestFormatDataShouldReturnFalseWhenGetDeviceIdByDevicePathFailed)
{
    DataInventory dataInventory = DataInventory();
    // GetDeviceIdByDevicePath get HOST_ID
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(HOST_ID)));
    auto processor = PCIeProcessor(PROF);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_PCIE));
    MOCKER_CPP(&ProcessedDataFormat::empty).reset();
}

TEST_F(PCIeProcessorUTest, TestRunShouldReturnFalseWhenProcessFailed)
{
    DataInventory dataInventory;
    auto processor = PCIeProcessor(PROF);
    // GetDeviceIdByDevicePath get HOST_ID
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<PCIeData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_PCIE));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<PCIeData>).reset();
}

TEST_F(PCIeProcessorUTest, TestRunShouldReturnFalseWhenProcessOneDeviceFailed)
{
    DataInventory dataInventory;
    auto processor = PCIeProcessor(PROF);

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(INVALID_DEVICE_ID)));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_PCIE));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();
}

TEST_F(PCIeProcessorUTest, TestLoadDataShouldReturnOriDataWhenDbIsNull)
{
    auto processor = PCIeProcessor(PROF);
    DBInfo pcieDB("pcie.db", "PcieOriginalData");
    pcieDB.dbRunner = nullptr;
    EXPECT_EQ(processor.LoadData("", pcieDB).size(), 0ul);
}
