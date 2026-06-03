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
#include "analysis/csrc/domain/data_process/ai_task/msproftx_host_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Domain::Environment;
namespace {
const int DEPTH = 0;
const std::string HOST_TX_PATH = "./msprof_tx_host";
const std::string DB_SUFFIX = "msproftx.db";
const std::string PROF_PATH_A = File::PathJoin({HOST_TX_PATH, "./PROF_0"});
// MsprofTx table data format
using MsprofTxDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint32_t,
    std::string, int32_t, uint64_t, uint64_t, uint64_t, int32_t, std::string>>;
// MsprofTxEx table data format
using MsprofTxExDataFormat = std::vector<std::tuple<uint32_t, uint32_t,
    std::string, uint64_t, uint64_t, uint64_t, std::string, std::string>>;

const MsprofTxDataFormat MSPROFTX_DATA = {
    {0, 0, 0, "marker", 0, 0, 19627611986845096, 19627611986845096, 0, "test"},
    {0, 0, 0, "push/pop", 0, 0, 19627611986854415, 19627611986854415, 0, "test"},
    {0, 0, 0, "start/end", 0, 0, 19627611986858995, 19627611993981237, 0, "test"},
};

const MsprofTxExDataFormat MSPROFTX_EX_DATA = {
    {0, 0, "marker_ex", 19627611986934385, 19627611986941564, 0, "domain", "test"}
};
}

class MsprofTxHostProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(HOST_TX_PATH)) {
            File::RemoveDir(HOST_TX_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(HOST_TX_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST, SQLITE})));
        CreateMsprofTx(File::PathJoin({PROF_PATH_A, HOST, SQLITE, DB_SUFFIX}));
        CreateMsprofTxEx(File::PathJoin({PROF_PATH_A, HOST, SQLITE, DB_SUFFIX}));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(HOST_TX_PATH, DEPTH));
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
    static void CreateMsprofTx(const std::string& dbPath)
    {
        std::shared_ptr<MsprofTxDB> database;
        MAKE_SHARED0_RETURN_VOID(database, MsprofTxDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols("MsprofTx");
        dbRunner->CreateTable("MsprofTx", cols);
        dbRunner->InsertData("MsprofTx", MSPROFTX_DATA);
    }
    static void CreateMsprofTxEx(const std::string& dbPath)
    {
        std::shared_ptr<MsprofTxDB> database;
        MAKE_SHARED0_RETURN_VOID(database, MsprofTxDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols("MsprofTxEx");
        dbRunner->CreateTable("MsprofTxEx", cols);
        dbRunner->InsertData("MsprofTxEx", MSPROFTX_EX_DATA);
    }
};

TEST_F(MsprofTxHostProcessorUTest, ShouldReturnTrueWhenProcessorRunSuccess)
{
    DataInventory dataInventory;
    auto processor = MsprofTxHostProcessor(PROF_PATH_A);
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069324370978"},
        {"endCollectionTimeEnd", "1701069338159976"},
        {"startClockMonotonicRaw", "36471129942580"},
        {"pid", "10"},
        {"hostCntvct", "65177261204177"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
        {"hostMonotonic", "651599377155020"},
    };
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_MSTX));
    auto res = dataInventory.GetPtr<std::vector<MsprofTxHostData>>();
    EXPECT_EQ(4ul, res->size());
}

TEST_F(MsprofTxHostProcessorUTest, ShouldReturnFalseWhenCheckFailed)
{
    DataInventory dataInventory;
    auto processor = MsprofTxHostProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MSTX));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MSTX));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Utils::FileReader::Check).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MSTX));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();
    MOCKER_CPP(&Utils::FileReader::Check).reset();
}

TEST_F(MsprofTxHostProcessorUTest, ShouldReturnFalseWhenReserveException)
{
    DataInventory dataInventory;
    auto processor = MsprofTxHostProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    Analysis::Test::StubReserveFailureForVector<std::vector<MsprofTxHostData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MSTX));
    Analysis::Test::ResetReserveFailureForVector<std::vector<MsprofTxHostData>>();
}
