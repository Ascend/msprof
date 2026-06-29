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
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/services/parser/host/cann/compact_info_parser.h"
#include "test/msprof_cpp/analysis_ut/fake/fake_trace_generator.h"

using namespace Analysis;
using namespace Analysis::Domain;
using namespace Analysis::Domain::Host::Cann;
using namespace Analysis::Utils;
using namespace Analysis::Domain::Environment;

const auto DATA_DIR = "./PROF";
const uint16_t DATA_NUM = 10;

class CompactInfoParserUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }

    virtual void TearDown()
    {
    }

    static void SetUpTestCase()
    {
        GenCompactInfoData(EventType::EVENT_TYPE_NODE_BASIC_INFO, MSPROF_REPORT_NODE_LEVEL);
        GenCompactInfoData(EventType::EVENT_TYPE_MEM_CPY, MSPROF_REPORT_NODE_LEVEL);
        GenCompactInfoData(EventType::EVENT_TYPE_TASK_TRACK, MSPROF_REPORT_NODE_LEVEL, 0, true);
        GenCompactInfoData(EventType::EVENT_TYPE_HCCL_OP_INFO, MSPROF_REPORT_NODE_LEVEL);
        GenCompactInfoData(EventType::EVENT_TYPE_NODE_ATTR_INFO, MSPROF_REPORT_NODE_LEVEL);
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(DATA_DIR, 0));
    }

    /* GenCompactInfoData数据构造：
     1. 生成(aging/unaging)的compact二进制数据文件，包括node_basic_info，node_attr_info，task_track，memcpy_info，hccl_op_info
     2. 前一半数据数据写入unaging文件，后一半数据写入aging文件
     3. 通过设置invalidDataNum，把最后invalidDataNum个数据改成无效数据，magicNumber设置成MSPROF_DATA_HEAD_MAGIC_NUM + 1
     4. 当生成task track数据时，倒数第2个数据的taskType设置成flipTaskType (task id翻转)，
        最后1个数据的taskType设置成maintenanceTaskType (流销毁task)
     可以看护的场景：
     1. unaging和aging文件中，node_basic_info，node_attr_info，task_track和memcpy_info数据的读取
     2. 设置invalidDataNum，验证无效数据的处理
     3. 对于task track数据，验证flipTask数据的读取和解析，以及maintenanceTask的过滤 */
    static void GenCompactInfoData(EventType type, uint16_t level,
                                   uint16_t invalidDataNum = 0, bool isTaskTrack = false)
    {
        const uint32_t dataLen = 8;
        const uint64_t flipTaskType = 98;
        const uint64_t maintenanceTaskType = 6;
        std::vector<MsprofCompactInfo> agingTraces;
        std::vector<MsprofCompactInfo> unAgingTraces;
        for (uint32_t i = 0; i < DATA_NUM; ++i) {
            MsprofCompactInfo info;
            info.level = level;
            info.type = static_cast<uint32_t>(type);
            info.threadId = i;
            info.dataLen = dataLen;
            info.timeStamp = DATA_NUM + i;
            if (i >= DATA_NUM - invalidDataNum) {
                info.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM + 1;
            }
            if (isTaskTrack) {
                // task track数据添加flip task和maintenance task
                if (i == DATA_NUM - 2) {  // 倒数第2个task track
                    info.data.runtimeTrack.taskType = flipTaskType;
                }
                if (i == DATA_NUM - 1) {  // 最后1个task track，
                    info.data.runtimeTrack.taskType = maintenanceTaskType;
                }
            }
            if (i * 2 < DATA_NUM) {  // agingTraces和unAgingTraces各生成DATA_NUM / 2个数据
                unAgingTraces.emplace_back(info);
            } else {
                agingTraces.emplace_back(info);
            }
        }
        auto fakeGen = std::make_shared<FakeTraceGenerator>(DATA_DIR);
        fakeGen->WriteBin<MsprofCompactInfo>(unAgingTraces, type, false);
        fakeGen->WriteBin<MsprofCompactInfo>(agingTraces, type, true);
    }

    static void Check(const std::vector<std::shared_ptr<MsprofCompactInfo>> &data,
                      EventType type, uint16_t level, uint16_t dataNum)
    {
        ASSERT_EQ(dataNum, data.size());
        const uint32_t dataLen = 8;
        for (size_t i = 0; i < dataNum; ++i) {
            EXPECT_EQ(MSPROF_DATA_HEAD_MAGIC_NUM, data[i]->magicNumber);
            EXPECT_EQ(level, data[i]->level);
            EXPECT_EQ(static_cast<uint32_t>(type), data[i]->type);
            EXPECT_EQ(i, data[i]->threadId);
            EXPECT_EQ(dataLen, data[i]->dataLen);
            EXPECT_EQ(DATA_NUM + i, data[i]->timeStamp);
        }
    }
};

