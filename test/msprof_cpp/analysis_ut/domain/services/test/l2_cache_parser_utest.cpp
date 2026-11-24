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

#include <dirent.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <memory>
#include <mockcpp/mockcpp.hpp>
#include <string>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_l2_cache.h"
#include "analysis/csrc/domain/services/calculator/l2_cache/l2_cache_task_calculator.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/l2_cache/include/l2_cache_parser.h"
#include "analysis/csrc/domain/services/parser/parser.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/l2_cache_parser_item.h"
#include "analysis/csrc/domain/services/persistence/device/l2_cache_persistence.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/process/include/topo_graph.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"
#include "test/msprof_cpp/analysis_ut/stubs/dfx/log_stubs.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const std::string ROOT_DIR = "./l2_cache_parser_utest";
const std::string DATA_DIR = Utils::File::PathJoin({ROOT_DIR, "data"});
const std::string SQLITE_DIR = Utils::File::PathJoin({ROOT_DIR, "sqlite"});
const std::string DB_PATH = Utils::File::PathJoin({SQLITE_DIR, "l2cache.db"});

static_assert(sizeof(HalL2CacheRaw) == L2_CACHE_RECORD_SIZE, "HalL2CacheRaw must be 72 bytes");

template <typename ValueType>
void WriteLittleEndian(std::vector<uint8_t> &buffer, size_t offset, ValueType value)
{
    for (size_t index = 0; index < sizeof(ValueType); ++index)
    {
        buffer[offset + index] = static_cast<uint8_t>((value >> (index * 8U)) & 0xffU);
    }
}

std::vector<uint8_t> CreateL2Record(uint16_t taskType, uint16_t streamId, uint16_t taskId, uint16_t reserve,
                                    const std::vector<uint64_t> &events)
{
    std::vector<uint8_t> record(L2_CACHE_RECORD_SIZE, 0);
    WriteLittleEndian<uint16_t>(record, 0, taskType);
    WriteLittleEndian<uint16_t>(record, 2, streamId);
    WriteLittleEndian<uint16_t>(record, 4, taskId);
    WriteLittleEndian<uint16_t>(record, 6, reserve);
    for (size_t index = 0; index < events.size() && index < L2_CACHE_EVENT_SLOT_COUNT; ++index)
    {
        WriteLittleEndian<uint64_t>(record, 8 + index * sizeof(uint64_t), events[index]);
    }
    return record;
}

bool WriteTwoRecordFile()
{
    std::vector<uint8_t> first = CreateL2Record(1, 2, 3, 0, {10, 20, 30, 40, 50, 60, 70, 80});
    std::vector<uint8_t> second = CreateL2Record(0, 4, 5, 0, {1, 2, 3, 4, 5, 6, 7, 8});
    return WriteBin(first, DATA_DIR, "l2_cache.data.0.slice_0") &&
           WriteBin(second, DATA_DIR, "l2_cache.data.0.slice_0");
}

size_t CountOpenFileDescriptors()
{
    DIR *dir = opendir("/proc/self/fd");
    if (dir == nullptr)
    {
        return 0;
    }
    size_t count = 0;
    while (readdir(dir) != nullptr)
    {
        ++count;
    }
    closedir(dir);
    return count;
}


bool WriteBytes(const std::vector<uint8_t> &bytes, const std::string &name)
{
    auto data = bytes;
    return WriteBin(data, DATA_DIR, name);
}

bool WriteRecordsWithTail(const std::vector<std::vector<uint8_t>> &records, const std::vector<uint8_t> &tail,
                          const std::string &name)
{
    for (const auto &record : records)
    {
        if (!WriteBytes(record, name))
        {
            return false;
        }
    }
    if (!tail.empty() && !WriteBytes(tail, name))
    {
        return false;
    }
    return true;
}

void ExpectRawEq(const HalL2CacheRaw &row, uint16_t taskType, uint16_t streamId, uint16_t taskId, uint16_t reserve,
                 const std::vector<uint64_t> &events)
{
    EXPECT_EQ(taskType, row.taskType);
    EXPECT_EQ(streamId, row.streamId);
    EXPECT_EQ(taskId, row.taskId);
    EXPECT_EQ(reserve, row.reserve);
    ASSERT_EQ(L2_CACHE_EVENT_SLOT_COUNT, events.size());
    for (size_t index = 0; index < L2_CACHE_EVENT_SLOT_COUNT; ++index)
    {
        EXPECT_EQ(events[index], row.events[index]) << "event slot " << index;
    }
}

