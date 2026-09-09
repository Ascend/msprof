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

#include "analysis/csrc/domain/services/parser/ccu_parser.h"
#include "analysis/csrc/domain/services/persistence/device/ccu_persistence.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <limits>
#include <memory>
#include <tuple>
#include <typeindex>
#include <vector>

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_control.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "gtest/gtest.h"

namespace Analysis
{
namespace Domain
{
using namespace Infra;
using namespace Utils;

namespace
{
const std::string TEST_ROOT = "./ccu_parser_utest";
const std::string DEVICE_PATH = File::PathJoin({TEST_ROOT, "device_0"});
const std::string DATA_PATH = File::PathJoin({DEVICE_PATH, "data"});
const std::string SQLITE_PATH = File::PathJoin({DEVICE_PATH, "sqlite"});
const std::string HOST_SQLITE_PATH = File::PathJoin({TEST_ROOT, "host", "sqlite"});
const std::string RUNTIME_DB_PATH = File::PathJoin({HOST_SQLITE_PATH, "runtime.db"});
const std::string CCU_DB_PATH = File::PathJoin({SQLITE_PATH, "ccu.db"});

std::atomic<size_t> nonCcuExecutions{0};
class NonCcuWriter : public Process
{
    uint32_t ProcessEntry(DataInventory&, const Context&) override
    {
        ++nonCcuExecutions;
        return ANALYSIS_OK;
    }
};

template <typename T>
void PutLittleEndian(std::vector<uint8_t>& data, size_t offset, T value)
{
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        data[offset + i] = static_cast<uint8_t>((value >> (i * 8)) & 0xff);
    }
}

std::vector<uint8_t> BuildMissionRecord(uint16_t streamId, uint16_t taskId, uint64_t setckeStart = 0)
{
    std::vector<uint8_t> data(64, 0);
    for (uint16_t i = 0; i < 16; ++i)
    {
        PutLittleEndian<uint16_t>(data, i * sizeof(uint16_t), static_cast<uint16_t>(i + 1));
    }
    PutLittleEndian<uint64_t>(data, 32, setckeStart);
    PutLittleEndian<uint16_t>(data, 40, 31);
    PutLittleEndian<uint64_t>(data, 42, 300);
    PutLittleEndian<uint64_t>(data, 50, 200);
    PutLittleEndian<uint16_t>(data, 58, 21);
    PutLittleEndian<uint16_t>(data, 60, streamId);
    PutLittleEndian<uint16_t>(data, 62, taskId);
    return data;
}

std::vector<uint8_t> BuildChannelRecord()
{
    std::vector<uint8_t> data(2816, 0);
    for (uint32_t i = 0; i < 704; ++i)
    {
        PutLittleEndian<uint32_t>(data, i * sizeof(uint32_t), 1000 + i);
    }
    return data;
}

void WriteBytes(const std::string& path, const std::vector<uint8_t>& data)
{
    std::ofstream output(path, std::ios::out | std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output.is_open());
    output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    ASSERT_TRUE(output.good());
}

void CreateRuntimeDb(uint32_t taskId)
{
    const TableColumns columns = {
        {"task_id", SQL_INTEGER_TYPE},
        {"stream_id", SQL_INTEGER_TYPE},
        {"device_id", SQL_INTEGER_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE}
    };
    const DBRunner runner(RUNTIME_DB_PATH);
    ASSERT_TRUE(runner.CreateTable("HostTask", columns));
    const std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint64_t>> rows = {
        std::make_tuple(taskId, 9, 0, 20),
        std::make_tuple(taskId, 70000, 0, 10)
    };
    ASSERT_TRUE(runner.InsertData("HostTask", rows));
}

void CreateMalformedRuntimeDb()
{
    const TableColumns columns = {
        {"task_id", SQL_INTEGER_TYPE}
    };
    const DBRunner runner(RUNTIME_DB_PATH);
    ASSERT_TRUE(runner.CreateTable("HostTask", columns));
}

template <typename T>
std::unique_ptr<Process> CreateProcess()
{
    return std::unique_ptr<Process>(new T);
}

void RegisterCcuTopologyForTest()
{
    // The device-parser UT target stubs registration macros, so populate the same metadata explicitly.
    static ProcessRegister missionSequence(typeid(CcuMissionParser), CreateProcess<CcuMissionParser>, true,
                                           "CcuMissionParser", {});
    static ProcessRegister missionChip(typeid(CcuMissionParser), {CHIP_V6_1_0});
    static ProcessRegister channelSequence(typeid(CcuChannelParser), CreateProcess<CcuChannelParser>, true,
                                           "CcuChannelParser", {});
    static ProcessRegister channelChip(typeid(CcuChannelParser), {CHIP_V6_1_0});
    static ProcessRegister persistenceSequence(
        typeid(CcuPersistence), CreateProcess<CcuPersistence>, true, "CcuPersistence",
        {typeid(CcuMissionParser), typeid(CcuChannelParser)});
    static ProcessRegister persistenceData(
        typeid(CcuPersistence), std::vector<std::type_index>{typeid(CcuMissionDataSet), typeid(CcuChannelDataSet)});
    static ProcessRegister persistenceChip(typeid(CcuPersistence), {CHIP_V6_1_0});
    (void)missionSequence;
    (void)missionChip;
    (void)channelSequence;
    (void)channelChip;
    (void)persistenceSequence;
    (void)persistenceData;
    (void)persistenceChip;
}

ProcessCollection SelectCcuTopology()
{
    RegisterCcuTopologyForTest();
    ProcessCollection registered = ProcessRegister::CopyProcessInfo();
    ProcessCollection selected;
    const std::vector<std::type_index> processTypes = {
        typeid(CcuMissionParser), typeid(CcuChannelParser), typeid(CcuPersistence)
    };
    for (const auto& processType : processTypes)
    {
        const auto process = registered.find(processType);
        EXPECT_NE(process, registered.end());
        if (process != registered.end())
        {
            selected.emplace(processType, process->second);
        }
    }
    return selected;
}

}  // namespace

class CcuParserUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        if (File::CheckDir(TEST_ROOT))
        {
            ASSERT_TRUE(File::RemoveDir(TEST_ROOT, 0));
        }
        ASSERT_TRUE(File::CreateDir(TEST_ROOT));
        ASSERT_TRUE(File::CreateDir(DEVICE_PATH));
        ASSERT_TRUE(File::CreateDir(DATA_PATH));
        ASSERT_TRUE(File::CreateDir(SQLITE_PATH));
        ASSERT_TRUE(File::CreateDir(File::PathJoin({TEST_ROOT, "host"})));
        ASSERT_TRUE(File::CreateDir(HOST_SQLITE_PATH));
        context_.deviceContextInfo.deviceFilePath = DEVICE_PATH;
        context_.deviceContextInfo.deviceInfo.chipID = CHIP_V6_1_0;
        context_.deviceContextInfo.deviceInfo.deviceId = 0;
    }

    void TearDown() override
    {
        ASSERT_TRUE(File::RemoveDir(TEST_ROOT, 0));
    }

    DeviceContext context_;
};

