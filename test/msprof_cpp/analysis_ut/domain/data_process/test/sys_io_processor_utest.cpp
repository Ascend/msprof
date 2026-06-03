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
#include "analysis/csrc/domain/data_process/system/sys_io_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
// device_id, replayid, timestamp, bandwidth, rxpacket, rxbyte, rxpackets, rxbytes, rxerrors, rxdropped,
// txpacket, txbyte, txpackets, txbytes, txerrors, txdropped, funcid
using SysIODataFormat = std::vector<std::tuple<uint16_t, uint16_t, double, uint32_t, double, double, uint32_t,
    uint32_t, uint32_t, uint32_t, double, double, uint32_t, uint32_t, uint32_t, uint32_t, uint16_t>>;

using SysIOReportDataFormat = std::vector<std::tuple<uint16_t, std::string, std::string, std::string, std::string,
    std::string, std::string, std::string, std::string, std::string, std::string, uint16_t>>;

using SysIOReceiveSendFormat = std::vector<std::tuple<uint16_t, double, double, double, double,
    double, double, double, double, double, uint16_t>>;

namespace {
const std::string SYS_IO_DIR = "./sys_io";
const std::string DEVICE_SUFFIX = "device_0";
const std::string NIC_DB = "nic.db";
const std::string ROCE_DB = "roce.db";
const std::string NIC_RECEIVE_SEND_DB = "nicreceivesend_table.db";
const std::string ROCE_RECEIVE_SEND_DB = "rocereceivesend_table.db";
const std::string PROF_DIR = File::PathJoin({SYS_IO_DIR, "PROF_0"});
const std::string NIC_ORI_DATA_TABLE_NAME = "NicOriginalData";
const std::string ROCE_ORI_DATA_TABLE_NAME = "RoceOriginalData";
const std::string NIC_REPORT_DATA_TABLE_NAME = "NicReportData";
const std::string ROCE_REPORT_DATA_TABLE_NAME = "RoceReportData";
const std::string NIC_RECEIVE_SEND_TABLE_NAME = "NicReceiveSend";
const std::string ROCE_RECEIVE_SEND_TABLE_NAME = "RoceReceiveSend";

const SysIODataFormat SYS_IO_DATA = {
    {0, 0, 236328380142660, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 236328388200700, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 236328400152980, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 236328408182900, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 236328420031060, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

const SysIOReportDataFormat SYS_IO_REPORT_DATA = {
    {2, "50839867980.0", "200000", "0.0", "0.0", "0.0", "0", "0", "0.0", "0", "0", 0}
};

const SysIOReceiveSendFormat SYS_IO_RECEIVE_SEND = {
    {2, 71735332046900, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {2, 71735344028140, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {2, 71735352036920, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {2, 71735364160360, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {2, 71735372066700, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};
}

class SysIOProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::CreateDir(SYS_IO_DIR));
        EXPECT_TRUE(File::CreateDir(PROF_DIR));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_PREFIX + "0"})));
        EXPECT_TRUE(CreateSysIODB(File::PathJoin({PROF_DIR, DEVICE_PREFIX + "0", SQLITE})));
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
            {"hostMonotonic", "36471130547330"},
            {"devMonotonic", "36471130547330"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    static bool CreateSysIODB(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));

        std::shared_ptr<NicDB> nicDb;
        MAKE_SHARED0_RETURN_VALUE(nicDb, NicDB, false);
        std::shared_ptr<DBRunner> nicDbRunner;
        MAKE_SHARED_RETURN_VALUE(nicDbRunner, DBRunner, false, File::PathJoin({sqlitePath, nicDb->GetDBName()}));
        EXPECT_TRUE(nicDbRunner->CreateTable("NicOriginalData", nicDb->GetTableCols("NicOriginalData")));
        EXPECT_TRUE(nicDbRunner->InsertData("NicOriginalData", SYS_IO_DATA));
        EXPECT_TRUE(nicDbRunner->CreateTable("NicReportData", nicDb->GetTableCols("NicReportData")));
        EXPECT_TRUE(nicDbRunner->InsertData("NicReportData", SYS_IO_REPORT_DATA));

        std::shared_ptr<NicReceiveSendDB> nicReceiveSendDb;
        MAKE_SHARED0_RETURN_VALUE(nicReceiveSendDb, NicReceiveSendDB, false);
        std::shared_ptr<DBRunner> nicReceiveSendDbRunner;
        MAKE_SHARED_RETURN_VALUE(nicReceiveSendDbRunner, DBRunner, false,
                                 File::PathJoin({sqlitePath, nicReceiveSendDb->GetDBName()}));
        EXPECT_TRUE(nicReceiveSendDbRunner->CreateTable("NicReceiveSend",
                                                        nicReceiveSendDb->GetTableCols("NicReceiveSend")));
        EXPECT_TRUE(nicReceiveSendDbRunner->InsertData("NicReceiveSend", SYS_IO_RECEIVE_SEND));

        std::shared_ptr<RoceDB> roceDb;
        MAKE_SHARED0_RETURN_VALUE(roceDb, RoceDB, false);
        std::shared_ptr<DBRunner> roceDbRunner;
        MAKE_SHARED_RETURN_VALUE(roceDbRunner, DBRunner, false, File::PathJoin({sqlitePath, roceDb->GetDBName()}));
        EXPECT_TRUE(roceDbRunner->CreateTable("RoceOriginalData", roceDb->GetTableCols("RoceOriginalData")));
        EXPECT_TRUE(roceDbRunner->InsertData("RoceOriginalData", SYS_IO_DATA));
        EXPECT_TRUE(roceDbRunner->CreateTable("RoceReportData", roceDb->GetTableCols("RoceReportData")));
        EXPECT_TRUE(roceDbRunner->InsertData("RoceReportData", SYS_IO_REPORT_DATA));

        std::shared_ptr<RoceReceiveSendDB> roceReceiveSendDb;
        MAKE_SHARED0_RETURN_VALUE(roceReceiveSendDb, RoceReceiveSendDB, false);
        std::shared_ptr<DBRunner> roceReceiveSendDbRunner;
        MAKE_SHARED_RETURN_VALUE(roceReceiveSendDbRunner, DBRunner, false,
                                 File::PathJoin({sqlitePath, roceReceiveSendDb->GetDBName()}));
        EXPECT_TRUE(roceReceiveSendDbRunner->CreateTable("RoceReceiveSend",
                                                         roceReceiveSendDb->GetTableCols("RoceReceiveSend")));
        EXPECT_TRUE(roceReceiveSendDbRunner->InsertData("RoceReceiveSend", SYS_IO_RECEIVE_SEND));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(SYS_IO_DIR, 0));
        GlobalMockObject::verify();
    }
};

