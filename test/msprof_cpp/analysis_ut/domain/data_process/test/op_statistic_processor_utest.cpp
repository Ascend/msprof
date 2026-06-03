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
#include "analysis/csrc/domain//data_process/ai_task/op_statistic_processor.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Test;
using ProcessedFormat = std::vector<OpStatisticData>;

const std::string BASE_PATH = "./op_statistic";
const std::string DEVICE_SUFFIX = "device_0";
const std::string SQLITE_SUFFIX = "sqlite";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string PROF_PATH_B = File::PathJoin({BASE_PATH, "PROF_1"});
const std::string DB_SUFFIX = "op_counter.db";
const std::string TABLE_NAME = "op_report";
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};

const OriOpCountDataFormat OP_DATA = {
    {"RmsNormTactic",                   "AI_CORE", "1610", 20218924.375, 3900.125,  12558.338121, 93541.875,  4.833029},
    {"PagedAttentionMaskNdKernel",      "MIX_AIC", "720",  18795054.5,   23860.5,   26104.242361, 32020.75,   4.492674},
    {"AddBF16Tactic",                   "AI_CORE", "1600", 11507650.75,  2260,      7192.281719,  51141,      2.75073},
    {"UnpadFlashAttentionBF16NdKernel", "MIX_AIC", "80",   11179324,     137662.75, 139741.55,    147122.875, 2.672249}
};

class OpStatisticProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, 0);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        CreateOpMetricData(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), OP_DATA);
        CreateOpMetricData(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), OP_DATA);
    }
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, 0));
    }
    static void CreateOpMetricData(const std::string& dbPath, OriOpCountDataFormat data)
    {
        std::shared_ptr<OpCounterDB> database;
        MAKE_SHARED0_RETURN_VOID(database, OpCounterDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};


TEST_F(OpStatisticProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    for (auto path: PROF_PATHS) {
        auto processor = OpStatisticProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_OP_STATISTIC));
    }
}

TEST_F(OpStatisticProcessorUTest, TestRunShouldReturnTrueWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    for (auto path: PROF_PATHS) {
        auto processor = OpStatisticProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_OP_STATISTIC));
    }
}


TEST_F(OpStatisticProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    MOCKER_CPP(&Analysis::Utils::File::Check).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = OpStatisticProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_OP_STATISTIC));
    }
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(OpStatisticProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({BASE_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).stubs().will(returnValue(deviceList));
    auto processor = OpStatisticProcessor({File::PathJoin({BASE_PATH, "test"})});
    auto dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_OP_STATISTIC));
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}

TEST_F(OpStatisticProcessorUTest, TestRunShouldReturnFalseWhenReserveFailed)
{
    StubReserveFailureForVector<ProcessedFormat>();
    for (auto path: PROF_PATHS) {
        auto processor = OpStatisticProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_OP_STATISTIC));
    }
    ResetReserveFailureForVector<ProcessedFormat>();
}

TEST_F(OpStatisticProcessorUTest, TestRunShouldReturnFalseWhenConstructDBRunnerFailed)
{
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = OpStatisticProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_OP_STATISTIC));
    }
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(OpStatisticProcessorUTest, TestRunShouldReturnTureWhenProcessRunSuccessAndCheckData)
{
    auto dataInventory = DataInventory();
    for (auto path: PROF_PATHS) {
        auto processor = OpStatisticProcessor(path);
        EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_OP_STATISTIC));
    }
    auto res = dataInventory.GetPtr<std::vector<OpStatisticData>>();
    EXPECT_EQ(4ul, res->size());
    auto data = res->at(0);
    EXPECT_EQ("RmsNormTactic", data.opType);
    EXPECT_EQ("1610", data.count);
}