TEST_F(CcuParserUTest, MissionDecoderKeepsPythonFieldAndSlotOrder)
{
    const auto data = BuildMissionRecord(7, 9, 1000);
    std::vector<CcuMissionRecord> records;

    ASSERT_TRUE(CcuMissionParser::DecodeV6_1(data.data(), data.size(), records));
    ASSERT_EQ(16UL, records.size());
    EXPECT_EQ(7U, records.front().streamId);
    EXPECT_EQ((9U << 16) | 7U, records.front().taskId);
    EXPECT_EQ(21U, records.front().lpInstrId);
    EXPECT_EQ(200U, records.front().lpStartTime);
    EXPECT_EQ(300U, records.front().lpEndTime);
    EXPECT_EQ(31U, records.front().setckeBitInstrId);
    EXPECT_EQ(1000U, records.front().setckeBitStartTime);
    EXPECT_EQ(15U, records.front().relId);
    EXPECT_EQ(1004U, records.front().relEndTime);
    EXPECT_EQ(0U, records.back().relId);
    EXPECT_EQ(1064U, records.back().relEndTime);
}

TEST_F(CcuParserUTest, MissionDecoderCreatesDefaultRowWithoutSetcke)
{
    const auto data = BuildMissionRecord(3, 2);
    std::vector<CcuMissionRecord> records;

    ASSERT_TRUE(CcuMissionParser::DecodeV6_1(data.data(), data.size(), records));
    ASSERT_EQ(1UL, records.size());
    EXPECT_EQ(0U, records.front().relId);
    EXPECT_EQ(0U, records.front().relEndTime);
}