bool HasLogLevel(const std::vector<TestLogMessage> &logs, const std::string &level)
{
    for (const auto &item : logs)
    {
        if (item.level == level)
        {
            return true;
        }
    }
    return false;
}

bool HasLogMessage(const std::vector<TestLogMessage> &logs, const std::string &level, const std::string &part)
{
    for (const auto &item : logs)
    {
        if (item.level == level && item.message.find(part) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

std::string FormatCapturedLogs(const std::vector<TestLogMessage> &logs)
{
    std::string out;
    for (const auto &item : logs)
    {
        out += item.level;
        out += " ";
        out += item.message;
        out += "\n";
    }
    if (out.empty())
    {
        out = "(no captured logs)\n";
    }
    return out;
}

class DefaultLayoutParser final : public Parser
{
   public:
    const uint8_t *Bytes() const { return binaryData.get(); }
    uint64_t Size() const { return binaryDataSize; }

   private:
    std::vector<std::string> GetFilePattern() override { return {"default_layout.data"}; }
    uint32_t GetTrunkSize() override { return L2_CACHE_RECORD_SIZE; }
    uint32_t ParseData(Infra::DataInventory &, const Infra::Context &) override { return ANALYSIS_OK; }
};

class ZeroTrunkParser final : public Parser
{
   private:
    std::vector<std::string> GetFilePattern() override { return {"default_layout.data"}; }
    uint32_t GetTrunkSize() override { return 0; }
    uint32_t ParseData(Infra::DataInventory &, const Infra::Context &) override { return ANALYSIS_OK; }
};

class OverflowOffsetParser final : public Parser
{
   private:
    std::vector<std::string> GetFilePattern() override { return {"default_layout.data"}; }
    uint32_t GetTrunkSize() override { return L2_CACHE_RECORD_SIZE; }
    uint64_t GetReadOffset(uint64_t fileSize, uint32_t trunkSize) const override
    {
        (void)trunkSize;
        return fileSize + 1U;
    }
    uint32_t ParseData(Infra::DataInventory &, const Infra::Context &) override { return ANALYSIS_OK; }
};

class L2CacheParserUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        ASSERT_TRUE(Utils::File::CreateDir(ROOT_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(DATA_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(SQLITE_DIR));
    }

    void TearDown() override
    {
        GlobalMockObject::reset();
        ASSERT_TRUE(Utils::File::RemoveDir(ROOT_DIR, 0));
    }

    DeviceContext CreateContext(const std::string &events = "0x59,0x5b,0x5c") const
    {
        DeviceContext context;
        context.deviceContextInfo.deviceFilePath = ROOT_DIR;
        context.deviceContextInfo.deviceInfo.chipID = CHIP_V2_1_0;
        context.deviceContextInfo.sampleInfo.l2CacheTaskProfilingEventsStr = events;
        return context;
    }
};
}  // namespace

TEST_F(L2CacheParserUTest, DecodesEightCountsAndKeepsReserveOutOfEventLists)
{
    std::vector<uint8_t> record = CreateL2Record(1, 2, 3, 0xabcd, {10, 20, 30, 40, 50, 60, 70, 80});
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "l2_cache.data.0.slice_0"));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    const auto parsed = inventory.GetPtr<std::vector<HalL2CacheRaw>>();
    ASSERT_NE(nullptr, parsed);
    ASSERT_EQ(1UL, parsed->size());
    EXPECT_EQ(1U, parsed->at(0).taskType);
    EXPECT_EQ(2U, parsed->at(0).streamId);
    EXPECT_EQ(3U, parsed->at(0).taskId);
    EXPECT_EQ(0xabcdU, parsed->at(0).reserve);
    EXPECT_EQ(10UL, parsed->at(0).events[0]);
    EXPECT_EQ(80UL, parsed->at(0).events[7]);
}

TEST_F(L2CacheParserUTest, DirectConstructionStillParsesWithoutProcessRegistration)
{
    ASSERT_TRUE(WriteTwoRecordFile());
    L2CacheParser parser;
    Infra::DataInventory inventory;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    const auto parsed = inventory.GetPtr<std::vector<HalL2CacheRaw>>();
    ASSERT_NE(nullptr, parsed);
    ASSERT_EQ(2UL, parsed->size());
    EXPECT_EQ(1U, parsed->at(0).taskType);
    EXPECT_EQ(3U, parsed->at(0).taskId);
    EXPECT_EQ(0U, parsed->at(1).taskType);
    EXPECT_EQ(5U, parsed->at(1).taskId);
}

