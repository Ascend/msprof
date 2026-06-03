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
    auto parser = std::make_shared<MemcpyInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_MEM_CPY, MSPROF_REPORT_NODE_LEVEL, DATA_NUM);
}

TEST_F(CompactInfoParserUTest, TestCompactInfoParserProduceDataShouldReturnEmptyWhenReserveFailed)
{
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).stubs().will(returnValue(false));
    auto parser = std::make_shared<MemcpyInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).reset();
}

TEST_F(CompactInfoParserUTest, TestCompactInfoParserProduceDataShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs()
        .will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<MemcpyInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(CompactInfoParserUTest, TestCompactInfoParserProduceDataShouldReturn9DataWhen1DataIsInvalid)
{
    const uint16_t invalidDataNum = 1;
    GenCompactInfoData(EventType::EVENT_TYPE_MEM_CPY, MSPROF_REPORT_NODE_LEVEL, invalidDataNum);
    auto parser = std::make_shared<MemcpyInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_MEM_CPY, MSPROF_REPORT_NODE_LEVEL, DATA_NUM - invalidDataNum);
}

TEST_F(CompactInfoParserUTest, TestCompactInfoParserProduceDataShouldReturnEmptyWhenReadBinaryFailed)
{
    MOCKER_CPP(&FileReader::ReadBinary)
        .stubs().will(returnValue(ANALYSIS_ERROR));
    auto parser = std::make_shared<MemcpyInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(CompactInfoParserUTest, TestNodeBasicInfoParserProduceDataShouldReturn10DataWhenParseSuccess)
{
    auto parser = std::make_shared<NodeBasicInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
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
    auto parser = std::make_shared<NodeBasicInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).reset();
}

TEST_F(CompactInfoParserUTest, TestNodeBasicInfoParserProduceDataShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs()
        .will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<NodeBasicInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(CompactInfoParserUTest, TestNodeBasicInfoParserProduceDataShouldReturnEmptyWhenReadUnagingBinaryFailed)
{
    MOCKER_CPP(&FileReader::ReadBinary)
        .stubs().will(returnValue(ANALYSIS_OK)).then(returnValue(ANALYSIS_ERROR));
    auto parser = std::make_shared<NodeBasicInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(CompactInfoParserUTest, TestNodeBasicInfoParserProduceDataShouldReturn9DataWhen1DataIsInvalid)
{
    const uint16_t invalidDataNum = 1;
    GenCompactInfoData(EventType::EVENT_TYPE_NODE_BASIC_INFO, MSPROF_REPORT_NODE_LEVEL, invalidDataNum);
    auto parser = std::make_shared<NodeBasicInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_NODE_BASIC_INFO, MSPROF_REPORT_NODE_LEVEL, DATA_NUM - invalidDataNum);
}

TEST_F(CompactInfoParserUTest, TestTaskTrackParserProduceDataShouldReturn8CompactInfoAnd1FlipTaskWhenParseSuccess)
{
    MOCKER_CPP(&Context::IsAllExport).stubs()
        .will(returnValue(true));
    const uint16_t flipTaskNum = 1;
    const uint16_t maintenanceTaskNum = 1;
    auto parser = std::make_shared<TaskTrackParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto compactInfo = parser->ParseData<MsprofCompactInfo>();
    auto flipTask = parser->ParseData<Adapter::FlipTask>();
    Check(compactInfo, EventType::EVENT_TYPE_TASK_TRACK, MSPROF_REPORT_NODE_LEVEL,
          DATA_NUM - flipTaskNum - maintenanceTaskNum);
    EXPECT_EQ(flipTaskNum, flipTask.size());
}

TEST_F(CompactInfoParserUTest, TestTaskTrackParserProduceDataShouldReturnEmptyWhenReserveFailed)
{
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).stubs().will(returnValue(false));
    auto parser = std::make_shared<TaskTrackParser>(File::PathJoin({DATA_DIR, "host", "data"}));
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
    auto parser = std::make_shared<TaskTrackParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto compactInfo = parser->ParseData<MsprofCompactInfo>();
    auto flipTask = parser->ParseData<Adapter::FlipTask>();
    EXPECT_EQ(0, compactInfo.size());
    EXPECT_EQ(0, flipTask.size());
}

TEST_F(CompactInfoParserUTest, TestTaskTrackParserProduceDataShouldReturnEmptyWhenCreateFlipTaskFailed)
{
    MOCKER_CPP(&Adapter::Flip::CreateFlipTask).stubs().will(returnValue(std::shared_ptr<Adapter::FlipTask>{}));
    auto parser = std::make_shared<TaskTrackParser>(File::PathJoin({DATA_DIR, "host", "data"}));
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
    auto parser = std::make_shared<TaskTrackParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_TASK_TRACK, MSPROF_REPORT_NODE_LEVEL,
          DATA_NUM - invalidDataNum);
}

TEST_F(CompactInfoParserUTest, TestNodeAttrInfoParserShouldReturn10DataWhenParseSuccess)
{
    auto parser = std::make_shared<NodeAttrInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_NODE_ATTR_INFO, MSPROF_REPORT_NODE_LEVEL, DATA_NUM);
}

TEST_F(CompactInfoParserUTest, TestNodeAttrInfoParserProduceDataShouldReturnEmptyWhenReserveFailed)
{
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).stubs().will(returnValue(false));
    auto parser = std::make_shared<NodeAttrInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).reset();
}

TEST_F(CompactInfoParserUTest, TestNodeAttrInfoParserProduceDataShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs()
        .will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<NodeAttrInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(CompactInfoParserUTest, TestNodeAttrInfoParserProduceDataShouldReturn9DataWhen1DataIsInvalid)
{
    const uint16_t invalidDataNum = 1;
    GenCompactInfoData(EventType::EVENT_TYPE_NODE_ATTR_INFO, MSPROF_REPORT_NODE_LEVEL, invalidDataNum);
    auto parser = std::make_shared<NodeAttrInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_NODE_ATTR_INFO, MSPROF_REPORT_NODE_LEVEL, DATA_NUM - invalidDataNum);
}

TEST_F(CompactInfoParserUTest, TestHcclOpInfoParserShouldReturn10DataWhenParseSuccess)
{
    auto parser = std::make_shared<HcclOpInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_HCCL_OP_INFO, MSPROF_REPORT_NODE_LEVEL, DATA_NUM);
}

TEST_F(CompactInfoParserUTest, TestHcclOpInfoParserProduceDataShouldReturnEmptyWhenReserveFailed)
{
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).stubs().will(returnValue(false));
    auto parser = std::make_shared<HcclOpInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofCompactInfo>>).reset();
}

TEST_F(CompactInfoParserUTest, TestHcclOpInfoParserProduceDataShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs()
        .will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<HcclOpInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(CompactInfoParserUTest, TestHcclOpInfoParserProduceDataShouldReturn9DataWhen1DataIsInvalid)
{
    const uint16_t invalidDataNum = 1;
    GenCompactInfoData(EventType::EVENT_TYPE_HCCL_OP_INFO, MSPROF_REPORT_NODE_LEVEL, invalidDataNum);
    auto parser = std::make_shared<HcclOpInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofCompactInfo>();
    Check(data, EventType::EVENT_TYPE_HCCL_OP_INFO, MSPROF_REPORT_NODE_LEVEL, DATA_NUM - invalidDataNum);
}
