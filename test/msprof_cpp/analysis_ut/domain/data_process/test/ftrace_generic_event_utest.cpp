/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <vector>

#include "analysis/csrc/domain/data_process/system/ftrace_generic_event_processor.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/domain/services/parser/ftrace/include/ftrace_binary_parser.h"
#include "gtest/gtest.h"

namespace
{
using namespace Analysis::Domain;

constexpr size_t PAGE_SIZE = 4096;
constexpr size_t FOOTER_SIZE = 64;
constexpr size_t DATA_OFFSET = 16;
constexpr uint32_t KERNEL_TYPE = 42;
constexpr uint32_t PADDING_TYPE = 29;
constexpr uint32_t COMMON_PID = 1234;

void WriteFile(const std::string &path, const void *data, size_t size)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(size));
}

// One type with two usable fields and one field that the meta parser must drop (size 0).
const char *META_JSON = R"({
  "pageSize": 4096,
  "pageDataOffset": 16,
  "ringbuf": {"timeExtendLen": 8, "timeStampLen": 8},
  "sources": [{"system": "sched"}],
  "types": [
    {
      "system": "sched",
      "name": "sched_switch",
      "kernelId": 42,
      "formatHash": "0123456789abcdef",
      "format": "field:unsigned short common_type;",
      "fields": [
        {"name": "common_type", "offset": 0, "size": 2, "cType": "unsigned short"},
        {"name": "common_pid", "offset": 4, "size": 4, "cType": "int"},
        {"name": "broken_size", "offset": 8, "size": 0, "cType": "int"}
      ]
    }
  ]
})";

class FtraceBinaryParserTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        char pattern[] = "/tmp/ftrace_ut_XXXXXX";
        const char *dir = mkdtemp(pattern);
        ASSERT_NE(nullptr, dir);
        dir_ = dir;
        metaPath_ = dir_ + "/trace_event_meta.json";
        WriteFile(metaPath_, META_JSON, strlen(META_JSON));
    }

    void TearDown() override
    {
        if (!dir_.empty())
        {
            EXPECT_TRUE(Analysis::Utils::File::RemoveDir(dir_, 0));
        }
    }

    // Builds one page: header(time_stamp, commit) + data area, committed is bytes used after
    // the page header. Every small event carries an 8 byte payload (type_len = 2).
    static std::vector<uint8_t> MakePage(uint64_t pageTimestamp, std::vector<uint8_t> &data)
    {
        std::vector<uint8_t> page(PAGE_SIZE, 0);
        memcpy(page.data(), &pageTimestamp, sizeof(pageTimestamp));
        const uint64_t committed = data.size();
        memcpy(page.data() + sizeof(pageTimestamp), &committed, sizeof(committed));
        memcpy(page.data() + DATA_OFFSET, data.data(), data.size());
        return page;
    }

    static void AppendSmallEvent(std::vector<uint8_t> &data, uint32_t typeLen, uint32_t timeDelta, uint32_t kernelType,
                                 uint32_t commonPid)
    {
        const uint32_t header = (timeDelta << 5) | typeLen;
        const size_t base = data.size();
        data.resize(base + 4 + typeLen * 4, 0);
        memcpy(data.data() + base, &header, sizeof(header));
        const uint16_t type = static_cast<uint16_t>(kernelType);
        memcpy(data.data() + base + 4, &type, sizeof(type));
        memcpy(data.data() + base + 8, &commonPid, sizeof(commonPid));
    }

    static std::vector<uint8_t> MakeCpuFile(const std::vector<uint8_t> &page, uint64_t footerEvents, bool validFooter)
    {
        std::vector<uint8_t> file(page);
        std::vector<uint8_t> footer(FOOTER_SIZE, 0);
        if (validFooter)
        {
            memcpy(footer.data(), "FTBC0001", 8);
            memcpy(footer.data() + 24, &footerEvents, sizeof(footerEvents));
        }
        file.insert(file.end(), footer.begin(), footer.end());
        return file;
    }

    std::string dir_;
    std::string metaPath_;
};