TEST_F(L2CacheParserUTest, L2StagesAreNotRegisteredInDeviceParserUTestLinkSet)
{
    // Scope: the device_parser_utest binary only. It links the three L2 TUs and
    // TopoNodeRegistry, but not device_topology_register.cpp, so this cannot prove
    // production initialization. Negative checks below only show L2CacheParser,
    // L2CacheTaskCalculator, and L2CachePersistence are absent here. LlcParser is
    // also null because the production register TU is not linked. Do not treat an
    // empty registry or leftover nodes from other UT cases as a production positive.
    using Application::TopoNodeId;
    using Application::TopoNodeStage;
    EXPECT_EQ(nullptr, Application::TopoNodeRegistry::Find(TopoNodeId{TopoNodeStage::DEVICE_PROCESSING, "L2CacheParser"}));
    EXPECT_EQ(nullptr,
              Application::TopoNodeRegistry::Find(TopoNodeId{TopoNodeStage::DEVICE_PROCESSING, "L2CacheTaskCalculator"}));
    EXPECT_EQ(nullptr,
              Application::TopoNodeRegistry::Find(TopoNodeId{TopoNodeStage::DEVICE_PERSISTENCE, "L2CachePersistence"}));
    EXPECT_EQ(nullptr, Application::TopoNodeRegistry::FindProcessorByRuntimeType(typeid(L2CacheParser)));
    EXPECT_EQ(nullptr, Application::TopoNodeRegistry::FindProcessorByRuntimeType(typeid(L2CacheTaskCalculator)));
    EXPECT_EQ(nullptr, Application::TopoNodeRegistry::FindProcessorByRuntimeType(typeid(L2CachePersistence)));
    EXPECT_EQ(nullptr, Application::TopoNodeRegistry::FindProcessorByName("L2CacheParser"));
    EXPECT_EQ(nullptr, Application::TopoNodeRegistry::FindProcessorByName("L2CacheTaskCalculator"));
    EXPECT_EQ(nullptr, Application::TopoNodeRegistry::FindProcessorByName("L2CachePersistence"));
    const auto &definitions = Application::TopoNodeRegistry::GetDefinitions();
    for (const auto &item : definitions)
    {
        EXPECT_EQ(std::string::npos, item.first.name.find("L2Cache")) << item.first.name;
    }
    EXPECT_EQ(nullptr, Application::TopoNodeRegistry::Find(TopoNodeId{TopoNodeStage::DEVICE_PROCESSING, "LlcParser"}));
}

TEST_F(L2CacheParserUTest, FilePatternCoversL2Prefixes)
{
    std::vector<uint8_t> record = CreateL2Record(2, 3, 4, 0, {1, 2, 3, 4, 5, 6, 7, 8});
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "l2_cache.data.0.host.profiler_default_tag.0.slice_0"));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    const auto parsed = inventory.GetPtr<std::vector<HalL2CacheRaw>>();
    ASSERT_NE(nullptr, parsed);
    ASSERT_EQ(1UL, parsed->size());
    EXPECT_EQ(2U, parsed->at(0).taskType);
    EXPECT_EQ(4U, parsed->at(0).taskId);
}

TEST_F(L2CacheParserUTest, EmptyFileListDoesNotInject)
{
    L2CacheParser parser;
    Infra::DataInventory inventory;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<HalL2CacheRaw>>());
}

TEST_F(L2CacheParserUTest, MissingMatchingFileReturnsError)
{
    std::vector<std::string> files = {Utils::File::PathJoin({DATA_DIR, "l2_cache.data.0.slice_0"})};
    MOCKER_CPP(&Utils::File::GetOriginData).stubs().will(returnValue(files));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    EXPECT_NE(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<HalL2CacheRaw>>());
}

TEST_F(L2CacheParserUTest, OpenOrReadFailureOnExistingPathIsAnError)
{
    ASSERT_TRUE(WriteTwoRecordFile());
    const auto files = Utils::File::GetOriginData(DATA_DIR, {"l2_cache.data"}, {"done", "complete"});
    ASSERT_FALSE(files.empty());
    ASSERT_TRUE(Utils::File::DeleteFile(files.front()));
    MOCKER_CPP(&Utils::File::GetOriginData).stubs().will(returnValue(files));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    EXPECT_NE(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<HalL2CacheRaw>>());
    GlobalMockObject::verify();
}