TEST_F(CcuParserUTest, MissionDecoderRejectsRelativeTimeOverflowWithoutPublishingPartialRows)
{
    const auto data = BuildMissionRecord(3, 2, std::numeric_limits<uint64_t>::max());
    std::vector<CcuMissionRecord> records(1);

    EXPECT_FALSE(CcuMissionParser::DecodeV6_1(data.data(), data.size(), records));
    ASSERT_EQ(1UL, records.size());
    EXPECT_EQ(0U, records.front().taskId);
}

TEST_F(CcuParserUTest, ChannelDecoderKeepsV61SpecialOffsets)
{
    const auto data = BuildChannelRecord();
    std::vector<CcuChannelRecord> records;

    ASSERT_TRUE(CcuChannelParser::DecodeV6_1(data.data(), data.size(), records));
    ASSERT_EQ(128UL, records.size());
    EXPECT_EQ((static_cast<uint64_t>(1001) << 32) | 1000, records[0].timestamp);
    EXPECT_EQ(1004U, records[0].maxBw);
    EXPECT_EQ(1003U, records[0].minBw);
    EXPECT_EQ(1002U, records[0].avgBw);
    EXPECT_EQ((static_cast<uint64_t>(1647) << 32) | 1646, records[120].timestamp);
    EXPECT_EQ(1650U, records[120].maxBw);
    EXPECT_EQ((static_cast<uint64_t>(1679) << 32) | 1678, records[124].timestamp);
    EXPECT_EQ(1682U, records[124].maxBw);
}

TEST_F(CcuParserUTest, TopologyKeepsSourceStateHostMappingAndSqliteContract)
{
    const uint32_t taskA = (9U << 16) | 7U;
    auto sourceA = BuildMissionRecord(7, 9);
    sourceA.insert(sourceA.begin(), {0xaa, 0xbb, 0xcc});
    const std::vector<uint8_t> sourceAHead(sourceA.begin(), sourceA.begin() + 20);
    const std::vector<uint8_t> sourceATail(sourceA.begin() + 20, sourceA.end());
    auto sourceB = BuildMissionRecord(3, 2);
    sourceB.insert(sourceB.begin(), 0xdd);

    WriteBytes(File::PathJoin({DATA_PATH, "ccu0.instr.7.slice_0"}), sourceAHead);
    WriteBytes(File::PathJoin({DATA_PATH, "ccu1.instr.8.slice_1"}), sourceB);
    WriteBytes(File::PathJoin({DATA_PATH, "ccu0.instr.7.slice_2"}), sourceATail);
    WriteBytes(File::PathJoin({DATA_PATH, "ccu0.stat.7.slice_0"}), BuildChannelRecord());
    CreateRuntimeDb(taskA);

    ProcessCollection topology = SelectCcuTopology();
    ProcessControlOptions options;
    options.maxWorkerThreads = 2;
    ProcessControl control(topology, options);
    DataInventory inventory;
    ASSERT_TRUE(control.ExecuteProcess(inventory, context_));

    using MissionRows =
        std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint32_t, uint64_t, uint32_t,
                               uint64_t>>;
    MissionRows missionRows;
    DBRunner runner(CCU_DB_PATH);
    ASSERT_TRUE(runner.QueryData("SELECT stream_id, task_id, lp_instr_id, lp_start_time, lp_end_time, "
                                 "setckebit_instr_id, setckebit_start_time, rel_id, rel_end_time "
                                 "FROM OriginMission ORDER BY rowid",
                                 missionRows));
    ASSERT_EQ(2UL, missionRows.size());
    EXPECT_EQ(65535U, std::get<0>(missionRows[0]));
    EXPECT_EQ((2U << 16) | 3U, std::get<1>(missionRows[0]));
    EXPECT_EQ(70000U, std::get<0>(missionRows[1]));
    EXPECT_EQ(taskA, std::get<1>(missionRows[1]));

    std::vector<std::tuple<uint32_t, uint64_t, uint32_t, uint32_t, uint32_t>> channelRows;
    ASSERT_TRUE(runner.QueryData("SELECT channel_id, timestamp, max_bw, min_bw, avg_bw "
                                 "FROM OriginChannel ORDER BY rowid",
                                 channelRows));
    ASSERT_EQ(128UL, channelRows.size());
    EXPECT_EQ(0U, std::get<0>(channelRows.front()));
    EXPECT_EQ(120U, std::get<0>(channelRows[120]));

    EXPECT_TRUE(File::Exist(File::PathJoin({DATA_PATH, "ccu0.instr.7.slice_0.complete"})));
    EXPECT_TRUE(File::Exist(File::PathJoin({DATA_PATH, "ccu1.instr.8.slice_1.complete"})));
    EXPECT_TRUE(File::Exist(File::PathJoin({DATA_PATH, "ccu0.instr.7.slice_2.complete"})));
    EXPECT_TRUE(File::Exist(File::PathJoin({DATA_PATH, "ccu0.stat.7.slice_0.complete"})));
}