TEST_F(CompactInfoParserUTest, TestMemcpyInfoParserShouldReturn10DataWhenParseSuccess)
{
    auto parser = std::make_shared<MemcpyInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_MEM_CPY, MSPROF_REPORT_NODE_LEVEL, DATA_NUM);
}

TEST_F(CompactInfoParserUTest, TestCompactInfoParserProduceDataShouldReturnEmptyWhenReserveFailed)
{
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).stubs().will(returnValue(false));
    auto parser = std::make_shared<MemcpyInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).reset();
}

TEST_F(CompactInfoParserUTest, TestCompactInfoParserProduceDataShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs()
        .will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<MemcpyInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(CompactInfoParserUTest, TestCompactInfoParserProduceDataShouldReturn9DataWhen1DataIsInvalid)
{
    const uint16_t invalidDataNum = 1;
    GenCompactInfoData(EventType::EVENT_TYPE_MEM_CPY, MSPROF_REPORT_NODE_LEVEL, invalidDataNum);
    auto parser = std::make_shared<MemcpyInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_MEM_CPY, MSPROF_REPORT_NODE_LEVEL, DATA_NUM - invalidDataNum);
}

TEST_F(CompactInfoParserUTest, TestCompactInfoParserProduceDataShouldReturnEmptyWhenReadBinaryFailed)
{
    MOCKER_CPP(&FileReader::ReadBinary)
        .stubs().will(returnValue(ANALYSIS_ERROR));
    auto parser = std::make_shared<MemcpyInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(CompactInfoParserUTest, TestNodeBasicInfoParserProduceDataShouldReturn10DataWhenParseSuccess)
{
    auto parser = std::make_shared<NodeBasicInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_NODE_BASIC_INFO, MSPROF_REPORT_NODE_LEVEL, DATA_NUM);
    for (size_t i = 0; i < DATA_NUM; ++i) {
        if (i * 2 < DATA_NUM) {  // 前1/2的数据是unaging，opState是0；后1/2的数据是aging，opState是1
            EXPECT_EQ(0, data[i]->data.nodeBasicInfo.opState);
        } else {
            EXPECT_EQ(1, data[i]->data.nodeBasicInfo.opState);
        }
    }
}

TEST_F(CompactInfoParserUTest, TestNodeBasicInfoParserProduceDataShouldReturnEmptyWhenReserveFailed)
{
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).stubs().will(returnValue(false));
    auto parser = std::make_shared<NodeBasicInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).reset();
}

TEST_F(CompactInfoParserUTest, TestNodeBasicInfoParserProduceDataShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs()
        .will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<NodeBasicInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(CompactInfoParserUTest, TestNodeBasicInfoParserProduceDataShouldReturnEmptyWhenReadUnagingBinaryFailed)
{
    MOCKER_CPP(&FileReader::ReadBinary)
        .stubs().will(returnValue(ANALYSIS_OK)).then(returnValue(ANALYSIS_ERROR));
    auto parser = std::make_shared<NodeBasicInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(CompactInfoParserUTest, TestNodeBasicInfoParserProduceDataShouldReturn9DataWhen1DataIsInvalid)
{
    const uint16_t invalidDataNum = 1;
    GenCompactInfoData(EventType::EVENT_TYPE_NODE_BASIC_INFO, MSPROF_REPORT_NODE_LEVEL, invalidDataNum);
    auto parser = std::make_shared<NodeBasicInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_NODE_BASIC_INFO, MSPROF_REPORT_NODE_LEVEL, DATA_NUM - invalidDataNum);
}

TEST_F(CompactInfoParserUTest, TestTaskTrackParserProduceDataShouldReturn8CompactInfoAnd1FlipTaskWhenParseSuccess)
{
    MOCKER_CPP(&Context::IsAllExport).stubs()
        .will(returnValue(true));
    const uint16_t flipTaskNum = 1;
    const uint16_t maintenanceTaskNum = 1;
    auto parser = std::make_shared<TaskTrackParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto compactInfo = parser->ParseData<MsprofCompactInfo>();
    auto flipTask = parser->ParseData<Adapter::FlipTask>();
    Check(compactInfo, EventType::EVENT_TYPE_TASK_TRACK, MSPROF_REPORT_NODE_LEVEL,
          DATA_NUM - flipTaskNum - maintenanceTaskNum);
    EXPECT_EQ(flipTaskNum, flipTask.size());
}

TEST_F(CompactInfoParserUTest, TestTaskTrackParserProduceDataShouldReturnEmptyWhenReserveFailed)
{
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).stubs().will(returnValue(false));
    auto parser = std::make_shared<TaskTrackParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto compactInfo = parser->ParseData<MsprofCompactInfo>();
    auto flipTask = parser->ParseData<Adapter::FlipTask>();
    EXPECT_EQ(0, compactInfo.size());
    EXPECT_EQ(0, flipTask.size());
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).reset();
}