TEST_F(L2CacheParserUTest, SampleInfoEventsComeFromContextNotDiskJson)
{
    Utils::FileWriter writer(Utils::File::PathJoin({ROOT_DIR, "sample.json"}));
    ASSERT_TRUE(writer.IsOpen());
    writer.WriteText("{\"l2CacheTaskProfilingEvents\":\"0x999\"}");
    writer.Close();
    ASSERT_TRUE(WriteTwoRecordFile());
    L2CacheParser parser;
    Infra::DataInventory inventory;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext("0x59,0x5b,0x5c")));
    ASSERT_NE(nullptr, inventory.GetPtr<std::vector<HalL2CacheRaw>>());
}

TEST_F(L2CacheParserUTest, ParserItemRejectsWrongLength)
{
    std::vector<uint8_t> bad(40, 0);
    HalL2CacheRaw out{};
    EXPECT_EQ(PARSER_ERROR_SIZE_MISMATCH,
              L2CacheParseItem(bad.data(), static_cast<uint32_t>(bad.size()), reinterpret_cast<uint8_t *>(&out), 0));
}

TEST_F(L2CacheParserUTest, FullChainDecodeCalculatePersist)
{
    ASSERT_TRUE(WriteTwoRecordFile());
    Infra::DataInventory inventory;
    const auto context = CreateContext();
    L2CacheParser parser;
    L2CacheTaskCalculator calculator;
    L2CachePersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, context));
    ASSERT_NE(nullptr, inventory.GetPtr<std::vector<HalL2CacheRaw>>());
    ASSERT_EQ(2UL, inventory.GetPtr<std::vector<HalL2CacheRaw>>()->size());
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, context));
    ASSERT_NE(nullptr, inventory.GetPtr<std::vector<L2CacheSummaryRow>>());
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, context));

    Infra::DBRunner runner(DB_PATH);
    std::vector<L2CacheParseDbRow> parseDb;
    ASSERT_TRUE(runner.QueryData("SELECT task_type, stream_id, task_id, event_lists FROM L2CacheParse", parseDb));
    ASSERT_EQ(2UL, parseDb.size());
    EXPECT_EQ("10,20,30,40,50,60,70,80", std::get<3>(parseDb[0]));
    EXPECT_EQ("1,2,3,4,5,6,7,8", std::get<3>(parseDb[1]));
    std::vector<L2CacheSummaryDbRow> summaryDb;
    ASSERT_TRUE(
        runner.QueryData("SELECT task_type, stream_id, task_id, hit_rate, victim_rate FROM L2CacheSummary", summaryDb));
    ASSERT_EQ(2UL, summaryDb.size());
}