TEST_F(CcuParserUTest, TopologyConsumesPrefixAcrossShortSlicesAndRejectsUnsafeRetry)
{
    auto bytes = BuildMissionRecord(7, 9);
    bytes.insert(bytes.begin(), 7, 0xff);
    const std::vector<std::size_t> offsets = {0, 2, 5, bytes.size()};
    for (std::size_t i = 0; i < 3; ++i)
    {
        WriteBytes(File::PathJoin({DATA_PATH, "ccu0.instr.7.slice_" + std::to_string(i)}),
                   std::vector<uint8_t>(bytes.begin() + offsets[i], bytes.begin() + offsets[i + 1]));
    }
    auto topology = SelectCcuTopology();
    ProcessControlOptions options;
    options.maxWorkerThreads = 2;
    ProcessControl control(topology, options);
    DataInventory first;
    ASSERT_TRUE(control.ExecuteProcess(first, context_));
    DBRunner runner(CCU_DB_PATH);
    std::vector<std::tuple<uint32_t>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT task_id FROM OriginMission", rows));
    ASSERT_EQ(1U, rows.size());
    EXPECT_EQ((9U << 16) | 7U, std::get<0>(rows[0]));
    DataInventory completed;
    ASSERT_TRUE(control.ExecuteProcess(completed, context_));
    ASSERT_TRUE(File::DeleteFile(File::PathJoin({DATA_PATH, "ccu0.instr.7.slice_0.complete"})));
    DataInventory partial;
    EXPECT_FALSE(control.ExecuteProcess(partial, context_));
    ASSERT_TRUE(File::DeleteFile(File::PathJoin({DATA_PATH, "ccu0.instr.7.slice_1.complete"})));
    ASSERT_TRUE(File::DeleteFile(File::PathJoin({DATA_PATH, "ccu0.instr.7.slice_2.complete"})));
    DataInventory unmarked;
    EXPECT_FALSE(control.ExecuteProcess(unmarked, context_));
    rows.clear();
    ASSERT_TRUE(runner.QueryData("SELECT task_id FROM OriginMission", rows));
    EXPECT_EQ(1U, rows.size());
}

TEST_F(CcuParserUTest, ZeroRowRetryShouldRejectExistingRowsWithoutCreatingMarkers)
{
    const std::string sourceName = "ccu0.instr.7.slice_0";
    const auto sourcePath = File::PathJoin({DATA_PATH, sourceName});
    WriteBytes(sourcePath, BuildMissionRecord(7, 9));
    auto topology = SelectCcuTopology();
    ProcessControl control(topology);
    DataInventory first;
    ASSERT_TRUE(control.ExecuteProcess(first, context_));
    ASSERT_TRUE(File::DeleteFile(sourcePath + ".complete"));
    WriteBytes(sourcePath, {1});

    DataInventory retry;
    EXPECT_FALSE(control.ExecuteProcess(retry, context_));
    EXPECT_FALSE(File::Exist(sourcePath + ".complete"));
    DBRunner runner(CCU_DB_PATH);
    std::vector<std::tuple<uint64_t>> count;
    ASSERT_TRUE(runner.QueryData("SELECT COUNT(*) FROM OriginMission", count));
    ASSERT_EQ(1U, count.size());
    EXPECT_EQ(1U, std::get<0>(count[0]));
}

TEST_F(CcuParserUTest, CompletedSourceWithoutPersistedTableShouldFail)
{
    const std::string sourceName = "ccu0.instr.7.slice_0";
    WriteBytes(File::PathJoin({DATA_PATH, sourceName}), BuildMissionRecord(7, 9));
    auto topology = SelectCcuTopology();
    ProcessControl control(topology);
    DataInventory first;
    ASSERT_TRUE(control.ExecuteProcess(first, context_));
    ASSERT_TRUE(File::Exist(File::PathJoin({DATA_PATH, sourceName + ".complete"})));
    ASSERT_TRUE(File::DeleteFile(CCU_DB_PATH));

    DataInventory retry;
    EXPECT_FALSE(control.ExecuteProcess(retry, context_));
}

