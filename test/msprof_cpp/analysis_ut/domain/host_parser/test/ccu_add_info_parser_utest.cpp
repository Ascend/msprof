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

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <string>

#include "analysis/csrc/domain/services/parser/host/cann/ccu_add_info_parser.h"
#include "analysis/csrc/domain/services/adapter/parser_struct_adapter.h"
#include "analysis/csrc/infrastructure/utils/file.h"

using namespace Analysis::Domain::Host::Cann;
using namespace Analysis::Utils;

namespace
{
constexpr std::size_t RECORD_SIZE = 256;
const std::string DATA_PATH = "./ccu_add_info_parser_utest";

template <typename T>
void WriteLittleEndian(std::array<uint8_t, RECORD_SIZE>& data, std::size_t offset, T value)
{
    for (std::size_t index = 0; index < sizeof(T); ++index)
    {
        data[offset + index] = static_cast<uint8_t>((value >> (index * 8)) & 0xff);
    }
}

bool WriteRecords(const std::string& fileName,
                  std::initializer_list<std::array<uint8_t, RECORD_SIZE>> records)
{
    std::ofstream output(File::PathJoin({DATA_PATH, fileName}), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!output.is_open())
    {
        return false;
    }
    for (const auto& record : records)
    {
        output.write(reinterpret_cast<const char*>(record.data()), static_cast<std::streamsize>(record.size()));
    }
    return output.good();
}

std::array<uint8_t, RECORD_SIZE> BuildTaskRecord()
{
    std::array<uint8_t, RECORD_SIZE> data{};
    data[24] = 0;
    data[25] = 1;
    WriteLittleEndian<uint64_t>(data, 32, 9581505218827963271ULL);
    WriteLittleEndian<uint64_t>(data, 40, 7415574198778220483ULL);
    WriteLittleEndian<uint32_t>(data, 48, 0);
    WriteLittleEndian<uint32_t>(data, 52, 2);
    WriteLittleEndian<uint16_t>(data, 56, 1);
    WriteLittleEndian<uint32_t>(data, 60, 7);
    data[64] = 1;
    data[65] = 2;
    WriteLittleEndian<uint16_t>(data, 66, 3);
    return data;
}

std::array<uint8_t, RECORD_SIZE> BuildWaitSignalRecord()
{
    std::array<uint8_t, RECORD_SIZE> data{};
    data[24] = 0;
    WriteLittleEndian<uint64_t>(data, 32, 6823696491211891432ULL);
    WriteLittleEndian<uint64_t>(data, 40, 7415574198778220483ULL);
    WriteLittleEndian<uint32_t>(data, 48, 0);
    WriteLittleEndian<uint32_t>(data, 52, 2);
    data[56] = 1;
    WriteLittleEndian<uint16_t>(data, 58, 4);
    WriteLittleEndian<uint32_t>(data, 60, 5);
    data[64] = 1;
    WriteLittleEndian<uint16_t>(data, 66, 39);
    data[68] = 6;
    WriteLittleEndian<uint32_t>(data, 72, 210);
    WriteLittleEndian<uint32_t>(data, 76, 2);
    for (std::size_t index = 0; index < 16; ++index)
    {
        WriteLittleEndian<uint16_t>(data, 80 + index * sizeof(uint16_t), std::numeric_limits<uint16_t>::max());
        WriteLittleEndian<uint32_t>(data, 112 + index * sizeof(uint32_t), std::numeric_limits<uint32_t>::max());
    }
    WriteLittleEndian<uint16_t>(data, 80, 2);
    WriteLittleEndian<uint32_t>(data, 112, 1);
    WriteLittleEndian<uint16_t>(data, 82, 3);
    WriteLittleEndian<uint32_t>(data, 116, 4);
    return data;
}

std::array<uint8_t, RECORD_SIZE> BuildGroupRecord(uint8_t reduceOpType, uint8_t dataType)
{
    std::array<uint8_t, RECORD_SIZE> data{};
    data[24] = 0;
    WriteLittleEndian<uint64_t>(data, 32, 1362511707695072317ULL);
    WriteLittleEndian<uint64_t>(data, 40, 7415574198778220483ULL);
    WriteLittleEndian<uint32_t>(data, 48, 0);
    WriteLittleEndian<uint32_t>(data, 52, 2);
    data[56] = 1;
    WriteLittleEndian<uint16_t>(data, 58, 7);
    WriteLittleEndian<uint32_t>(data, 60, 8);
    data[64] = 1;
    WriteLittleEndian<uint16_t>(data, 66, 119);
    data[68] = 9;
    data[69] = reduceOpType;
    data[70] = dataType;
    data[71] = dataType;
    WriteLittleEndian<uint64_t>(data, 72, 8192);
    for (std::size_t index = 0; index < 16; ++index)
    {
        WriteLittleEndian<uint16_t>(data, 80 + index * sizeof(uint16_t), std::numeric_limits<uint16_t>::max());
        WriteLittleEndian<uint32_t>(data, 112 + index * sizeof(uint32_t), std::numeric_limits<uint32_t>::max());
    }
    WriteLittleEndian<uint16_t>(data, 80, 2);
    WriteLittleEndian<uint32_t>(data, 112, 1);
    return data;
}
}  // namespace

class CcuAddInfoParserUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        GlobalMockObject::verify();
        if (File::Exist(DATA_PATH))
        {
            ASSERT_TRUE(File::RemoveDir(DATA_PATH, 0));
        }
        ASSERT_TRUE(File::CreateDir(DATA_PATH));
    }

    void TearDown() override
    {
        ASSERT_TRUE(File::RemoveDir(DATA_PATH, 0));
    }
};

TEST_F(CcuAddInfoParserUTest, TestAdditionalAdapterShouldPreserveHeaderAndCcuLayouts)
{
    using Analysis::Domain::Adapter::ParserAdditionalInfoAdapter;
    const std::array<std::array<uint8_t, RECORD_SIZE>, 3> raw = {
        BuildTaskRecord(), BuildWaitSignalRecord(), BuildGroupRecord(2, 6)};
    const std::array<AdditionalInfoFormat, 3> formats = {
        AdditionalInfoFormat::CCU_TASK_INFO_TYPE, AdditionalInfoFormat::CCU_WAIT_SIGNAL_INFO_TYPE,
        AdditionalInfoFormat::CCU_GROUP_INFO_TYPE};
    ParserAdditionalInfo parsed{};
    for (size_t i = 0; i < raw.size(); ++i)
    {
        MsprofAdditionalInfo source{};
        std::memcpy(&source, raw[i].data(), raw[i].size());
        source.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        source.level = 7;
        source.type = 123;
        source.threadId = 456;
        source.dataLen = 232;
        source.timeStamp = 987654321;
        ASSERT_TRUE(ParserAdditionalInfoAdapter::AdapterAdditionalInfo(&source, &parsed, formats[i]));
        EXPECT_EQ(source.magicNumber, parsed.magicNumber);
        EXPECT_EQ(source.level, parsed.level);
        EXPECT_EQ(source.type, parsed.type);
        EXPECT_EQ(source.threadId, parsed.threadId);
        EXPECT_EQ(source.dataLen, parsed.dataLen);
        EXPECT_EQ(source.timeStamp, parsed.timeStamp);
        EXPECT_EQ(7415574198778220483ULL, parsed.ccuInfo.groupName);
        EXPECT_EQ(1U, parsed.ccuInfo.workFlowMode);
        if (i == 0)
        {
            EXPECT_EQ(9581505218827963271ULL, parsed.ccuInfo.itemId);
            EXPECT_EQ(1U, parsed.ccuInfo.streamId);
            EXPECT_EQ(2U, parsed.ccuInfo.missionId);
            EXPECT_EQ(3U, parsed.ccuInfo.instrId);
        }
        else if (i == 1)
        {
            EXPECT_EQ(4U, parsed.ccuInfo.streamId);
            EXPECT_EQ(6U, parsed.ccuInfo.missionId);
            EXPECT_EQ(39U, parsed.ccuInfo.instrId);
            EXPECT_EQ(210U, parsed.ccuInfo.ckeId);
            EXPECT_EQ(2U, parsed.ccuInfo.mask);
            EXPECT_EQ(3U, parsed.ccuInfo.channelIds[1]);
            EXPECT_EQ(4U, parsed.ccuInfo.remoteRankIds[1]);
        }
        else
        {
            EXPECT_EQ(7U, parsed.ccuInfo.streamId);
            EXPECT_EQ(9U, parsed.ccuInfo.missionId);
            EXPECT_EQ(119U, parsed.ccuInfo.instrId);
            EXPECT_EQ(8192U, parsed.ccuInfo.dataSize);
            EXPECT_EQ(2U, parsed.ccuInfo.reduceOpType);
            EXPECT_EQ(6U, parsed.ccuInfo.inputDataType);
            EXPECT_EQ(6U, parsed.ccuInfo.outputDataType);
            EXPECT_EQ(0U, parsed.ccuInfo.ckeId);
        }
        if (i != 0)
        {
            EXPECT_EQ(UINT16_MAX, parsed.ccuInfo.channelIds[15]);
            EXPECT_EQ(UINT32_MAX, parsed.ccuInfo.remoteRankIds[15]);
        }
    }
}

