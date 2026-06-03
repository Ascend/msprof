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
#include "analysis/csrc/domain/services/parser/host/cann/api_event_parser.h"
#include "test/msprof_cpp/analysis_ut/fake/fake_trace_generator.h"

using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Domain::Host::Cann;

const auto DATA_DIR = "./PROF";
const uint16_t API_DATA_NUM = 20;
const uint16_t EVENT_DATA_NUM = 15;

class ApiEventParserUTest : public testing::Test {
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
        GenApiEventData();
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(DATA_DIR, 0));
    }

    /* GenApiEventData数据构造：
     1. 生成(aging/unaging)的api_event二进制数据文件，其中api数据相互独立，而event连续的两条数据可以合并成一条api数据
     2. api数据写入unaging文件，event数据写入aging文件
     3. 通过设置invalidDataNum，把最后invalidDataNum个api数据改成无效数据，magicNumber设置成MSPROF_DATA_HEAD_MAGIC_NUM + 1
     4. 由于合并两条event数据需要相同的level，type，threadId，requestId和itemId，
        所以对于每个event数据，level，type和itemId都设置成固定值，threadId和requestId的值是i/2（每经过两条数据值+1），
        保证当i=2n和2n+1时，level，type，threadId，requestId和itemId值一样。
     5. event的reserve设置成MSPROF_EVENT_FLAG，用于区分是api还是event数据
     可以看护的场景：
     1. unaging和aging文件中，api和event数据的读取
     2. 设置invalidDataNum，验证无效数据的处理
     3. 两条event数据合并成一条api数据
     4. 单独留一条event数据无法合并成api数据 */
    static void GenApiEventData(uint16_t invalidDataNum = 0)
    {
        std::vector<MsprofApi> unAgingTraces;
        std::vector<MsprofEvent> agingTraces;
        const uint16_t level = MSPROF_REPORT_NODE_LEVEL;
        for (uint32_t i = 0; i != API_DATA_NUM; ++i) {
            MsprofApi info;
            info.level = level;
            info.type = static_cast<uint32_t>(EventType::EVENT_TYPE_API);
            info.threadId = i;
            info.beginTime = API_DATA_NUM + EVENT_DATA_NUM + i;
            info.endTime = API_DATA_NUM + EVENT_DATA_NUM + i + 1;
            if (i >= API_DATA_NUM - invalidDataNum) {
                info.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM + 1;
            }
            unAgingTraces.emplace_back(info);
        }
        const uint16_t concatEventNum = 2;  // 2个event数据合成一个api数据
        for (uint32_t i = 0; i != EVENT_DATA_NUM; ++i) {
            MsprofEvent info;
            info.level = level;
            info.type = static_cast<uint32_t>(EventType::EVENT_TYPE_EVENT);
            info.threadId = i / concatEventNum;
            info.requestId = i / concatEventNum;
            info.timeStamp = API_DATA_NUM + EVENT_DATA_NUM + (i + 1) / concatEventNum;
            info.reserve = MSPROF_EVENT_FLAG;
            info.itemId = 0;
            agingTraces.emplace_back(info);
        }
        auto fakeGen = std::make_shared<FakeTraceGenerator>(DATA_DIR);
        fakeGen->WriteBin<MsprofApi>(unAgingTraces, EventType::EVENT_TYPE_API, false);
        fakeGen->WriteBin<MsprofEvent>(agingTraces, EventType::EVENT_TYPE_EVENT, true);
    }

    static void Check(const std::vector<std::shared_ptr<MsprofApi>> &data, uint16_t dataNum)
    {
        ASSERT_EQ(dataNum, data.size());
        const uint16_t level = MSPROF_REPORT_NODE_LEVEL;
        const uint16_t eventDataNum = EVENT_DATA_NUM / 2;  // 2个event数据合成一个api数据
        const uint16_t ApiDataNum = dataNum - eventDataNum;
        for (size_t i = 0; i < ApiDataNum; ++i) {
            EXPECT_EQ(MSPROF_DATA_HEAD_MAGIC_NUM, data[i]->magicNumber);
            EXPECT_EQ(level, data[i]->level);
            EXPECT_EQ(static_cast<uint32_t>(EventType::EVENT_TYPE_API), data[i]->type);
            EXPECT_EQ(i, data[i]->threadId);
            EXPECT_EQ(API_DATA_NUM + EVENT_DATA_NUM + i, data[i]->beginTime);
            EXPECT_EQ(API_DATA_NUM + EVENT_DATA_NUM + i + 1, data[i]->endTime);
        }
        for (size_t i = 0; i < eventDataNum; ++i) {
            EXPECT_EQ(MSPROF_DATA_HEAD_MAGIC_NUM, data[i + ApiDataNum]->magicNumber);
            EXPECT_EQ(level, data[i + ApiDataNum]->level);
            EXPECT_EQ(static_cast<uint32_t>(EventType::EVENT_TYPE_EVENT), data[i + ApiDataNum]->type);
            EXPECT_EQ(i, data[i + ApiDataNum]->threadId);
            EXPECT_EQ(API_DATA_NUM + EVENT_DATA_NUM + i, data[i + ApiDataNum]->beginTime);
            EXPECT_EQ(API_DATA_NUM + EVENT_DATA_NUM + i + 1, data[i + ApiDataNum]->endTime);
        }
    }
};

TEST_F(ApiEventParserUTest, TestProduceDataShouldReturn27ApiDataAnd0EventDataWhenParseSuccess)
{
    auto parser = std::make_shared<ApiEventParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto apiData = parser->ParseData<MsprofApi>();
    auto eventData = parser->ParseData<MsprofEvent>();
    const uint16_t apiDataNum = API_DATA_NUM + EVENT_DATA_NUM / 2;  // 2个event数据合成一个api数据
    Check(apiData, apiDataNum);
    EXPECT_EQ(0, eventData.size());
}

TEST_F(ApiEventParserUTest, TestProduceDataShouldReturnEmptyWhenReserveFailed)
{
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofApi>>).stubs().will(returnValue(false));
    auto parser = std::make_shared<ApiEventParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofApi>();
    EXPECT_EQ(0, data.size());
    MOCKER_CPP(&Reserve<std::shared_ptr<MsprofApi>>).reset();
}

TEST_F(ApiEventParserUTest, TestProduceDataShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs()
        .will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<ApiEventParser>(File::PathJoin({DATA_DIR, "host", "data"}));
    auto data = parser->ParseData<MsprofApi>();
    EXPECT_EQ(0, data.size());
}
