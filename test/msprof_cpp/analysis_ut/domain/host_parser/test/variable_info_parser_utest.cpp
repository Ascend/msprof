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

#include <cstring>
#include <fstream>

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/domain/services/parser/host/cann/variable_info_parser.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "analysis/csrc/infrastructure/utils/prof_struct.h"
#include "test/msprof_cpp/analysis_ut/fake/fake_trace_generator.h"

using namespace Analysis;
using namespace Analysis::Domain;
using namespace Analysis::Domain::Host::Cann;
using namespace Analysis::Utils;

const auto DATA_DIR = "./PROF_VAR";
const uint16_t DATA_NUM = 10;
const std::string VAR_PREFIX = "variable.capture_op_info.slice_";

#pragma pack(1)
struct FakeVariableOpRecord
{
    uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    uint16_t level = 0;
    uint32_t type = 0;
    uint32_t threadId = 0;
    uint32_t dataLen = 0;
    uint64_t timeStamp = 0;
    MsprofRuntimeOpInfoPayload payload{};
};
#pragma pack()

class VariableInfoParserUTest : public testing::Test
{
protected:
    virtual void SetUp() { GlobalMockObject::verify(); }

    virtual void TearDown() {}

    static void SetUpTestCase()
    {
        static_assert(sizeof(FakeVariableOpRecord) == 24 + sizeof(MsprofRuntimeOpInfoPayload),
                      "variable record size");
        GenAdditionalCaptureOpInfo(false);
        GenAdditionalCaptureOpInfo(true);
        GenVariableCaptureOpInfo();
    }

    static void TearDownTestCase() { EXPECT_TRUE(File::RemoveDir(DATA_DIR, 0)); }

    static void FillPayload(MsprofRuntimeOpInfoPayload &payload, uint32_t i)
    {
        payload.modelId = i + 1;
        payload.deviceId = 0;
        payload.streamId = i;
        payload.taskId = i + 10;
        payload.taskType = 2;  // AI_VECTOR_CORE
        payload.blockNum = 1;
        payload.nodeId = 1000 + i;
        payload.opType = 2000 + i;
        payload.hashId = 0;
        payload.opFlag = 0;
        payload.tensorNum = 0;
    }

    static MsprofAdditionalInfo MakeAdditionalRecord(uint32_t i, bool invalid)
    {
        MsprofAdditionalInfo info{};
        info.level = MSPROF_REPORT_RUNTIME_LEVEL;
        info.type = static_cast<uint32_t>(EventType::EVENT_TYPE_RUNTIME_OP_INFO);
        info.threadId = i;
        info.dataLen = static_cast<uint32_t>(sizeof(MsprofRuntimeOpInfoPayload));
        info.timeStamp = DATA_NUM + i;
        if (invalid)
        {
            info.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM + 1;
        }
        MsprofRuntimeOpInfoPayload payload{};
        FillPayload(payload, i);
        memcpy(info.data, &payload, sizeof(payload));
        return info;
    }

    static FakeVariableOpRecord MakeVariableRecord(uint32_t i, bool invalid, uint32_t dataLen = 0)
    {
        FakeVariableOpRecord rec{};
        rec.level = MSPROF_REPORT_RUNTIME_LEVEL;
        rec.type = static_cast<uint32_t>(EventType::EVENT_TYPE_RUNTIME_OP_INFO);
        rec.threadId = i;
        rec.dataLen = dataLen == 0 ? static_cast<uint32_t>(sizeof(MsprofRuntimeOpInfoPayload)) : dataLen;
        rec.timeStamp = DATA_NUM + i;
        if (invalid)
        {
            rec.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM + 1;
        }
        FillPayload(rec.payload, i);
        return rec;
    }