TEST_F(L2CacheParserUTest, FopenFailureDoesNotLeakFd)
{
    ASSERT_TRUE(WriteTwoRecordFile());
    const size_t fdCountBefore = CountOpenFileDescriptors();
    // Force fopen path via deleted file after listing.
    const auto files = Utils::File::GetOriginData(DATA_DIR, {"l2_cache.data"}, {"done", "complete"});
    ASSERT_FALSE(files.empty());
    ASSERT_TRUE(Utils::File::DeleteFile(files.front()));
    MOCKER_CPP(&Utils::File::GetOriginData).stubs().will(returnValue(files));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    EXPECT_NE(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    EXPECT_EQ(fdCountBefore, CountOpenFileDescriptors());
    GlobalMockObject::verify();
}

TEST_F(L2CacheParserUTest, FreadShortReadDoesNotLeakFd)
{
    // Real file remains on disk; mock fread short-read to exercise fclose on fread error path.
    ASSERT_TRUE(WriteTwoRecordFile());
    const size_t fdCountBefore = CountOpenFileDescriptors();
    MOCKER_CPP(&fread).stubs().will(returnValue(static_cast<size_t>(1)));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    EXPECT_NE(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<HalL2CacheRaw>>());
    EXPECT_EQ(fdCountBefore, CountOpenFileDescriptors());
    GlobalMockObject::verify();
}

// Original red test: two 72B records plus a 1B tail. On the pre-fix default
// prefix-skip path this misaligned the 4 header fields and 8 counters.
// After L2 GetReadOffset returns 0, the complete prefix must decode fully.
TEST_F(L2CacheParserUTest, TwoRecordsPlusOneTrailingByteKeepsFieldsAligned)
{
    const std::vector<uint64_t> firstEvents = {1, 22, 333, 4444, 55555, 666666, 7777777, 88888888};
    const std::vector<uint64_t> secondEvents = {9, 88, 777, 6666, 55555, 444444, 3333333, 22222222};
    const auto first = CreateL2Record(0x1111, 0x2222, 0x3333, 0x4444, firstEvents);
    const auto second = CreateL2Record(0x1001, 0x2002, 0x3003, 0x4004, secondEvents);
    ASSERT_TRUE(WriteRecordsWithTail({first, second}, {0x7f}, "l2_cache.data.0.slice_0"));

    L2CacheParser parser;
    Infra::DataInventory inventory;
    StartTestLogCapture();
    const uint32_t status = parser.Run(inventory, CreateContext());
    const auto logs = StopTestLogCapture();
    ASSERT_EQ(ANALYSIS_OK, status);
    EXPECT_TRUE(HasLogMessage(logs, "[WARN]", "remaining bytes"));
    EXPECT_FALSE(HasLogLevel(logs, "[ERROR]"));
    const auto parsed = inventory.GetPtr<std::vector<HalL2CacheRaw>>();
    ASSERT_NE(nullptr, parsed);
    ASSERT_EQ(2UL, parsed->size());
    ExpectRawEq(parsed->at(0), 0x1111, 0x2222, 0x3333, 0x4444, firstEvents);
    ExpectRawEq(parsed->at(1), 0x1001, 0x2002, 0x3003, 0x4004, secondEvents);
}

TEST_F(L2CacheParserUTest, TwoRecordsPlus71TrailingBytesParseCompletePrefix)
{
    const auto first = CreateL2Record(1, 2, 3, 4, {10, 20, 30, 40, 50, 60, 70, 80});
    const auto second = CreateL2Record(5, 6, 7, 8, {11, 21, 31, 41, 51, 61, 71, 81});
    std::vector<uint8_t> tail(71, 0xaa);
    ASSERT_TRUE(WriteRecordsWithTail({first, second}, tail, "l2_cache.data.0.slice_0"));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    StartTestLogCapture();
    const uint32_t status = parser.Run(inventory, CreateContext());
    const auto logs = StopTestLogCapture();
    ASSERT_EQ(ANALYSIS_OK, status);
    EXPECT_TRUE(HasLogMessage(logs, "[WARN]", "remaining bytes"));
    EXPECT_FALSE(HasLogLevel(logs, "[ERROR]"));
    const auto parsed = inventory.GetPtr<std::vector<HalL2CacheRaw>>();
    ASSERT_NE(nullptr, parsed);
    ASSERT_EQ(2UL, parsed->size());
    ExpectRawEq(parsed->at(0), 1, 2, 3, 4, {10, 20, 30, 40, 50, 60, 70, 80});
    ExpectRawEq(parsed->at(1), 5, 6, 7, 8, {11, 21, 31, 41, 51, 61, 71, 81});
}

TEST_F(L2CacheParserUTest, Exact72And144ByteFilesSucceedWithoutRemainderWarn)
{
    const auto first = CreateL2Record(1, 2, 3, 0, {1, 2, 3, 4, 5, 6, 7, 8});
    ASSERT_TRUE(WriteBytes(first, "l2_cache.data.0.slice_0"));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    StartTestLogCapture();
    const uint32_t status = parser.Run(inventory, CreateContext());
    const auto logs = StopTestLogCapture();
    ASSERT_EQ(ANALYSIS_OK, status);
    EXPECT_FALSE(HasLogMessage(logs, "[WARN]", "remaining bytes"));
    ASSERT_EQ(1UL, inventory.GetPtr<std::vector<HalL2CacheRaw>>()->size());

    ASSERT_TRUE(WriteBytes(CreateL2Record(9, 8, 7, 0, {8, 7, 6, 5, 4, 3, 2, 1}), "l2_cache.data.0.slice_0"));
    Infra::DataInventory inventory2;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory2, CreateContext()));
    ASSERT_EQ(2UL, inventory2.GetPtr<std::vector<HalL2CacheRaw>>()->size());
}

TEST_F(L2CacheParserUTest, RealEmptyFileSucceedsWithoutInject)
{
    ASSERT_TRUE(WriteBytes({}, "l2_cache.data.0.slice_0"));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    StartTestLogCapture();
    const uint32_t status = parser.Run(inventory, CreateContext());
    const auto logs = StopTestLogCapture();
    ASSERT_EQ(ANALYSIS_OK, status);
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<HalL2CacheRaw>>());
    EXPECT_FALSE(HasLogLevel(logs, "[ERROR]"));
}