TEST_F(CcuParserUTest, CompletedSourcePreflightShouldRunBeforeOtherTableWrites)
{
    const std::string missionSource = "ccu0.instr.7.slice_0";
    const std::string channelSource = "ccu0.stat.7.slice_0";
    WriteBytes(File::PathJoin({DATA_PATH, missionSource}), BuildMissionRecord(7, 9));
    WriteBytes(File::PathJoin({DATA_PATH, channelSource}), BuildChannelRecord());
    WriteBytes(File::PathJoin({DATA_PATH, channelSource + ".complete"}), {});
    auto topology = SelectCcuTopology();
    ProcessControl control(topology);
    DataInventory inventory;

    EXPECT_FALSE(control.ExecuteProcess(inventory, context_));
    DBRunner runner(CCU_DB_PATH);
    EXPECT_FALSE(runner.CheckTableExists("OriginMission"));
}

TEST_F(CcuParserUTest, CompletedAndPendingSourcesWithoutTableShouldNotLoseCompletedRows)
{
    const std::string completedSource = "ccu0.instr.7.slice_0";
    const std::string pendingSource = "ccu0.instr.8.slice_1";
    WriteBytes(File::PathJoin({DATA_PATH, completedSource}), BuildMissionRecord(7, 9));
    WriteBytes(File::PathJoin({DATA_PATH, completedSource + ".complete"}), {});
    WriteBytes(File::PathJoin({DATA_PATH, pendingSource}), BuildMissionRecord(3, 2));
    auto topology = SelectCcuTopology();
    ProcessControl control(topology);
    DataInventory inventory;

    EXPECT_FALSE(control.ExecuteProcess(inventory, context_));
    EXPECT_FALSE(File::Exist(File::PathJoin({DATA_PATH, pendingSource + ".complete"})));
    DBRunner runner(CCU_DB_PATH);
    EXPECT_FALSE(runner.CheckTableExists("OriginMission"));
}

TEST_F(CcuParserUTest, MalformedHostTaskShouldFailInsteadOfUsingDefaultStream)
{
    const std::string sourceName = "ccu0.instr.7.slice_0";
    WriteBytes(File::PathJoin({DATA_PATH, sourceName}), BuildMissionRecord(7, 9));
    CreateMalformedRuntimeDb();
    auto topology = SelectCcuTopology();
    ProcessControl control(topology);
    DataInventory inventory;

    EXPECT_FALSE(control.ExecuteProcess(inventory, context_));
    EXPECT_FALSE(File::Exist(File::PathJoin({DATA_PATH, sourceName + ".complete"})));
}

TEST_F(CcuParserUTest, SqliteIntegerOverflowShouldFailBeforeCompletionMarker)
{
    const std::string missionSource = "ccu0.instr.6.slice_0";
    const std::string channelSource = "ccu0.stat.7.slice_0";
    WriteBytes(File::PathJoin({DATA_PATH, missionSource}), BuildMissionRecord(7, 9));
    auto channel = BuildChannelRecord();
    PutLittleEndian<uint32_t>(channel, sizeof(uint32_t), std::numeric_limits<uint32_t>::max());
    WriteBytes(File::PathJoin({DATA_PATH, channelSource}), channel);
    auto topology = SelectCcuTopology();
    ProcessControl control(topology);
    DataInventory inventory;

    EXPECT_FALSE(control.ExecuteProcess(inventory, context_));
    DBRunner runner(CCU_DB_PATH);
    EXPECT_FALSE(runner.CheckTableExists("OriginMission"));
    EXPECT_FALSE(runner.CheckTableExists("OriginChannel"));
    EXPECT_FALSE(File::Exist(File::PathJoin({DATA_PATH, missionSource + ".complete"})));
    EXPECT_FALSE(File::Exist(File::PathJoin({DATA_PATH, channelSource + ".complete"})));
}

TEST_F(CcuParserUTest, ArbitraryLengthSourceIdsShouldRemainIndependent)
{
    const std::string sourceA = "ccu0.instr.184467440737095516160.slice_0";
    const std::string sourceB = "ccu0.instr.184467440737095516161.slice_1";
    WriteBytes(File::PathJoin({DATA_PATH, sourceA}), BuildMissionRecord(7, 9));
    WriteBytes(File::PathJoin({DATA_PATH, sourceB}), BuildMissionRecord(3, 2));
    auto topology = SelectCcuTopology();
    ProcessControl control(topology);
    DataInventory inventory;
    ASSERT_TRUE(control.ExecuteProcess(inventory, context_));

    DBRunner runner(CCU_DB_PATH);
    std::vector<std::tuple<uint32_t>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT task_id FROM OriginMission", rows));
    EXPECT_EQ(2UL, rows.size());
}