TEST_F(CcuAddInfoParserUTest, TestParseShouldMatchPythonFieldContract)
{
    ASSERT_TRUE(WriteRecords("unaging.additional.ccu_task_info.slice_0", {BuildTaskRecord()}));
    ASSERT_TRUE(WriteRecords("unaging.additional.ccu_wait_signal_info.slice_0", {BuildWaitSignalRecord()}));
    ASSERT_TRUE(WriteRecords("unaging.additional.ccu_group_info.slice_0", {BuildGroupRecord(0, 2),
                                                                           BuildGroupRecord(255, 255),
                                                                           BuildGroupRecord(13, 13),
                                                                           BuildGroupRecord(1, 2)}));

    CcuInfoData data;
    CcuAddInfoParser parser(DATA_PATH);
    ASSERT_TRUE(parser.Parse(data));

    ASSERT_EQ(1U, data.taskRecords.size());
    const auto& task = data.taskRecords[0];
    EXPECT_EQ(0U, task.version);
    EXPECT_EQ(1U, task.workFlowMode);
    EXPECT_EQ("9581505218827963271", task.itemId);
    EXPECT_EQ("7415574198778220483", task.groupName);
    EXPECT_EQ(1U, task.streamId);
    EXPECT_EQ(7U, task.taskId);
    EXPECT_EQ(1U, task.dieId);
    EXPECT_EQ(2U, task.missionId);
    EXPECT_EQ(3U, task.instrId);

    ASSERT_EQ(2U, data.waitSignalRecords.size());
    const auto& wait = data.waitSignalRecords[0];
    EXPECT_EQ("6823696491211891432", wait.itemId);
    EXPECT_EQ(4U, wait.streamId);
    EXPECT_EQ(5U, wait.taskId);
    EXPECT_EQ(39U, wait.instrId);
    EXPECT_EQ(6U, wait.missionId);
    EXPECT_EQ(210U, wait.ckeId);
    EXPECT_EQ(2U, wait.mask);
    EXPECT_EQ(2U, wait.channelId);
    EXPECT_EQ(1U, wait.remoteRankId);
    EXPECT_EQ(3U, data.waitSignalRecords[1].channelId);
    EXPECT_EQ(4U, data.waitSignalRecords[1].remoteRankId);

    ASSERT_EQ(4U, data.groupRecords.size());
    EXPECT_EQ("MUL", data.groupRecords[3].reduceOpType);
    const auto& group = data.groupRecords[0];
    EXPECT_EQ(7U, group.streamId);
    EXPECT_EQ(8U, group.taskId);
    EXPECT_EQ(119U, group.instrId);
    EXPECT_EQ(9U, group.missionId);
    EXPECT_EQ("SUM", group.reduceOpType);
    EXPECT_EQ("INT32", group.inputDataType);
    EXPECT_EQ("INT32", group.outputDataType);
    EXPECT_EQ(8192ULL, group.dataSize);
    EXPECT_EQ(2U, group.channelId);
    EXPECT_EQ(1U, group.remoteRankId);
    EXPECT_EQ("RESERVED", data.groupRecords[1].reduceOpType);
    EXPECT_EQ("RESERVED", data.groupRecords[1].inputDataType);
    EXPECT_EQ("13", data.groupRecords[2].reduceOpType);
    EXPECT_EQ("13", data.groupRecords[2].inputDataType);
    EXPECT_EQ("13", data.groupRecords[2].outputDataType);
}

TEST_F(CcuAddInfoParserUTest, TestParseShouldUseCommonAgingSortOrder)
{
    auto unaging = BuildTaskRecord();
    auto aging = BuildTaskRecord();
    WriteLittleEndian<uint32_t>(unaging, 60, 11);
    WriteLittleEndian<uint32_t>(aging, 60, 22);
    ASSERT_TRUE(WriteRecords("unaging.additional.ccu_task_info.slice_1", {unaging}));
    ASSERT_TRUE(WriteRecords("aging.additional.ccu_task_info.slice_0", {aging}));
    CcuInfoData data;
    ASSERT_TRUE(CcuAddInfoParser(DATA_PATH).Parse(data));
    ASSERT_EQ(2U, data.taskRecords.size());
    EXPECT_EQ(11U, data.taskRecords[0].taskId);
    EXPECT_EQ(22U, data.taskRecords[1].taskId);
}

TEST_F(CcuAddInfoParserUTest, TestParseShouldIgnoreMissingFiles)
{
    CcuInfoData data;
    CcuAddInfoParser parser(DATA_PATH);
    EXPECT_TRUE(parser.Parse(data));
    EXPECT_TRUE(data.Empty());
}