TEST_F(L2CacheParserUTest, OnlyOneOr71BytesFailsWithoutInject)
{
    for (size_t length : {1UL, 71UL})
    {
        ASSERT_TRUE(Utils::File::RemoveDir(DATA_DIR, 0));
        ASSERT_TRUE(Utils::File::CreateDir(DATA_DIR));
        ASSERT_TRUE(WriteBytes(std::vector<uint8_t>(length, 0x11), "l2_cache.data.0.slice_0"));
        L2CacheParser parser;
        Infra::DataInventory inventory;
        StartTestLogCapture();
        const uint32_t status = parser.Run(inventory, CreateContext());
        const auto logs = StopTestLogCapture();
        EXPECT_NE(ANALYSIS_OK, status) << "length " << length;
        EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<HalL2CacheRaw>>());
        EXPECT_TRUE(HasLogLevel(logs, "[ERROR]"));
    }
}

TEST_F(L2CacheParserUTest, CrossSliceRecordWithOneByteFirstSlice)
{
    const std::vector<uint64_t> events = {11, 22, 33, 44, 55, 66, 77, 88};
    const auto record = CreateL2Record(1, 2, 3, 4, events);
    ASSERT_TRUE(WriteBytes(std::vector<uint8_t>(record.begin(), record.begin() + 1), "l2_cache.data.0.slice_0"));
    ASSERT_TRUE(WriteBytes(std::vector<uint8_t>(record.begin() + 1, record.end()), "l2_cache.data.0.slice_1"));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    StartTestLogCapture();
    const uint32_t status = parser.Run(inventory, CreateContext());
    const auto logs = StopTestLogCapture();
    ASSERT_EQ(ANALYSIS_OK, status);
    EXPECT_FALSE(HasLogLevel(logs, "[ERROR]"));
    const auto parsed = inventory.GetPtr<std::vector<HalL2CacheRaw>>();
    ASSERT_NE(nullptr, parsed);
    ASSERT_EQ(1UL, parsed->size());
    ExpectRawEq(parsed->at(0), 1, 2, 3, 4, events);
}

TEST_F(L2CacheParserUTest, Slice2AndSlice10UseNumericOrder)
{
    const auto recTwo = CreateL2Record(2, 20, 200, 0, {2, 2, 2, 2, 2, 2, 2, 2});
    const auto recTen = CreateL2Record(10, 10, 10, 0, {10, 10, 10, 10, 10, 10, 10, 10});
    ASSERT_TRUE(WriteBytes(recTen, "l2_cache.data.0.slice_10"));
    ASSERT_TRUE(WriteBytes(recTwo, "l2_cache.data.0.slice_2"));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    const auto parsed = inventory.GetPtr<std::vector<HalL2CacheRaw>>();
    ASSERT_NE(nullptr, parsed);
    ASSERT_EQ(2UL, parsed->size());
    EXPECT_EQ(2U, parsed->at(0).taskType);
    EXPECT_EQ(10U, parsed->at(1).taskType);
}

TEST_F(L2CacheParserUTest, MultiSliceWithAndWithoutTrailingFrame)
{
    const auto recA = CreateL2Record(1, 1, 1, 0, {1, 2, 3, 4, 5, 6, 7, 8});
    const auto recB = CreateL2Record(2, 2, 2, 0, {8, 7, 6, 5, 4, 3, 2, 1});
    ASSERT_TRUE(WriteBytes(recA, "l2_cache.data.0.slice_0"));
    ASSERT_TRUE(WriteBytes(recB, "l2_cache.data.0.slice_1"));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    ASSERT_EQ(2UL, inventory.GetPtr<std::vector<HalL2CacheRaw>>()->size());

    ASSERT_TRUE(WriteBytes({0x3c}, "l2_cache.data.0.slice_1"));
    Infra::DataInventory inventoryTail;
    StartTestLogCapture();
    const uint32_t status = parser.Run(inventoryTail, CreateContext());
    const auto logs = StopTestLogCapture();
    ASSERT_EQ(ANALYSIS_OK, status);
    EXPECT_TRUE(HasLogMessage(logs, "[WARN]", "remaining bytes"));
    ASSERT_EQ(2UL, inventoryTail.GetPtr<std::vector<HalL2CacheRaw>>()->size());
    EXPECT_EQ(1U, inventoryTail.GetPtr<std::vector<HalL2CacheRaw>>()->at(0).taskType);
    EXPECT_EQ(2U, inventoryTail.GetPtr<std::vector<HalL2CacheRaw>>()->at(1).taskType);
}