TEST_F(CompactInfoParserUTest, TestTaskTrackParserProduceDataShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs()
        .will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<TaskTrackParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto compactInfo = parser->ParseData<MsprofCompactInfo>();
    auto flipTask = parser->ParseData<Adapter::FlipTask>();
    EXPECT_EQ(0, compactInfo.size());
    EXPECT_EQ(0, flipTask.size());
}

TEST_F(CompactInfoParserUTest, TestTaskTrackParserProduceDataShouldReturnEmptyWhenCreateFlipTaskFailed)
{
    MOCKER_CPP(&Adapter::Flip::CreateFlipTask).stubs().will(returnValue(std::shared_ptr<Adapter::FlipTask>{}));
    auto parser = std::make_shared<TaskTrackParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto compactInfo = parser->ParseData<MsprofCompactInfo>();
    auto flipTask = parser->ParseData<Adapter::FlipTask>();
    EXPECT_EQ(0, compactInfo.size());
    EXPECT_EQ(0, flipTask.size());
    MOCKER_CPP(&Adapter::Flip::CreateFlipTask).reset();
}

TEST_F(CompactInfoParserUTest, TestTaskTrackParserProduceDataShouldReturn7DataWhen3DataIsInvalid)
{
    const uint16_t invalidDataNum = 3;
    GenCompactInfoData(EventType::EVENT_TYPE_TASK_TRACK, MSPROF_REPORT_NODE_LEVEL, invalidDataNum, true);
    auto parser = std::make_shared<TaskTrackParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_TASK_TRACK, MSPROF_REPORT_NODE_LEVEL, DATA_NUM - invalidDataNum);
}

TEST_F(CompactInfoParserUTest, TestNodeAttrInfoParserShouldReturn10DataWhenParseSuccess)
{
    auto parser = std::make_shared<NodeAttrInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_NODE_ATTR_INFO, MSPROF_REPORT_NODE_LEVEL, DATA_NUM);
}

TEST_F(CompactInfoParserUTest, TestNodeAttrInfoParserProduceDataShouldReturnEmptyWhenReserveFailed)
{
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).stubs().will(returnValue(false));
    auto parser = std::make_shared<NodeAttrInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).reset();
}

TEST_F(CompactInfoParserUTest, TestNodeAttrInfoParserProduceDataShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs()
        .will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<NodeAttrInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(CompactInfoParserUTest, TestNodeAttrInfoParserProduceDataShouldReturn9DataWhen1DataIsInvalid)
{
    const uint16_t invalidDataNum = 1;
    GenCompactInfoData(EventType::EVENT_TYPE_NODE_ATTR_INFO, MSPROF_REPORT_NODE_LEVEL, invalidDataNum);
    auto parser = std::make_shared<NodeAttrInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_NODE_ATTR_INFO, MSPROF_REPORT_NODE_LEVEL, DATA_NUM - invalidDataNum);
}

TEST_F(CompactInfoParserUTest, TestHcclOpInfoParserShouldReturn10DataWhenParseSuccess)
{
    auto parser = std::make_shared<HcclOpInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_HCCL_OP_INFO, MSPROF_REPORT_NODE_LEVEL, DATA_NUM);
}