TEST_F(FtraceBinaryParserTest, ParseMetaReadsTypesAndDropsBadFields)
{
    GenericEventMeta meta;
    size_t pageSize = 0;
    ASSERT_TRUE(FtraceBinaryParser::ParseMeta(metaPath_, meta, pageSize));
    EXPECT_EQ(PAGE_SIZE, pageSize);
    EXPECT_EQ(PAGE_SIZE, meta.pageSize);
    EXPECT_EQ(DATA_OFFSET, meta.pageDataOffset);
    ASSERT_EQ(1U, meta.types.size());
    const auto &def = meta.types.front();
    EXPECT_EQ(1U, def.typeId);
    EXPECT_EQ("sched", def.system);
    EXPECT_EQ("sched_switch", def.name);
    EXPECT_EQ(static_cast<int32_t>(KERNEL_TYPE), def.kernelId);
    // the field with size 0 is a corrupted layout and must not reach the type definition
    ASSERT_EQ(2U, def.fields.size());
    EXPECT_EQ("common_type", def.fields[0].name);
    EXPECT_EQ(0, def.fields[0].offset);
    EXPECT_EQ(2, def.fields[0].size);
    EXPECT_EQ("common_pid", def.fields[1].name);
    EXPECT_EQ(4, def.fields[1].offset);
    EXPECT_EQ(4, def.fields[1].size);
}

TEST_F(FtraceBinaryParserTest, ParseCpuFileDecodesEventAndFooter)
{
    GenericEventMeta meta;
    size_t pageSize = 0;
    ASSERT_TRUE(FtraceBinaryParser::ParseMeta(metaPath_, meta, pageSize));

    const uint64_t pageTimestamp = 1000000;
    const uint32_t timeDelta = 7;
    std::vector<uint8_t> data;
    AppendSmallEvent(data, 2, timeDelta, KERNEL_TYPE, COMMON_PID);
    const auto file = MakeCpuFile(MakePage(pageTimestamp, data), 1, true);
    const std::string cpuPath = dir_ + "/trace.cpu3.bin";
    WriteFile(cpuPath, file.data(), file.size());

    std::vector<GenericEventData> events;
    uint64_t footerEvents = 0;
    EXPECT_TRUE(FtraceBinaryParser::ParseCpuFile(cpuPath, 3, meta, pageSize, events, footerEvents));
    ASSERT_EQ(1U, events.size());
    EXPECT_EQ(3U, events[0].cpuId);
    EXPECT_EQ(1U, events[0].typeId);
    EXPECT_EQ(pageTimestamp + timeDelta, events[0].rawTimestamp);
    ASSERT_EQ(2U, events[0].values.size());
    EXPECT_EQ(KERNEL_TYPE, events[0].values[0].uintVal);
    EXPECT_EQ(static_cast<int64_t>(COMMON_PID), events[0].values[1].intVal);
    EXPECT_EQ(1U, footerEvents);
    (void)std::remove(cpuPath.c_str());
}

TEST_F(FtraceBinaryParserTest, ParseCpuFileSkipsPaddingAndWalksBigEvent)
{
    GenericEventMeta meta;
    size_t pageSize = 0;
    ASSERT_TRUE(FtraceBinaryParser::ParseMeta(metaPath_, meta, pageSize));

    std::vector<uint8_t> data;
    // padding: header + array[0] holding the payload length
    const uint32_t paddingHeader = PADDING_TYPE;
    const uint32_t paddingPayload = 4;
    data.resize(8, 0);
    memcpy(data.data(), &paddingHeader, sizeof(paddingHeader));
    memcpy(data.data() + 4, &paddingPayload, sizeof(paddingPayload));
    // big event (type_len == 0): array[0] is the payload length, payload starts 8 bytes in
    const uint32_t bigHeader = 0;
    const uint32_t bigPayload = 12;  // 8 bytes of payload after the length word
    const size_t bigBase = data.size();
    data.resize(bigBase + 16, 0);
    memcpy(data.data() + bigBase, &bigHeader, sizeof(bigHeader));
    memcpy(data.data() + bigBase + 4, &bigPayload, sizeof(bigPayload));
    const uint16_t type = static_cast<uint16_t>(KERNEL_TYPE);
    memcpy(data.data() + bigBase + 8, &type, sizeof(type));
    const uint32_t pid = COMMON_PID;
    memcpy(data.data() + bigBase + 12, &pid, sizeof(pid));
    // one more small event so a wrong length convention cannot stay hidden
    AppendSmallEvent(data, 2, 5, KERNEL_TYPE, COMMON_PID);

    const auto file = MakeCpuFile(MakePage(2000, data), 2, true);
    const std::string cpuPath = dir_ + "/trace.cpu0.bin";
    WriteFile(cpuPath, file.data(), file.size());

    std::vector<GenericEventData> events;
    uint64_t footerEvents = 0;
    EXPECT_TRUE(FtraceBinaryParser::ParseCpuFile(cpuPath, 0, meta, pageSize, events, footerEvents));
    // padding produces no row, the large event (type_len == 0) and the small one both decode
    ASSERT_EQ(2U, events.size());
    EXPECT_EQ(2000U, events[0].rawTimestamp);  // large event, time_delta 0
    EXPECT_EQ(2005U, events[1].rawTimestamp);  // small event, time_delta 5
    EXPECT_EQ(2U, footerEvents);
    (void)std::remove(cpuPath.c_str());
}