// First slice is 1 byte; slice_2/slice_10 carry the rest of two records plus a
// 1-byte trailing frame. CrossSlice (no tail) and MultiSlice (whole records)
// do not cover this combination.
TEST_F(L2CacheParserUTest, OneByteFirstSliceThenSlice2And10WithTwoRecordsAndTail)
{
    const std::vector<uint64_t> firstEvents = {101, 202, 303, 404, 505, 606, 707, 808};
    const std::vector<uint64_t> secondEvents = {11, 22, 33, 44, 55, 66, 77, 88};
    const auto first = CreateL2Record(0x111, 0x222, 0x333, 0x444, firstEvents);
    const auto second = CreateL2Record(0x555, 0x666, 0x777, 0x888, secondEvents);
    ASSERT_TRUE(WriteBytes(std::vector<uint8_t>(first.begin(), first.begin() + 1), "l2_cache.data.0.slice_0"));
    ASSERT_TRUE(WriteBytes(std::vector<uint8_t>(first.begin() + 1, first.end()), "l2_cache.data.0.slice_2"));
    std::vector<uint8_t> slice10 = second;
    slice10.push_back(0x5a);
    ASSERT_TRUE(WriteBytes(slice10, "l2_cache.data.0.slice_10"));

    L2CacheParser parser;
    Infra::DataInventory inventory;
    StartTestLogCapture();
    const uint32_t status = parser.Run(inventory, CreateContext());
    const auto logs = StopTestLogCapture();
    ASSERT_EQ(ANALYSIS_OK, status);
    EXPECT_TRUE(HasLogMessage(logs, "[WARN]", "remaining bytes"));
    EXPECT_FALSE(HasLogLevel(logs, "[ERROR]"));
    const auto parsed = inventory.GetPtr<std::vector<HalL2CacheRaw>>();
    ASSERT_NE(nullptr, parsed);
    ASSERT_EQ(2UL, parsed->size());
    ExpectRawEq(parsed->at(0), 0x111, 0x222, 0x333, 0x444, firstEvents);
    ExpectRawEq(parsed->at(1), 0x555, 0x666, 0x777, 0x888, secondEvents);
}

// Controlled GetFileSize mismatch via ftell: sizing sees a smaller capacity than
// the later read length, so ReadData rejects before fread without OOB.
TEST_F(L2CacheParserUTest, ReadDataRejectsWhenLengthExceedsBinaryCapacity)
{
    ASSERT_TRUE(WriteBytes(CreateL2Record(1, 2, 3, 0, {1, 2, 3, 4, 5, 6, 7, 8}), "l2_cache.data.0.slice_0"));
    const size_t fdCountBefore = CountOpenFileDescriptors();
    MOCKER_CPP(&ftell).stubs().will(returnValue(static_cast<long>(L2_CACHE_RECORD_SIZE))).then(
        returnValue(static_cast<long>(L2_CACHE_RECORD_SIZE * 2)));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    StartTestLogCapture();
    const uint32_t status = parser.Run(inventory, CreateContext());
    const auto logs = StopTestLogCapture();
    EXPECT_NE(ANALYSIS_OK, status);
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<HalL2CacheRaw>>());
    EXPECT_TRUE(HasLogMessage(logs, "[ERROR]", "exceeds remaining binary capacity"))
        << "expected inner capacity error; captured logs:\n" << FormatCapturedLogs(logs);
    EXPECT_EQ(fdCountBefore, CountOpenFileDescriptors());
    GlobalMockObject::verify();
}

// Sizing allocates a larger buffer than the later successful read length, so the
// final accumulated-length check fails without injecting parsed rows.
TEST_F(L2CacheParserUTest, ReadDataRejectsWhenAccumulatedLengthMismatchesCapacity)
{
    ASSERT_TRUE(WriteBytes(CreateL2Record(1, 2, 3, 0, {1, 2, 3, 4, 5, 6, 7, 8}), "l2_cache.data.0.slice_0"));
    const size_t fdCountBefore = CountOpenFileDescriptors();
    MOCKER_CPP(&ftell).stubs().will(returnValue(static_cast<long>(L2_CACHE_RECORD_SIZE * 2))).then(
        returnValue(static_cast<long>(L2_CACHE_RECORD_SIZE)));
    L2CacheParser parser;
    Infra::DataInventory inventory;
    StartTestLogCapture();
    const uint32_t status = parser.Run(inventory, CreateContext());
    const auto logs = StopTestLogCapture();
    EXPECT_NE(ANALYSIS_OK, status);
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<HalL2CacheRaw>>());
    EXPECT_TRUE(HasLogMessage(logs, "[ERROR]", "does not match binary data size"))
        << "expected inner size-mismatch error; captured logs:\n" << FormatCapturedLogs(logs);
    EXPECT_EQ(fdCountBefore, CountOpenFileDescriptors());
    GlobalMockObject::verify();
}

