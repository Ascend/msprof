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
#include <vector>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/system/chip_trans_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Test;
namespace {
const int DEPTH = 0;
const std::string CHIP_TRAINS_PATH = "./chip_trains";
const std::string DB_PATH = File::PathJoin({CHIP_TRAINS_PATH, "msprof.db"});
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "chip_trans.db";
const std::string PROF_PATH = File::PathJoin({CHIP_TRAINS_PATH, "./PROF_000001"});
const std::string PA_TABLE_NAME = "PaLinkInfo";
const std::string PCIE_TABLE_NAME = "PcieInfo";

const OriPaFormat DATA_PA{
    {0, "949187772416", "1374389534720", 80273335147180.0},
    {1, "8589934592",   "137438953472",  80273335147180.0},
    {2, "0",            "0",             80273335147180.0},
    {3, "0",            "0",             80273335147180.0},
    {4, "0",            "0",             80273335147180.0},
    {5, "0",            "0",             80273335147180.0},
    {6, "25769803776",  "81604378624",   80273335147180.0},
    {7, "137438953472", "412316860416",  80273335147180.0}
};

const OriPcieFormat DATA_PCIE{
    {0, 932007903232, 1262720385024, 80273355627200.0},
    {1, 8589934592,   137438953472,  80273355627200.0},
    {2, 0,            0,             80273355627200.0},
    {3, 12884901888,  146028888064,  80273355627200.0},
    {4, 0,            0,             80273355627200.0},
    {5, 0,            0,             80273355627200.0},
    {6, 55834574848,  292057776128,  80273355627200.0},
    {7, 111669149696, 167503724544,  80273355627200.0},
};
}
class ChipTrainsProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Exist(CHIP_TRAINS_PATH)) {
            File::RemoveDir(CHIP_TRAINS_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(CHIP_TRAINS_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, DEVICE_SUFFIX, SQLITE})));
        CreatePaLinkInfo(File::PathJoin({PROF_PATH, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_PA);
        CreatePcieInfo(File::PathJoin({PROF_PATH, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_PCIE);
        MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo)
            .stubs()
            .will(returnValue(true));
    }
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(CHIP_TRAINS_PATH, DEPTH));
        MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).reset();
    }
    static void CreatePaLinkInfo(const std::string& dbPath, OriPaFormat data)
    {
        std::shared_ptr<ChipTransDB> database;
        MAKE_SHARED0_RETURN_VOID(database, ChipTransDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(PA_TABLE_NAME);
        dbRunner->CreateTable(PA_TABLE_NAME, cols);
        dbRunner->InsertData(PA_TABLE_NAME, data);
    }
    static void CreatePcieInfo(const std::string& dbPath, OriPcieFormat data)
    {
        std::shared_ptr<ChipTransDB> database;
        MAKE_SHARED0_RETURN_VOID(database, ChipTransDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(PCIE_TABLE_NAME);
        dbRunner->CreateTable(PCIE_TABLE_NAME, cols);
        dbRunner->InsertData(PCIE_TABLE_NAME, data);
    }
};

TEST_F(ChipTrainsProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = ChipTransProcessor(PROF_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS));
}

TEST_F(ChipTrainsProcessorUTest, TestRunShouldReturnFalseWhenProcessFailed)
{
    auto processor = ChipTransProcessor(PROF_PATH);
    auto dataInventory = DataInventory();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<PaLinkInfoData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<PaLinkInfoData>).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<PcieInfoData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<PcieInfoData>).reset();
}

TEST_F(ChipTrainsProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    if (File::Exist(File::PathJoin({PROF_PATH, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}))) {
        EXPECT_TRUE(File::DeleteFile(File::PathJoin({PROF_PATH, DEVICE_SUFFIX, SQLITE, DB_SUFFIX})));
    }
    auto processor = ChipTransProcessor(PROF_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS));
}

TEST_F(ChipTrainsProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(PA_TABLE_NAME);
    dbRunner->DropTable(PCIE_TABLE_NAME);
    auto processor = ChipTransProcessor(PROF_PATH);
    auto dataInventory = DataInventory();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS));
}

TEST_F(ChipTrainsProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    MOCKER_CPP(&Analysis::Utils::File::Check).stubs().will(returnValue(false));
    auto processor = ChipTransProcessor(PROF_PATH);
    auto dataInventory = DataInventory();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS));
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(ChipTrainsProcessorUTest, TestRunShouldReturnFalseWhenReserveFailed)
{
    StubReserveFailureForVector<std::vector<PaLinkInfoData>>();
    StubReserveFailureForVector<std::vector<PcieInfoData>>();
    auto processor = ChipTransProcessor(PROF_PATH);
    auto dataInventory = DataInventory();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS));
    ResetReserveFailureForVector<std::vector<PaLinkInfoData>>();
    ResetReserveFailureForVector<std::vector<PcieInfoData>>();
}

TEST_F(ChipTrainsProcessorUTest, TestRunShouldReturnFalseWhenConstructDBRunnerFailed)
{
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    auto processor = ChipTransProcessor(PROF_PATH);
    auto dataInventory = DataInventory();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(ChipTrainsProcessorUTest, TestRunShouldReturnTrueWhenNoData)
{
    auto dbPath = File::PathJoin({PROF_PATH, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    MOCKER_CPP(&OriPaFormat::empty).stubs().will(returnValue(true));
    auto processor = ChipTransProcessor(PROF_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS));
    MOCKER_CPP(&OriPaFormat::empty).reset();

    MOCKER_CPP(&OriPcieFormat::empty).stubs().will(returnValue(true));
    auto processor1 = ChipTransProcessor(PROF_PATH);
    auto dataInventory1 = DataInventory();
    EXPECT_TRUE(processor1.Run(dataInventory1, PROCESSOR_NAME_CHIP_TRAINS));
    MOCKER_CPP(&OriPcieFormat::empty).reset();
}