TEST_F(FtraceBinaryParserTest, ParseCpuFileRejectsMissingFooter)
{
    GenericEventMeta meta;
    size_t pageSize = 0;
    ASSERT_TRUE(FtraceBinaryParser::ParseMeta(metaPath_, meta, pageSize));

    std::vector<uint8_t> data;
    AppendSmallEvent(data, 2, 1, KERNEL_TYPE, COMMON_PID);
    const auto file = MakeCpuFile(MakePage(10, data), 0, false);
    const std::string cpuPath = dir_ + "/trace.cpu1.bin";
    WriteFile(cpuPath, file.data(), file.size());

    std::vector<GenericEventData> events;
    uint64_t footerEvents = 99;
    EXPECT_FALSE(FtraceBinaryParser::ParseCpuFile(cpuPath, 1, meta, pageSize, events, footerEvents));
    EXPECT_EQ(0U, footerEvents);
    (void)std::remove(cpuPath.c_str());
}
}  // namespace

TEST_F(FtraceBinaryParserTest, AppliesTimeExtendAndAbsoluteStampWithHighBits)
{
    GenericEventMeta meta;
    size_t pageSize = 0;
    ASSERT_TRUE(FtraceBinaryParser::ParseMeta(metaPath_, meta, pageSize));
    const uint64_t wrap = 1ULL << 59;
    std::vector<uint8_t> data;
    const auto appendTime = [&data](uint32_t type, uint32_t low, uint32_t high) {
        const uint32_t header = (low << 5) | type;
        const auto pos = data.size();
        data.resize(pos + 8);
        memcpy(data.data() + pos, &header, sizeof(header));
        memcpy(data.data() + pos + 4, &high, sizeof(high));
    };
    appendTime(30, 3, 1);
    AppendSmallEvent(data, 2, 7, KERNEL_TYPE, COMMON_PID);
    appendTime(31, 5, 0);
    AppendSmallEvent(data, 2, 11, KERNEL_TYPE, COMMON_PID);
    const auto file = MakeCpuFile(MakePage(wrap + 200, data), 2, true);
    const auto path = dir_ + "/trace.cpu0.bin";
    WriteFile(path, file.data(), file.size());

    std::vector<GenericEventData> events;
    uint64_t footerEvents = 0;
    ASSERT_TRUE(FtraceBinaryParser::ParseCpuFile(path, 0, meta, pageSize, events, footerEvents));
    ASSERT_EQ(2U, events.size());
    EXPECT_EQ(wrap + 200 + (1ULL << 27) + 3 + 7, events[0].rawTimestamp);
    EXPECT_EQ(2 * wrap + 5 + 11, events[1].rawTimestamp);
}

TEST_F(FtraceBinaryParserTest, KeepsDecodedRowsWhenPageOrFooterCountIsIncomplete)
{
    GenericEventMeta meta;
    size_t pageSize = 0;
    ASSERT_TRUE(FtraceBinaryParser::ParseMeta(metaPath_, meta, pageSize));
    std::vector<uint8_t> data;
    AppendSmallEvent(data, 2, 1, KERNEL_TYPE, COMMON_PID);
    const auto path = dir_ + "/trace.cpu0.bin";
    auto file = MakeCpuFile(MakePage(200, data), 2, true);
    WriteFile(path, file.data(), file.size());
    std::vector<GenericEventData> events;
    uint64_t footerEvents = 0;
    EXPECT_FALSE(FtraceBinaryParser::ParseCpuFile(path, 0, meta, pageSize, events, footerEvents));
    ASSERT_EQ(1U, events.size());
    EXPECT_EQ(2U, footerEvents);

    const uint32_t truncatedHeader = 31;
    const auto size = data.size();
    data.resize(size + sizeof(truncatedHeader));
    memcpy(data.data() + size, &truncatedHeader, sizeof(truncatedHeader));
    file = MakeCpuFile(MakePage(200, data), 1, true);
    WriteFile(path, file.data(), file.size());
    events.clear();
    EXPECT_FALSE(FtraceBinaryParser::ParseCpuFile(path, 0, meta, pageSize, events, footerEvents));
    EXPECT_EQ(1U, events.size());
    EXPECT_EQ(1U, footerEvents);
}

