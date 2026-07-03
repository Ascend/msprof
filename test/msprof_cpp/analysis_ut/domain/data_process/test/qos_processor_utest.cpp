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
#include "analysis/csrc/domain/data_process/system/qos_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;

namespace {
const int DEPTH = 0;
const std::string QOS_DIR = "./qos";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "qos.db";
const std::string PROF_DIR = File::PathJoin({QOS_DIR, "./PROF_0"});
const std::string TABLE_NAME = "QosBwData";
const OriQosData QOS_DATA = {
    {120332804861006.51, 0, 217, 0, 555, 0, 0, 0, 0, 0, 0, 0},
    {120332804960128.49, 0, 220, 0, 530, 0, 0, 0, 0, 0, 0, 0},
    {120332805060230.49, 1, 220, 0, 566, 0, 0, 0, 0, 0, 0, 0},
    {120332805160232.488, 1, 227, 0, 456, 0, 0, 0, 0, 0, 0, 0},
    {120332805260134.484, 1, 238, 0, 741, 0, 0, 0, 0, 0, 0, 0}
};
}

class QosProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::CreateDir(QOS_DIR));
        EXPECT_TRUE(File::CreateDir(PROF_DIR));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE})));
        EXPECT_TRUE(CreateQosDB(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE, DB_SUFFIX})));
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
            {"hostMonotonic", "36471130547330"},
            {"devMonotonic", "36471130547330"},
            {"platform_version", "5"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    static bool CreateQosDB(const std::string& dbPath)
    {
        std::shared_ptr<QosDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, QosDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        EXPECT_TRUE(dbRunner->CreateTable(TABLE_NAME, database->GetTableCols(TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData(TABLE_NAME, QOS_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(QOS_DIR, DEPTH));
        GlobalMockObject::verify();
    }
};

TEST_F(QosProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = QosProcessor(PROF_DIR);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
}

TEST_F(QosProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = QosProcessor(PROF_DIR);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    // make DBRunner shared ptr failed
    MOCKER_CPP(&DBInfo::ConstructDBRunner)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    // db里面表不存在
    MOCKER_CPP(&DataProcessor::CheckPathAndTable)
        .stubs()
        .will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<QosData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<QosData>).reset();
}

TEST_F(QosProcessorUTest, TestRunShouldReturnFalseWhenProcessDataFailed)
{
    auto processor = QosProcessor(PROF_DIR);
    DataInventory dataInventory;
    // LoadData failed
    MOCKER_CPP(&OriQosData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    MOCKER_CPP(&OriQosData::empty).reset();
    // Reserve failed
    Analysis::Test::StubReserveFailureForVector<std::vector<QosData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    Analysis::Test::ResetReserveFailureForVector<std::vector<QosData>>();
}

TEST_F(QosProcessorUTest, TestRunShouldReturnFalseWhenProcessSingleDeviceFailed)
{
    auto processor = QosProcessor(PROF_DIR);
    DataInventory dataInventory;

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(INVALID_DEVICE_ID)));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();
}

TEST_F(QosProcessorUTest, TestRunShouldReturnTrueWhenV6Platform)
{
    GlobalMockObject::verify();
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"hostMonotonic", "36471130547330"},
        {"devMonotonic", "36471130547330"},
        {"platform_version", "15"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));

    auto processor = QosProcessor(PROF_DIR);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));

    auto res = dataInventory.GetPtr<std::vector<QosData>>();
    ASSERT_NE(nullptr, res);
    ASSERT_FALSE(res->empty());
    ASSERT_EQ(5U, res->size());
    std::vector<uint32_t> expectedBw3 = {555, 530, 566, 456, 741};
    std::vector<uint32_t> expectedDieId = {0, 0, 1, 1, 1};
    for (size_t i = 0; i < res->size(); i++) {
        EXPECT_EQ(expectedDieId[i], (*res)[i].dieId);
        EXPECT_EQ(expectedBw3[i], (*res)[i].bw1);
    }
}