    /* 生成 additional.capture_op_info：unaging=static，aging=dynamic */
    static void GenAdditionalCaptureOpInfo(bool isAging, uint16_t invalidDataNum = 0)
    {
        std::vector<MsprofAdditionalInfo> traces;
        for (uint32_t i = 0; i < DATA_NUM; ++i)
        {
            traces.emplace_back(MakeAdditionalRecord(i, i >= DATA_NUM - invalidDataNum));
        }
        auto fakeGen = std::make_shared<FakeTraceGenerator>(DATA_DIR);
        fakeGen->WriteBin<MsprofAdditionalInfo>(traces, EventType::EVENT_TYPE_RUNTIME_OP_INFO, isAging);
    }

    static void GenVariableCaptureOpInfo(uint16_t invalidDataNum = 0, uint32_t dataLen = 0)
    {
        std::vector<FakeVariableOpRecord> traces;
        for (uint32_t i = 0; i < DATA_NUM; ++i)
        {
            traces.emplace_back(MakeVariableRecord(i, i >= DATA_NUM - invalidDataNum, dataLen));
        }
        auto fakeGen = std::make_shared<FakeTraceGenerator>(DATA_DIR);
        fakeGen->WriteBin<FakeVariableOpRecord>(traces, EventType::EVENT_TYPE_RUNTIME_OP_INFO, false,
                                                Analysis::Domain::Environment::HOST_ID, VAR_PREFIX);
    }

    static std::string HostDataDir() { return File::PathJoin(std::vector<std::string>{DATA_DIR, "host", "data"}); }
};

TEST_F(VariableInfoParserUTest, TestRuntimeOpInfoParserShouldParseAdditionalAndVariableWhenParseSuccess)
{
    auto parser = std::make_shared<RuntimeOpInfoParser>(HostDataDir());
    EXPECT_EQ(ANALYSIS_OK, parser->Parse());
    auto data = parser->GetOpInfo();
    // unaging additional + aging additional + variable
    ASSERT_EQ(DATA_NUM * 3, data.size());
    for (size_t i = 0; i < DATA_NUM; ++i)
    {
        EXPECT_EQ("runtime", data[i].level);
        EXPECT_EQ(i, data[i].threadId);
        EXPECT_EQ(DATA_NUM + i, data[i].timeStamp);
        EXPECT_EQ(i, data[i].streamId);
        EXPECT_EQ(i + 10, data[i].taskId);
        EXPECT_EQ("AI_VECTOR_CORE", data[i].taskType);
        EXPECT_EQ("0", data[i].isDynamic);
        EXPECT_TRUE(data[i].isValid);
    }
    for (size_t i = DATA_NUM; i < DATA_NUM * 2; ++i)
    {
        EXPECT_EQ("1", data[i].isDynamic);
    }
    for (size_t i = DATA_NUM * 2; i < DATA_NUM * 3; ++i)
    {
        EXPECT_EQ("0", data[i].isDynamic);
    }
}

TEST_F(VariableInfoParserUTest, TestRuntimeOpInfoParserShouldSkipInvalidMagicWhenAdditionalHasInvalidData)
{
    const uint16_t invalidDataNum = 1;
    GenAdditionalCaptureOpInfo(false, invalidDataNum);
    auto parser = std::make_shared<RuntimeOpInfoParser>(HostDataDir());
    EXPECT_EQ(ANALYSIS_OK, parser->Parse());
    auto data = parser->GetOpInfo();
    // unaging 9 + aging 10 + variable 10
    EXPECT_EQ(DATA_NUM * 3 - invalidDataNum, data.size());
    GenAdditionalCaptureOpInfo(false);
}

TEST_F(VariableInfoParserUTest, TestRuntimeOpInfoParserShouldSkipInvalidMagicWhenVariableHasInvalidData)
{
    const uint16_t invalidDataNum = 2;
    GenVariableCaptureOpInfo(invalidDataNum);
    auto parser = std::make_shared<RuntimeOpInfoParser>(HostDataDir());
    EXPECT_EQ(ANALYSIS_OK, parser->Parse());
    auto data = parser->GetOpInfo();
    EXPECT_EQ(DATA_NUM * 3 - invalidDataNum, data.size());
    GenVariableCaptureOpInfo();
}