TEST_F(FtraceBinaryParserTest, DecodesRelativeAndAbsoluteStringsAndRejectsWrongWidths)
{
    const std::string json = R"({"types":[{"system":"test","name":"loc","kernelId":42,"fields":[
      {"name":"absolute","offset":4,"size":4,"cType":"__data_loc char[]"},
      {"name":"relative","offset":8,"size":4,"cType":"__rel_loc char[]"},
      {"name":"negative","offset":12,"size":8,"cType":"long long"},
      {"name":"wrong","offset":20,"size":16,"cType":"double"}
    ]}]})";
    WriteFile(metaPath_, json.data(), json.size());
    GenericEventMeta meta;
    size_t pageSize = 0;
    ASSERT_TRUE(FtraceBinaryParser::ParseMeta(metaPath_, meta, pageSize));
    std::vector<uint8_t> data;
    AppendSmallEvent(data, 12, 1, KERNEL_TYPE, COMMON_PID);
    constexpr size_t payload = 4;
    const uint32_t absolute = (4U << 16) | 36U;
    const uint32_t relative = (4U << 16) | 28U;  // 8 + 4 + 28 == 40
    const int64_t negative = -42;
    memcpy(data.data() + payload + 4, &absolute, sizeof(absolute));
    memcpy(data.data() + payload + 8, &relative, sizeof(relative));
    memcpy(data.data() + payload + 12, &negative, sizeof(negative));
    memcpy(data.data() + payload + 36, "abc", 4);
    memcpy(data.data() + payload + 40, "def", 4);
    const auto file = MakeCpuFile(MakePage(200, data), 1, true);
    const auto path = dir_ + "/trace.cpu0.bin";
    WriteFile(path, file.data(), file.size());
    std::vector<GenericEventData> events;
    uint64_t footerEvents = 0;
    ASSERT_TRUE(FtraceBinaryParser::ParseCpuFile(path, 0, meta, pageSize, events, footerEvents));
    ASSERT_EQ(1U, events.size());
    ASSERT_EQ(4U, events[0].values.size());
    EXPECT_EQ("abc", events[0].values[0].strVal);
    EXPECT_EQ("def", events[0].values[1].strVal);
    EXPECT_EQ(-42, events[0].values[2].intVal);
    EXPECT_EQ(Analysis::Domain::FTRACE_EVENT_FLAG_FIELD_FAILED, events[0].flags);
}

TEST_F(FtraceBinaryParserTest, ParsesJsonStructureAndRejectsInvalidThreadIds)
{
    const std::string json = R"({"types":[{"system":"x","name":"evt","kernelId":"42",
      "format":"line\n\"system\" {x}", "fields":[
      {"name":"f}","offset":"0","size":2,"details":{"offset":99},"cType":"short"},
      {"name":"bad","offset":"0suffix","size":1,"cType":"char"}]}]})";
    WriteFile(metaPath_, json.data(), json.size());
    GenericEventMeta meta;
    size_t pageSize = 0;
    ASSERT_TRUE(FtraceBinaryParser::ParseMeta(metaPath_, meta, pageSize));
    ASSERT_EQ(1U, meta.types.size());
    EXPECT_EQ("line\n\"system\" {x}", meta.types.front().formatText);
    ASSERT_EQ(1U, meta.types.front().fields.size());
    EXPECT_EQ("f}", meta.types.front().fields[0].name);
    EXPECT_EQ(0, meta.types.front().fields[0].offset);

    const std::string threads = R"({"threads":[{"tid":"0","comm":"a}b","tgid":1},
      {"tid":null,"tgid":99},{"tid":-1,"tgid":99},{"tid":42,"tgid":"18446744073709551616"}]})";
    const auto path = dir_ + "/thread_map.json";
    WriteFile(path, threads.data(), threads.size());
    std::unordered_map<uint32_t, uint32_t> tgidOf;
    ASSERT_TRUE(FtraceBinaryParser::ParseThreadMap(path, tgidOf));
    ASSERT_EQ(1U, tgidOf.size());
    EXPECT_EQ(1U, tgidOf.at(0));
}