TEST_F(CompactInfoParserUTest, TestHcclOpInfoParserProduceDataShouldReturnEmptyWhenReserveFailed)
{
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).stubs().will(returnValue(false));
    auto parser = std::make_shared<HcclOpInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).reset();
}

TEST_F(CompactInfoParserUTest, TestHcclOpInfoParserProduceDataShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs()
        .will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<HcclOpInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(CompactInfoParserUTest, TestHcclOpInfoParserProduceDataShouldReturn9DataWhen1DataIsInvalid)
{
    const uint16_t invalidDataNum = 1;
    GenCompactInfoData(EventType::EVENT_TYPE_HCCL_OP_INFO, MSPROF_REPORT_NODE_LEVEL, invalidDataNum);
    auto parser = std::make_shared<HcclOpInfoParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_HCCL_OP_INFO, MSPROF_REPORT_NODE_LEVEL, DATA_NUM - invalidDataNum);
}



// ==================== DPU TaskTrackParser Tests ====================

static void GenDpuTrackData(uint16_t dataNum = DATA_NUM, uint16_t invalidDataNum = 0)
{
    const uint32_t dataLen = sizeof(MsprofDpuTrack);
    const uint16_t level = MSPROF_REPORT_NODE_LEVEL;
    std::vector<MsprofCompactInfo> agingTraces;
    std::vector<MsprofCompactInfo> unAgingTraces;
    for (uint32_t i = 0; i < dataNum; ++i) {
        MsprofCompactInfo info;
        info.level = level;
        info.type = static_cast<uint32_t>(EventType::EVENT_TYPE_INVALID);
        info.threadId = i;
        info.dataLen = dataLen;
        info.timeStamp = dataNum + i;
        if (i >= dataNum - invalidDataNum) {
            info.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM + 1;
        }
        MsprofDpuTrack track;
        track.deviceId = static_cast<uint16_t>(i);
        track.streamId = static_cast<uint16_t>(i * 2);
        track.taskId = i * 3;
        track.taskType = 1;
        track.startTime = static_cast<uint64_t>(i * 1000);
        info.data.dpuTrack = track;
        if (i * 2 < dataNum) {
            unAgingTraces.emplace_back(info);
        } else {
            agingTraces.emplace_back(info);
        }
    }
    auto fakeGen = std::make_shared<FakeTraceGenerator>(DATA_DIR);
    fakeGen->WriteBin<MsprofCompactInfo>(unAgingTraces, EventType::EVENT_TYPE_INVALID, false);
    fakeGen->WriteBin<MsprofCompactInfo>(agingTraces, EventType::EVENT_TYPE_INVALID, true);
}

TEST_F(CompactInfoParserUTest, TestDpuTaskTrackParserShouldReturn10DataWhenParseSuccess)
{
    GenDpuTrackData();
    auto parser = std::make_shared<DpuTaskTrackParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(data.size(), DATA_NUM);
    for (uint32_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(data[i]->data.dpuTrack.deviceId, i);
        EXPECT_EQ(data[i]->data.dpuTrack.streamId, i * 2);
        EXPECT_EQ(data[i]->data.dpuTrack.taskId, i * 3);
        EXPECT_EQ(data[i]->data.dpuTrack.startTime, i * 1000);
    }
}

