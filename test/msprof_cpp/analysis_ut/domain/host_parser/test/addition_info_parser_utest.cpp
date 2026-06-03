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

#include "analysis/csrc/domain/services/parser/host/cann/addition_info_parser.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "test/msprof_cpp/analysis_ut/fake/fake_trace_generator.h"

using namespace Analysis::Domain;
using namespace Analysis::Domain::Host::Cann;
using namespace Analysis::Utils;

const auto DATA_DIR = "./PROF";
const uint16_t DATA_NUM = 30;
const uint16_t TENSOR_DATA_NUM = 18000;
const uint32_t TENSOR_NUM_TO_CONCAT = 3;

class AdditionInfoParserUTest : public testing::Test {
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
        GenAdditionalInfoData(EventType::EVENT_TYPE_CONTEXT_ID, MSPROF_REPORT_NODE_LEVEL);
        GenAdditionalInfoData(EventType::EVENT_TYPE_FUSION_OP_INFO, MSPROF_REPORT_NODE_LEVEL);
        GenAdditionalInfoData(EventType::EVENT_TYPE_GRAPH_ID_MAP, MSPROF_REPORT_NODE_LEVEL);
        GenAdditionalInfoData(EventType::EVENT_TYPE_HCCL_INFO, MSPROF_REPORT_NODE_LEVEL);
        GenTensorData(MSPROF_REPORT_NODE_LEVEL, TENSOR_NUM_TO_CONCAT);
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(DATA_DIR, 0));
    }

    /* GenAdditionalInfoData数据构造：
     1. 生成(aging/unaging)的additional二进制数据文件，包括fusion_op_info，graph_id_map，context_id_info和hccl_info。
     2. 前一半数据数据写入unaging文件，后一半数据写入aging文件
     3. 通过设置invalidDataNum，把最后invalidDataNum个数据改成无效数据，magicNumber设置成MSPROF_DATA_HEAD_MAGIC_NUM + 1
     可以看护的场景：
     1. unaging和aging文件中，fusion_op_info，graph_id_map，context_id_info和hccl_info数据的读取
     2. 设置invalidDataNum，验证无效数据的处理 */
    static void GenAdditionalInfoData(EventType type, uint16_t level, uint16_t invalidDataNum = 0)
    {
        std::vector<MsprofAdditionalInfo> agingTraces;
        std::vector<MsprofAdditionalInfo> unAgingTraces;
        const uint32_t dataLen = 8;
        for (uint32_t i = 0; i < DATA_NUM; ++i) {
            MsprofAdditionalInfo info;
            info.level = level;
            info.type = static_cast<uint32_t>(type);
            info.threadId = i;
            info.dataLen = dataLen;
            info.timeStamp = DATA_NUM + i;
            if (i >= DATA_NUM - invalidDataNum) {
                info.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM + 1;
            }
            if (i * 2 < DATA_NUM) {  // agingTraces和unAgingTraces各生成DATA_NUM / 2个数据
                unAgingTraces.emplace_back(info);
            } else {
                agingTraces.emplace_back(info);
            }
        }
        auto fakeGen = std::make_shared<FakeTraceGenerator>(DATA_DIR);
        fakeGen->WriteBin<MsprofAdditionalInfo>(unAgingTraces, type, false);
        fakeGen->WriteBin<MsprofAdditionalInfo>(agingTraces, type, true);
    }

    /* GenTensorData数据构造：
     1. 生成(aging/unaging)的additional.tensor_info二进制数据文件
     2. 前一半数据数据写入unaging文件，后一半数据写入aging文件
     3. 通过设置invalidDataNum，把最后invalidDataNum个数据改成无效数据，magicNumber设置成MSPROF_DATA_HEAD_MAGIC_NUM + 1
     4. 由于合并多条tensor数据需要相同的threadId，timeStamp和opName，所以对于每个tensor数据，
        threadId，timeStamp和opName设置的值都以i/concatTensorNum递增，
        意味着连续的concatTensorNum个tensor数据合并成一个ConcatTensorInfo数据
     5. tensorNum设置成常数5
     6. 文件切片，分别生成(aging/unaging).additional.tensor_info.slice_0和(aging/unaging).additional.tensor_info.slice_1
     可以看护的场景：
     1. unaging和aging文件中，tensor_info数据的读取
     2. 设置invalidDataNum，验证无效数据的处理
     3. 多条tensor数据合成一个ConcatTensorInfo数据，tensorNum相加
     4. 多条tensor数据合并，单个tensor_info中tensor数量小于5，汇总后tensorNum>=20的情况也可以正常合并，不丢弃剩余tensor信息
     5. 文件切片，其中需要合并的tensor数据会分布在两个文件中(slice_0的尾部和slice_1的头部) */
    static void GenTensorData(uint16_t level, uint32_t concatTensorNum, uint16_t invalidDataNum = 0,
                              uint32_t tensorNum = MSPROF_GE_TENSOR_DATA_NUM)
    {
        const uint32_t dataLen = 8;
        std::vector<MsprofAdditionalInfo> agingTraces;
        std::vector<MsprofAdditionalInfo> unAgingTraces;
        for (uint32_t i = 0; i < TENSOR_DATA_NUM; ++i) {
            MsprofAdditionalInfo info;
            info.level = level;
            info.type = static_cast<uint32_t>(EventType::EVENT_TYPE_TENSOR_INFO);
            info.threadId = i / concatTensorNum;
            info.dataLen = dataLen;
            info.timeStamp = TENSOR_DATA_NUM + i / concatTensorNum;
            if (i >= TENSOR_DATA_NUM - invalidDataNum) {
                info.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM + 1;
            }
            auto tensorInfo = ReinterpretConvert<MsprofTensorInfo*>(info.data);
            tensorInfo->opName = i / concatTensorNum + 1;
            tensorInfo->tensorNum = tensorNum;
            if (i * 2 < TENSOR_DATA_NUM) {  // agingTraces和unAgingTraces各生成TENSOR_DATA_NUM / 2个数据
                unAgingTraces.emplace_back(info);
            } else {
                agingTraces.emplace_back(info);
            }
        }
        auto fakeGen = std::make_shared<FakeTraceGenerator>(DATA_DIR);
        fakeGen->WriteBin<MsprofAdditionalInfo>(unAgingTraces, EventType::EVENT_TYPE_TENSOR_INFO, false);
        fakeGen->WriteBin<MsprofAdditionalInfo>(agingTraces, EventType::EVENT_TYPE_TENSOR_INFO, true);
    }

    static void Check(const std::vector<std::shared_ptr<MsprofAdditionalInfo>> &data,
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

TEST_F(AdditionInfoParserUTest, TestCtxIdParserShouldReturn30DataWhenParseSuccess)
{
    auto parser = std::make_shared<CtxIdParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofAdditionalInfo>();
    Check(data, EventType::EVENT_TYPE_CONTEXT_ID, MSPROF_REPORT_NODE_LEVEL, DATA_NUM);
}

TEST_F(AdditionInfoParserUTest, TestAdditionInfoParserProduceDataShouldReturnEmptyWhenReserveFailed)
{
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofAdditionalInfo>>).stubs().will(returnValue(false));
    auto parser = std::make_shared<CtxIdParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofAdditionalInfo>();
    EXPECT_EQ(0, data.size());
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofAdditionalInfo>>).reset();
}