TEST_F(FtraceBinaryParserTest, ProcessorUsesHostAnchorSortsAndFlagsRetainedRows)
{
    const auto host = dir_ + "/host";
    const auto device = dir_ + "/device_0";
    const auto ftrace = host + "/data/ftrace";
    ASSERT_EQ(0, mkdir(host.c_str(), 0700));
    ASSERT_EQ(0, mkdir(device.c_str(), 0700));
    ASSERT_EQ(0, mkdir((host + "/data").c_str(), 0700));
    ASSERT_EQ(0, mkdir(ftrace.c_str(), 0700));
    const std::string anchor = R"({"collectionTimeBegin":"1715760307197379","clockMonotonicRaw":"1000"})";
    const std::string wrongAnchor = R"({"collectionTimeBegin":"1715760307197777","clockMonotonicRaw":"2000"})";
    WriteFile(host + "/start_info", anchor.data(), anchor.size());
    WriteFile(device + "/start_info", wrongAnchor.data(), wrongAnchor.size());
    WriteFile(ftrace + "/trace_event_meta.json", META_JSON, strlen(META_JSON));
    const std::string threads = R"({"threads":[{"tid":1234,"tgid":42}]})";
    WriteFile(ftrace + "/thread_map.json", threads.data(), threads.size());

    std::vector<uint8_t> data;
    AppendSmallEvent(data, 2, 1, KERNEL_TYPE, COMMON_PID);
    AppendSmallEvent(data, 2, 20, KERNEL_TYPE, COMMON_PID);
    auto file = MakeCpuFile(MakePage(990, data), 3, true);  // one early event and a footer mismatch
    WriteFile(ftrace + "/trace.cpu1.bin", file.data(), file.size());
    data.clear();
    AppendSmallEvent(data, 2, 1, KERNEL_TYPE, COMMON_PID);
    file = MakeCpuFile(MakePage(1002, data), 1, true);
    WriteFile(ftrace + "/trace.cpu10.bin", file.data(), file.size());
    WriteFile(ftrace + "/trace.cpufoo.bin", file.data(), file.size());

    Analysis::Domain::FtraceGenericEventProcessor processor(dir_);
    Analysis::Infra::DataInventory inventory;
    ASSERT_TRUE(processor.Process(inventory));
    const auto bundles = inventory.GetPtr<std::vector<Analysis::Domain::GenericEventBundle>>();
    ASSERT_NE(nullptr, bundles);
    ASSERT_EQ(1U, bundles->size());
    const auto &events = bundles->front().events;
    ASSERT_EQ(2U, events.size());
    EXPECT_EQ(10U, events[0].cpuId);
    EXPECT_EQ(1715760307197379003ULL, events[0].timestamp);
    EXPECT_EQ((42ULL << 32) | COMMON_PID, events[0].globalTid);
    EXPECT_EQ(0U, events[0].flags);
    EXPECT_EQ(1U, events[1].cpuId);
    EXPECT_EQ(1715760307197379011ULL, events[1].timestamp);
    EXPECT_EQ(Analysis::Domain::FTRACE_EVENT_FLAG_INCOMPLETE, events[1].flags);

    ASSERT_EQ(0, std::remove((host + "/start_info").c_str()));
    Analysis::Infra::DataInventory missingAnchor;
    ASSERT_TRUE(processor.Process(missingAnchor));
    const auto present = missingAnchor.GetPtr<std::vector<Analysis::Domain::GenericEventBundle>>();
    ASSERT_NE(nullptr, present);
    EXPECT_TRUE(present->front().events.empty());
}

TEST_F(FtraceBinaryParserTest, RejectsMultipleFtraceDatasets)
{
    for (const auto *name : {"PROF_first", "PROF_second"})
    {
        const auto dataset = dir_ + "/" + name;
        ASSERT_EQ(0, mkdir(dataset.c_str(), 0700));
        ASSERT_EQ(0, mkdir((dataset + "/data").c_str(), 0700));
        ASSERT_EQ(0, mkdir((dataset + "/data/ftrace").c_str(), 0700));
    }
    std::vector<Analysis::Domain::FtraceCpuFile> files;
    std::string metaPath, threadMap, ftraceDir, anchor;
    EXPECT_FALSE(FtraceBinaryParser::LocateFiles(dir_, files, metaPath, threadMap, ftraceDir, anchor));
    EXPECT_TRUE(files.empty());
}