TEST_F(CompactInfoParserUTest, TestDpuTaskTrackParserProduceDataShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs()
        .will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<DpuTaskTrackParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(CompactInfoParserUTest, TestDpuTaskTrackParserProduceDataShouldReturn8DataWhen2DataIsInvalid)
{
    const uint16_t invalidDataNum = 2;
    GenDpuTrackData(DATA_NUM, invalidDataNum);
    auto parser = std::make_shared<DpuTaskTrackParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(data.size(), DATA_NUM - invalidDataNum);
}

// ==================== TaskTrackParser DPU Kernel Name Collection Tests ====================

static void GenTaskTrackWithDPUData(const std::vector<uint32_t> &dpuIndices,
                                    const std::vector<uint64_t> &dpuKernelNames,
                                    uint16_t dataNum = DATA_NUM)
{
    const uint32_t dataLen = 8;
    const uint64_t dpuDeviceTypeBits = static_cast<uint64_t>(1) << 12;
    const uint16_t level = MSPROF_REPORT_RUNTIME_LEVEL;
    std::vector<MsprofCompactInfo> agingTraces;
    std::vector<MsprofCompactInfo> unAgingTraces;
    uint32_t dpuIdx = 0;
    for (uint32_t i = 0; i < dataNum; ++i) {
        MsprofCompactInfo info{};
        info.level = level;
        info.type = static_cast<uint32_t>(EventType::EVENT_TYPE_TASK_TRACK);
        info.threadId = i;
        info.dataLen = dataLen;
        info.timeStamp = DATA_NUM + i;
        info.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        info.data.runtimeTrack.deviceId = 0;
        info.data.runtimeTrack.streamId = 0;
        info.data.runtimeTrack.taskId = i;
        info.data.runtimeTrack.taskType = 1;
        info.data.runtimeTrack.kernelName = i + 100;
        if (dpuIdx < dpuIndices.size() && dpuIndices[dpuIdx] == i) {
            info.data.runtimeTrack.deviceId = static_cast<uint16_t>(dpuDeviceTypeBits | (i & 0xFFF));
            info.data.runtimeTrack.kernelName = dpuKernelNames[dpuIdx];
            dpuIdx++;
        }
        if (i * 2 < dataNum) {
            unAgingTraces.emplace_back(info);
        } else {
            agingTraces.emplace_back(info);
        }
    }
    auto fakeGen = std::make_shared<FakeTraceGenerator>(DATA_DIR);
    fakeGen->WriteBin<MsprofCompactInfo>(unAgingTraces, EventType::EVENT_TYPE_TASK_TRACK, false);
    fakeGen->WriteBin<MsprofCompactInfo>(agingTraces, EventType::EVENT_TYPE_TASK_TRACK, true);
}

TEST_F(CompactInfoParserUTest, TestTaskTrackParserShouldCollectDpuKernelNamesWhenDpuDataPresent)
{
    GenCompactInfoData(EventType::EVENT_TYPE_TASK_TRACK, MSPROF_REPORT_NODE_LEVEL, 0, true);
    const uint16_t totalData = 5;
    const std::vector<uint32_t> dpuIndices = {1, 3};
    const std::vector<uint64_t> dpuKernelNames = {0xABCD, 0x1234};
    GenTaskTrackWithDPUData(dpuIndices, dpuKernelNames, totalData);
    auto parser = std::make_shared<TaskTrackParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto compactData = parser->ParseData<MsprofCompactInfo>();
    auto &kernelMap = parser->GetDpuKernelNameMap();
    EXPECT_EQ(compactData.size(), totalData - dpuIndices.size());
    EXPECT_EQ(kernelMap.size(), dpuIndices.size());
    for (size_t idx = 0; idx < dpuIndices.size(); ++idx) {
        uint32_t i = dpuIndices[idx];
        uint16_t devId = static_cast<uint16_t>((static_cast<uint64_t>(1) << 12) | (i & 0xFFF));
        uint64_t expectedKey = (static_cast<uint64_t>(devId) << 32) | i;
        auto it = kernelMap.find(expectedKey);
        EXPECT_NE(it, kernelMap.end());
        EXPECT_EQ(it->second, dpuKernelNames[idx]);
    }
    for (const auto &data : compactData) {
        uint16_t deviceId = data->data.runtimeTrack.deviceId;
        EXPECT_NE(((deviceId >> 12) & 0xF), 1);
    }
}

TEST_F(CompactInfoParserUTest, TestTaskTrackParserDpuKernelNameMapShouldBeEmptyWhenNoDpuData)
{
    const uint16_t totalData = 3;
    const std::vector<uint32_t> dpuIndices = {};
    const std::vector<uint64_t> dpuKernelNames = {};
    GenTaskTrackWithDPUData(dpuIndices, dpuKernelNames, totalData);
    auto parser = std::make_shared<TaskTrackParser>(File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}));
    auto compactData = parser->ParseData<MsprofCompactInfo>();
    auto &kernelMap = parser->GetDpuKernelNameMap();
    EXPECT_EQ(compactData.size(), totalData);
    EXPECT_TRUE(kernelMap.empty());
}