TEST_F(L2CacheParserUTest, DefaultLayoutSkipsPrefixRemainderAndMatchesConcatenatedSlices)
{
    const auto recA = CreateL2Record(1, 2, 3, 4, {1, 2, 3, 4, 5, 6, 7, 8});
    const auto recB = CreateL2Record(9, 8, 7, 6, {8, 7, 6, 5, 4, 3, 2, 1});
    std::vector<uint8_t> prefix{0x5a};
    std::vector<uint8_t> expected;
    expected.insert(expected.end(), recA.begin(), recA.end());
    expected.insert(expected.end(), recB.begin(), recB.end());
    ASSERT_TRUE(WriteBytes(prefix, "default_layout.data.0.slice_0"));
    ASSERT_TRUE(WriteBytes(recA, "default_layout.data.0.slice_0"));
    ASSERT_TRUE(WriteBytes(recB, "default_layout.data.0.slice_0"));

    DefaultLayoutParser parser;
    Infra::DataInventory inventory;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    ASSERT_EQ(expected.size(), parser.Size());
    ASSERT_NE(nullptr, parser.Bytes());
    EXPECT_EQ(0, std::memcmp(parser.Bytes(), expected.data(), expected.size()));

    ASSERT_TRUE(Utils::File::RemoveDir(DATA_DIR, 0));
    ASSERT_TRUE(Utils::File::CreateDir(DATA_DIR));
    ASSERT_TRUE(WriteBytes(recA, "default_layout.data.0.slice_0"));
    ASSERT_TRUE(WriteBytes(recB, "default_layout.data.0.slice_1"));
    DefaultLayoutParser multi;
    Infra::DataInventory inventory2;
    ASSERT_EQ(ANALYSIS_OK, multi.Run(inventory2, CreateContext()));
    ASSERT_EQ(expected.size(), multi.Size());
    EXPECT_EQ(0, std::memcmp(multi.Bytes(), expected.data(), expected.size()));
}

TEST_F(L2CacheParserUTest, DefaultLayoutRejectsOffsetPastFirstFile)
{
    // total 71 bytes, trunk 72, default offset = 71, first file is only 1 byte
    ASSERT_TRUE(WriteBytes({0x11}, "default_layout.data.0.slice_0"));
    ASSERT_TRUE(WriteBytes(std::vector<uint8_t>(70, 0x22), "default_layout.data.0.slice_1"));
    DefaultLayoutParser parser;
    Infra::DataInventory inventory;
    StartTestLogCapture();
    const uint32_t status = parser.Run(inventory, CreateContext());
    const auto logs = StopTestLogCapture();
    EXPECT_NE(ANALYSIS_OK, status);
    EXPECT_TRUE(HasLogLevel(logs, "[ERROR]"));
}

TEST_F(L2CacheParserUTest, ZeroTrunkAndOverflowOffsetFailWithoutCrashing)
{
    ASSERT_TRUE(WriteBytes(CreateL2Record(1, 2, 3, 0, {1, 2, 3, 4, 5, 6, 7, 8}), "default_layout.data.0.slice_0"));
    ZeroTrunkParser zeroTrunk;
    Infra::DataInventory inventory;
    StartTestLogCapture();
    const uint32_t zeroStatus = zeroTrunk.Run(inventory, CreateContext());
    const auto zeroLogs = StopTestLogCapture();
    EXPECT_NE(ANALYSIS_OK, zeroStatus);
    EXPECT_TRUE(HasLogLevel(zeroLogs, "[ERROR]"));

    OverflowOffsetParser overflow;
    Infra::DataInventory inventory2;
    StartTestLogCapture();
    const uint32_t overflowStatus = overflow.Run(inventory2, CreateContext());
    const auto overflowLogs = StopTestLogCapture();
    EXPECT_NE(ANALYSIS_OK, overflowStatus);
    EXPECT_TRUE(HasLogLevel(overflowLogs, "[ERROR]"));
}

}  // namespace Domain
}  // namespace Analysis
