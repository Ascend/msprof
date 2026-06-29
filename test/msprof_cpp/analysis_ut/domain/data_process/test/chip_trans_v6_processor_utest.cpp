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
#include <vector>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/system/chip_trans_v6_processor.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"

using namespace Analysis::Domain;
using namespace Analysis::Utils;
namespace {
const int DEPTH = 0;
const std::string CHIP_TRAINS_PATH = "./chip_trains";
const std::string DB_PATH = File::PathJoin({CHIP_TRAINS_PATH, "msprof.db"});
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "chip_trans.db";
const std::string PROF_PATH = File::PathJoin({CHIP_TRAINS_PATH, "./PROF_000001"});
const std::string PCIE_TABLE_NAME = "PcieInfoV6";

const OriPcieV6Format DATA_PCIE{
    {0, 932007903232, 1262720385024, 80273355627200.0},
    {1, 8589934592,   137438953472,  80273355627200.0},
    {0, 0,            0,             80273355627200.0},
    {0, 12884901888,  146028888064,  80273355627200.0},
    {1, 0,            0,             80273355627200.0},
    {1, 0,            0,             80273355627200.0},
    {1, 55834574848,  292057776128,  80273355627200.0},
    {0, 111669149696, 167503724544,  80273355627200.0},
};
}
class ChipTransV6ProcessorUTest : public testing::Test {
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

    static void CreatePcieInfo(const std::string& dbPath, OriPcieV6Format data)
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

TEST_F(ChipTransV6ProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = ChipTransV6Processor(PROF_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS_V6));
}

TEST_F(ChipTransV6ProcessorUTest, TestRunShouldReturnFalseWhenProcessFailed)
{
    auto processor = ChipTransV6Processor(PROF_PATH);
    auto dataInventory = DataInventory();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<PcieInfoV6Data>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS_V6));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<PcieInfoV6Data>).reset();
}

TEST_F(ChipTransV6ProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    if (File::Exist(File::PathJoin({PROF_PATH, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}))) {
        EXPECT_TRUE(File::DeleteFile(File::PathJoin({PROF_PATH, DEVICE_SUFFIX, SQLITE, DB_SUFFIX})));
    }
    auto processor = ChipTransV6Processor(PROF_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS_V6));
}

TEST_F(ChipTransV6ProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).stubs().will(returnValue(CHECK_FAILED));
    auto processor = ChipTransV6Processor(PROF_PATH);
    auto dataInventory = DataInventory();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS_V6));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();
}

TEST_F(ChipTransV6ProcessorUTest, TestRunShouldReturnFalseWhenConstructDBRunnerFailed)
{
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    auto processor = ChipTransV6Processor(PROF_PATH);
    auto dataInventory = DataInventory();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS_V6));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(ChipTransV6ProcessorUTest, TestRunShouldReturnTrueWhenNoData)
{
    auto dbPath = File::PathJoin({PROF_PATH, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);

    MOCKER_CPP(&OriPcieV6Format::empty).stubs().will(returnValue(true));
    auto processor = ChipTransV6Processor(PROF_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CHIP_TRAINS_V6));
    MOCKER_CPP(&OriPcieV6Format::empty).reset();
}