TEST_F(CcuAddInfoParserUTest, TestParseShouldConsumePrefixAcrossShortSlices)
{
    const auto record = BuildTaskRecord();
    std::vector<uint8_t> bytes(7, 0xff);
    bytes.insert(bytes.end(), record.begin(), record.end());
    const std::vector<std::string> names = {
        "aging.additional.ccu_task_info.slice_0",
        "aging.additional.ccu_task_info.slice_1",
        "aging.additional.ccu_task_info.slice_2"
    };
    const std::vector<std::size_t> offsets = {0, 2, 5, bytes.size()};
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        std::ofstream output(File::PathJoin({DATA_PATH, names[i]}), std::ios::binary);
        output.write(reinterpret_cast<const char*>(bytes.data() + offsets[i]), offsets[i + 1] - offsets[i]);
        ASSERT_TRUE(output.good());
    }
    CcuInfoData data;
    ASSERT_TRUE(CcuAddInfoParser(DATA_PATH).Parse(data));
    ASSERT_EQ(1U, data.taskRecords.size());
    EXPECT_EQ(7U, data.taskRecords[0].taskId);
    EXPECT_EQ(3U, data.completedFiles.size());
}

TEST_F(CcuAddInfoParserUTest, TestParseShouldSkipCompleteAndZipFiles)
{
    const std::string name = "unaging.additional.ccu_task_info.slice_0";
    ASSERT_TRUE(WriteRecords(name, {BuildTaskRecord()}));
    ASSERT_TRUE(WriteRecords(name + ".complete", {}));
    ASSERT_TRUE(WriteRecords(name + ".zip", {BuildTaskRecord()}));
    ASSERT_TRUE(WriteRecords("unaging.additional.ccu_task_info.slice_invalid", {BuildTaskRecord()}));
    CcuInfoData data;
    ASSERT_TRUE(CcuAddInfoParser(DATA_PATH).Parse(data));
    EXPECT_TRUE(data.Empty());
    EXPECT_TRUE(data.completedFiles.empty());
    EXPECT_TRUE(data.taskSourceCompleted);
    EXPECT_TRUE(data.HasInput());
}

TEST_F(CcuAddInfoParserUTest, TestPartialCompleteShouldFailWithoutPublishingPartialData)
{
    const std::string name = "unaging.additional.ccu_task_info.slice_0";
    ASSERT_TRUE(WriteRecords(name, {BuildTaskRecord()}));
    ASSERT_TRUE(WriteRecords(name + ".complete", {}));
    ASSERT_TRUE(WriteRecords("unaging.additional.ccu_task_info.slice_1", {BuildTaskRecord()}));
    ASSERT_TRUE(WriteRecords("unaging.additional.ccu_wait_signal_info.slice_0", {BuildWaitSignalRecord()}));
    CcuInfoData data;
    EXPECT_FALSE(CcuAddInfoParser(DATA_PATH).Parse(data));
    EXPECT_TRUE(data.Empty());
    EXPECT_TRUE(data.completedFiles.empty());
}

TEST_F(CcuAddInfoParserUTest, TestParseShouldRejectEmptyInputFile)
{
    ASSERT_TRUE(WriteRecords("unaging.additional.ccu_task_info.slice_0", {}));
    CcuInfoData data;
    EXPECT_FALSE(CcuAddInfoParser(DATA_PATH).Parse(data));
    EXPECT_TRUE(data.Empty());
}

TEST_F(CcuAddInfoParserUTest, TestLookupBatchShouldPreserveCompletedSourceMetadata)
{
    const std::string name = "unaging.additional.ccu_task_info.slice_0";
    ASSERT_TRUE(WriteRecords(name, {BuildTaskRecord()}));
    ASSERT_TRUE(WriteRecords(name + ".complete", {}));
    const auto batches = CcuAddInfoParser(DATA_PATH).ParseData<CcuInfoData>();
    ASSERT_EQ(1U, batches.size());
    EXPECT_TRUE(batches[0]->Empty());
    EXPECT_TRUE(batches[0]->taskSourceCompleted);
}

TEST_F(CcuAddInfoParserUTest, TestLookupBatchShouldNotPublishFailedOrMissingInput)
{
    EXPECT_TRUE(CcuAddInfoParser(DATA_PATH).ParseData<CcuInfoData>().empty());
    ASSERT_TRUE(WriteRecords("unaging.additional.ccu_task_info.slice_0", {}));
    EXPECT_TRUE(CcuAddInfoParser(DATA_PATH).ParseData<CcuInfoData>().empty());
}