TEST_F(CcuParserUTest, UnifiedDeviceEntryParsesCcuAlongsideOtherRegisteredProcesses)
{
    RegisterCcuTopologyForTest();
    static ProcessRegister sequence(typeid(NonCcuWriter), CreateProcess<NonCcuWriter>, true, "NonCcuWriter", {});
    static ProcessRegister chip(typeid(NonCcuWriter), {CHIP_ID_ALL});
    nonCcuExecutions = 0;
    const std::vector<std::pair<std::string, std::string>> config = {
        {"info.json.0", R"({"platform_version":"15","devices":"0","DeviceInfo":[{"hwts_frequency":"49",
            "aic_frequency":"1850","aiv_frequency":"1850","ai_core_num":25,"aiv_num":25}],
            "CPU":[{"Frequency":"100"}]})"},
        {"sample.json", R"({"ai_core_profiling":"on","ai_core_metrics":"PipeUtilization",
            "ai_core_profiling_events":"0x416","ai_core_profiling_mode":"task-based",
            "aicore_sampling_interval":10,"aiv_profiling":"on","aiv_metrics":"PipeUtilization",
            "aiv_profiling_events":"0x416","aiv_profiling_mode":"task-based","aiv_sampling_interval":10})"},
        {"start_info", R"({"collectionTimeBegin":"100","clockMonotonicRaw":"100"})"},
        {"host_start.log", "clock_monotonic_raw:100\ncntvct:100\ncntvct_diff:0\n"},
        {"dev_start.log", "clock_monotonic_raw:100\ncntvct:100\n"}
    };
    for (const auto& item : config) {
        std::ofstream output(File::PathJoin({DEVICE_PATH, item.first}));
        ASSERT_TRUE(output.is_open());
        output << item.second;
    }
    WriteBytes(File::PathJoin({DATA_PATH, "ccu0.instr.0.slice_0"}), BuildMissionRecord(1, 2));
    WriteBytes(File::PathJoin({DATA_PATH, "ccu0.stat.0.slice_0"}), BuildChannelRecord());

    ASSERT_TRUE(context_.Init(DEVICE_PATH));
    EXPECT_EQ(1UL, DeviceContextEntry(TEST_ROOT.c_str(), "").size());
    EXPECT_EQ(1UL, nonCcuExecutions.load());
    DBRunner runner(CCU_DB_PATH);
    std::vector<std::tuple<uint32_t>> mission;
    ASSERT_TRUE(runner.QueryData("SELECT task_id FROM OriginMission", mission));
    EXPECT_FALSE(mission.empty());
    std::vector<std::tuple<uint32_t>> channel;
    ASSERT_TRUE(runner.QueryData("SELECT channel_id FROM OriginChannel", channel));
    EXPECT_EQ(128UL, channel.size());
}

TEST_F(CcuParserUTest, RegisteredTopologySupportsOnlyV61)
{
    RegisterCcuTopologyForTest();
    const ProcessCollection registered = ProcessRegister::CopyProcessInfo();
    const std::vector<std::type_index> processTypes = {
        typeid(CcuMissionParser), typeid(CcuChannelParser), typeid(CcuPersistence)
    };
    for (const auto& processType : processTypes)
    {
        const auto process = registered.find(processType);
        ASSERT_NE(process, registered.end());
        ASSERT_EQ(1UL, process->second.chipIds.size());
        EXPECT_EQ(CHIP_V6_1_0, process->second.chipIds.front());
    }

    const auto persistence = registered.find(typeid(CcuPersistence));
    ASSERT_NE(persistence, registered.end());
    ASSERT_EQ(2UL, persistence->second.processDependence.size());
    EXPECT_NE(std::find(persistence->second.processDependence.begin(), persistence->second.processDependence.end(),
                        typeid(CcuMissionParser)),
              persistence->second.processDependence.end());
    EXPECT_NE(std::find(persistence->second.processDependence.begin(), persistence->second.processDependence.end(),
                        typeid(CcuChannelParser)),
              persistence->second.processDependence.end());
}

}  // namespace Domain
}  // namespace Analysis