TEST_F(SysIOProcessorUTest, TestNicRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = NicProcessor(PROF_DIR);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NIC));
}

TEST_F(SysIOProcessorUTest, TestRoceRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = RoCEProcessor(PROF_DIR);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_ROCE));
}

TEST_F(SysIOProcessorUTest, TestNicTimelineRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = NicTimelineProcessor(PROF_DIR);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NIC_TIMELINE));
}

TEST_F(SysIOProcessorUTest, TestRoceTimelineRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = RoCETimelineProcessor(PROF_DIR);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_ROCE_TIMELINE));
}

TEST_F(SysIOProcessorUTest, TestRunShouldReturnFalseWhenSysIOProcessorFail)
{
    auto processor = NicProcessor(PROF_DIR);
    DataInventory dataInventory;
    // 获取host侧的时间信息失败
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    // 获取deviceID失败，返回HOST_ID（64）
    MOCKER_CPP(&GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(HOST_ID)));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC));
    MOCKER_CPP(&GetDeviceIdByDevicePath).reset();
    // 创建dbRunner的shared_ptr失败
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    // NicOriginalData表不存在
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).stubs().will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();
    // NicReportData表不存在
    MOCKER_CPP(&DataProcessor::CheckPathAndTable)
        .stubs()
        .will(returnValue(CHECK_SUCCESS))
        .then(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();
}

TEST_F(SysIOProcessorUTest, TestProcessDataShouldReturnEmptyVectorWhenSysIOProcessorFail)
{
    auto processor = NicProcessor(PROF_DIR);
    DataInventory dataInventory;
    // 时间相关信息不存在
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).reset();
    // LoadData failed
    MOCKER_CPP(&OriSysIOData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC));
    MOCKER_CPP(&OriSysIOData::empty).reset();
    // Reserve failed
    Analysis::Test::StubReserveFailureForVector<std::vector<SysIOOriginalData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC));
    Analysis::Test::ResetReserveFailureForVector<std::vector<SysIOOriginalData>>();
}

TEST_F(SysIOProcessorUTest, TestProcessSummaryDataShouldReturnEmptyVectorWhenQueryDataFailed)
{
    auto processor = NicProcessor(PROF_DIR);
    DataInventory dataInventory;
    // 取到的数据为空
    MOCKER_CPP(&OriSysIOReportData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC));
    MOCKER_CPP(&OriSysIOReportData::empty).reset();
}

TEST_F(SysIOProcessorUTest, TestRunShouldReturnFalseWhenSysIOTimelineProcessorFail)
{
    auto processor = NicTimelineProcessor(PROF_DIR);
    DataInventory dataInventory;
    // 获取host侧的时间信息失败
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC_TIMELINE));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    // 获取Monotonic信息失败
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC_TIMELINE));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).reset();
    // 获取deviceID失败，返回HOST_ID（64）
    MOCKER_CPP(&GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(HOST_ID)));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC_TIMELINE));
    MOCKER_CPP(&GetDeviceIdByDevicePath).reset();
    // 创建dbRunner的shared_ptr失败
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC_TIMELINE));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    // NicReceiveSend表不存在
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).stubs().will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC_TIMELINE));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();
}

TEST_F(SysIOProcessorUTest, TestRunShouldReturnFalseWhenFormatDataFail)
{
    auto processor = NicTimelineProcessor(PROF_DIR);
    DataInventory dataInventory;
    Analysis::Test::StubReserveFailureForVector<std::vector<SysIOReceiveSendData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NIC_TIMELINE));
    Analysis::Test::ResetReserveFailureForVector<std::vector<SysIOReceiveSendData>>();
}