TEST_F(AdditionInfoParserUTest, TestAdditionInfoParserProduceDataShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs()
        .will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<CtxIdParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofAdditionalInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(AdditionInfoParserUTest, TestAdditionInfoParserProduceDataShouldReturn29DataWhen1DataIsInvalid)
{
    const uint16_t invalidDataNum = 1;
    GenAdditionalInfoData(EventType::EVENT_TYPE_CONTEXT_ID, MSPROF_REPORT_NODE_LEVEL, invalidDataNum);
    auto parser = std::make_shared<CtxIdParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofAdditionalInfo>();
    Check(data, EventType::EVENT_TYPE_CONTEXT_ID, MSPROF_REPORT_NODE_LEVEL, DATA_NUM - invalidDataNum);
}

TEST_F(AdditionInfoParserUTest, TestFusionOpInfoParserShouldReturn30DataWhenParseSuccess)
{
    auto parser = std::make_shared<FusionOpInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofAdditionalInfo>();
    Check(data, EventType::EVENT_TYPE_FUSION_OP_INFO, MSPROF_REPORT_NODE_LEVEL, DATA_NUM);
}

TEST_F(AdditionInfoParserUTest, TestGraphIdParserShouldReturn30DataWhenParseSuccess)
{
    auto parser = std::make_shared<GraphIdParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofAdditionalInfo>();
    Check(data, EventType::EVENT_TYPE_GRAPH_ID_MAP, MSPROF_REPORT_NODE_LEVEL, DATA_NUM);
}

TEST_F(AdditionInfoParserUTest, TestHcclInfoParserShouldReturn30DataWhenParseSuccess)
{
    auto parser = std::make_shared<HcclInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofAdditionalInfo>();
    Check(data, EventType::EVENT_TYPE_HCCL_INFO, MSPROF_REPORT_NODE_LEVEL, DATA_NUM);
}

TEST_F(AdditionInfoParserUTest, TestTensorInfoParserShouldReturn6000ConcatTensorInfoDataWhenParseSuccess)
{
    auto parser = std::make_shared<TensorInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto concatTensorInfo = parser->ParseData<ConcatTensorInfo>();
    const uint32_t dataLen = 8;
    const uint32_t dataNum = TENSOR_DATA_NUM / TENSOR_NUM_TO_CONCAT;
    ASSERT_EQ(dataNum, concatTensorInfo.size());
    for (size_t i = 0; i < dataNum; ++i) {
        EXPECT_EQ(MSPROF_DATA_HEAD_MAGIC_NUM, concatTensorInfo[i]->magicNumber);
        EXPECT_EQ(MSPROF_REPORT_NODE_LEVEL, concatTensorInfo[i]->level);
        EXPECT_EQ(static_cast<uint32_t>(EventType::EVENT_TYPE_TENSOR_INFO), concatTensorInfo[i]->type);
        EXPECT_EQ(dataLen, concatTensorInfo[i]->dataLen);
        EXPECT_EQ(MSPROF_GE_TENSOR_DATA_NUM * TENSOR_NUM_TO_CONCAT, concatTensorInfo[i]->tensorNum);
    }
}

TEST_F(AdditionInfoParserUTest, TestTensorInfoParserProduceDataShouldReturnEmptyWhenReserveFailed)
{
    MOCKER_CPP(&Reserve<std::shared_ptr<ConcatTensorInfo>>).stubs().will(returnValue(false));
    auto parser = std::make_shared<TensorInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<ConcatTensorInfo>();
    EXPECT_EQ(0, data.size());
    MOCKER_CPP(&Reserve<std::shared_ptr<ConcatTensorInfo>>).reset();
}

TEST_F(AdditionInfoParserUTest, TestTensorInfoParserProduceDataShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs()
        .will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<TensorInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<ConcatTensorInfo>();
    EXPECT_EQ(0, data.size());
}

TEST_F(AdditionInfoParserUTest, TestTensorInfoParserProduceDataShouldReturn6000DataWhen1DataIsInvalid)
{
    GenTensorData(MSPROF_REPORT_NODE_LEVEL, TENSOR_NUM_TO_CONCAT, 1);
    auto parser = std::make_shared<TensorInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<ConcatTensorInfo>();
    const uint32_t dataLen = 8;
    const uint32_t dataNum = TENSOR_DATA_NUM / TENSOR_NUM_TO_CONCAT;
    ASSERT_EQ(dataNum, data.size());
    for (size_t i = 0; i < dataNum; ++i) {
        EXPECT_EQ(MSPROF_DATA_HEAD_MAGIC_NUM, data[i]->magicNumber);
        EXPECT_EQ(MSPROF_REPORT_NODE_LEVEL, data[i]->level);
        EXPECT_EQ(static_cast<uint32_t>(EventType::EVENT_TYPE_TENSOR_INFO), data[i]->type);
        EXPECT_EQ(dataLen, data[i]->dataLen);
    }
}

TEST_F(AdditionInfoParserUTest, TestTensorInfoParserProduceDataShouldReturn3600DataWhenAllConcatTensorNumExceed20)
{
    uint32_t concatTensorNum = 8;
    uint32_t geTensorNum = 3;
    GenTensorData(MSPROF_REPORT_NODE_LEVEL, concatTensorNum, 0, geTensorNum);
    auto parser = std::make_shared<TensorInfoParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<ConcatTensorInfo>();
    const uint32_t dataLen = 8;
    const uint16_t dataNum = TENSOR_DATA_NUM / concatTensorNum;
    ASSERT_EQ(dataNum, data.size());
    for (size_t i = 0; i < dataNum; ++i) {
        EXPECT_EQ(MSPROF_DATA_HEAD_MAGIC_NUM, data[i]->magicNumber);
        EXPECT_EQ(MSPROF_REPORT_NODE_LEVEL, data[i]->level);
        EXPECT_EQ(static_cast<uint32_t>(EventType::EVENT_TYPE_TENSOR_INFO), data[i]->type);
        EXPECT_EQ(dataLen, data[i]->dataLen);
        EXPECT_EQ(geTensorNum * concatTensorNum, data[i]->tensorNum);
    }
}