TEST_F(VariableInfoParserUTest, TestRuntimeOpInfoParserShouldSkipWhenVariableDataLenCannotCoverPayload)
{
    const uint32_t shortDataLen = 10;
    const uint32_t headerSize = 24;
    auto filePath = File::PathJoin(std::vector<std::string>{HostDataDir(), "unaging.variable.capture_op_info.slice_0"});
    std::ofstream outFile(filePath, std::ios::out | std::ios::binary);
    for (uint32_t i = 0; i < DATA_NUM; ++i)
    {
        auto rec = MakeVariableRecord(i, false, shortDataLen);
        outFile.write(reinterpret_cast<const char *>(&rec), headerSize + shortDataLen);
    }
    outFile.close();
    auto parser = std::make_shared<RuntimeOpInfoParser>(HostDataDir());
    EXPECT_EQ(ANALYSIS_OK, parser->Parse());
    auto data = parser->GetOpInfo();
    EXPECT_EQ(DATA_NUM * 2, data.size());
    GenVariableCaptureOpInfo();
}

TEST_F(VariableInfoParserUTest, TestRuntimeOpInfoParserShouldReturnEmptyWhenPopNullptr)
{
    MOCKER_CPP(&ChunkGenerator::Pop).stubs().will(returnValue(static_cast<CHAR_PTR>(nullptr)));
    auto parser = std::make_shared<RuntimeOpInfoParser>(HostDataDir());
    EXPECT_EQ(ANALYSIS_ERROR, parser->Parse());
    EXPECT_EQ(0, parser->GetOpInfo().size());
    MOCKER_CPP(&ChunkGenerator::Pop).reset();
}

TEST_F(VariableInfoParserUTest, TestVariableInfoParserShouldParseVariableRecordsWhenParseSuccess)
{
    auto parser = std::make_shared<VariableInfoParser>(HostDataDir(), "VariableInfoParser");
    parser->Init({"unaging.variable.capture_op_info.slice"});
    EXPECT_EQ(ANALYSIS_OK, parser->Parse());
    auto data = parser->GetData<ParserVariableInfo>();
    ASSERT_EQ(DATA_NUM, data.size());
    for (size_t i = 0; i < DATA_NUM; ++i)
    {
        EXPECT_EQ(MSPROF_DATA_HEAD_MAGIC_NUM, data[i]->magicNumber);
        EXPECT_EQ(MSPROF_REPORT_RUNTIME_LEVEL, data[i]->level);
        EXPECT_EQ(i, data[i]->threadId);
        EXPECT_EQ(sizeof(MsprofRuntimeOpInfoPayload), data[i]->dataLen);
        EXPECT_EQ(sizeof(MsprofRuntimeOpInfoPayload), data[i]->data.size());
    }
}

TEST_F(VariableInfoParserUTest, TestVariableInfoParserShouldReturnEmptyWhenDirHasNoVariableFile)
{
    const std::string emptyDir = "./PROF_VAR_EMPTY";
    File::CreateDir(emptyDir);
    File::CreateDir(File::PathJoin(std::vector<std::string>{emptyDir, "host"}));
    auto dataPath = File::PathJoin(std::vector<std::string>{emptyDir, "host", "data"});
    File::CreateDir(dataPath);
    auto parser = std::make_shared<VariableInfoParser>(dataPath, "VariableInfoParser");
    parser->Init({"unaging.variable.capture_op_info.slice"});
    EXPECT_EQ(ANALYSIS_OK, parser->Parse());
    EXPECT_EQ(0, parser->GetData<ParserVariableInfo>().size());
    EXPECT_TRUE(File::RemoveDir(emptyDir, 0));
}

TEST_F(VariableInfoParserUTest, TestVariableInfoParserShouldReturnErrorWhenProducerNull)
{
    auto parser = std::make_shared<VariableInfoParser>(HostDataDir(), "VariableInfoParser");
    EXPECT_EQ(ANALYSIS_ERROR, parser->Parse());
}
