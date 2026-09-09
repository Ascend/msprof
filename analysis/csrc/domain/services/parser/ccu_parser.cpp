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

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <regex>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/binary_utils.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Infra;
using namespace Utils;

namespace
{
constexpr size_t CCU_MISSION_RECORD_SIZE = 64;
constexpr size_t CCU_CHANNEL_RECORD_SIZE = 2816;

constexpr uint32_t CCU_V6_1_CHANNEL_COUNT = 128;
constexpr uint64_t CCU_V6_1_RELATIVE_TIME_SCALE = 4;
constexpr uint32_t DEFAULT_STREAM_ID = 65535;
constexpr size_t READ_BUFFER_SIZE = 1024 * 1024;

enum class CcuDataKind
{
    MISSION,
    CHANNEL
};

struct SourceKey
{
    uint32_t dieId = 0;
    std::string sourceId;

    bool operator<(const SourceKey& other) const
    {
        return std::tie(dieId, sourceId) < std::tie(other.dieId, other.sourceId);
    }
};

struct CcuSourceFile
{
    std::string name;
    std::string path;
    SourceKey source;
    uint64_t size = 0;
    uint32_t sliceOrder = 0;
    bool shouldProcess = false;
};

struct CcuSourceState
{
    uint64_t totalSize = 0;
    uint64_t leadingOffset = 0;
    bool hasPending = false;
    bool hasCompleted = false;
    std::vector<uint8_t> tail;
};

std::string NormalizeNumber(const std::string& value)
{
    const auto first = value.find_first_not_of('0');
    return first == std::string::npos ? "0" : value.substr(first);
}

const std::regex& GetFilePattern(CcuDataKind dataKind)
{
    // Keep these prefixes aligned with the existing Python CCU file patterns.
    static const std::regex missionPattern(R"(^ccu([01])\.instr\.([0-9]+)\.slice_([0-9]+))");
    static const std::regex channelPattern(R"(^ccu([01])\.stat\.([0-9]+)\.slice_([0-9]+))");
    return dataKind == CcuDataKind::MISSION ? missionPattern : channelPattern;
}

bool DiscoverFiles(const std::string& dataPath, CcuDataKind dataKind, std::vector<CcuSourceFile>& files)
{
    if (!File::CheckDir(dataPath))
    {
        ERROR("Invalid CCU data directory: %", dataPath);
        return false;
    }
    const std::vector<std::string> prefixes = dataKind == CcuDataKind::MISSION
                                                  ? std::vector<std::string>{"ccu0.instr.", "ccu1.instr."}
                                                  : std::vector<std::string>{"ccu0.stat.", "ccu1.stat."};
    const auto paths = File::GetOriginData(dataPath, prefixes, {".complete", ".done", ".zip"});
    const auto& pattern = GetFilePattern(dataKind);
    for (const auto& path : paths)
    {
        const auto name = File::BaseName(path);
        std::smatch match;
        if (!std::regex_search(name, match, pattern))
        {
            continue;
        }
        CcuSourceFile file;
        file.name = name;
        file.path = path;
        file.source.dieId = static_cast<uint32_t>(match[1].str().front() - '0');
        file.source.sourceId = NormalizeNumber(match[2].str());
        const auto suffix = Split(name, "_").back();
        if (StrToU32(file.sliceOrder, suffix) != ANALYSIS_OK)
        {
            ERROR("Failed to parse CCU slice number: %", name);
            file.sliceOrder = 0;
        }
        if (!FileReader::Check(path))
        {
            ERROR("Invalid CCU source file: %", name);
            return false;
        }
        file.size = File::Size(path);
        file.shouldProcess = !File::Exist(path + ".complete");
        files.emplace_back(std::move(file));
    }
    std::sort(files.begin(), files.end(),
              [](const CcuSourceFile& left, const CcuSourceFile& right) { return left.sliceOrder < right.sliceOrder; });
    INFO("Found % CCU device files", files.size());
    return true;
}

bool InitSourceStates(const std::vector<CcuSourceFile>& files, size_t recordSize,
                      std::map<SourceKey, CcuSourceState>& states)
{
    for (const auto& file : files)
    {
        auto& state = states[file.source];
        if (state.totalSize > std::numeric_limits<uint64_t>::max() - file.size)
        {
            ERROR("CCU source size overflow: %", file.name);
            return false;
        }
        state.totalSize += file.size;
        state.hasPending |= file.shouldProcess;
        state.hasCompleted |= !file.shouldProcess;
    }
    for (auto& item : states)
    {
        if (item.second.hasPending && item.second.hasCompleted)
        {
            ERROR("Partially completed CCU source; clear parsed output and reimport the full capture");
            return false;
        }
        item.second.leadingOffset = item.second.totalSize % recordSize;
        item.second.tail.reserve(recordSize - 1);
    }
    return true;
}

template <typename DecodeFunc>
bool DecodeFile(const CcuSourceFile& file, size_t recordSize, CcuSourceState& state, DecodeFunc decode)
{
    if (File::Size(file.path) != file.size)
    {
        ERROR("CCU file size changed while parsing: %", file.name);
        return false;
    }
    std::stringstream input;
    FileReader reader(file.path, std::ios::in | std::ios::binary);
    if (reader.ReadBinary(input) != ANALYSIS_OK || input.tellp() != static_cast<std::streamoff>(file.size))
    {
        ERROR("Read CCU source file failed or size changed: %", file.name);
        return false;
    }

    uint64_t fileOffset = 0;
    if (state.leadingOffset != 0)
    {
        fileOffset = std::min(state.leadingOffset, file.size);
        state.leadingOffset -= fileOffset;
        input.seekg(static_cast<std::streamoff>(fileOffset), std::ios::beg);
        if (input.fail())
        {
            ERROR("Seek CCU source file failed: %", file.name);
            return false;
        }
    }

    std::array<uint8_t, READ_BUFFER_SIZE> readBuffer{};
    uint64_t remaining = file.size - fileOffset;
    while (remaining > 0)
    {
        const size_t readSize = static_cast<size_t>(std::min<uint64_t>(remaining, readBuffer.size()));
        input.read(reinterpret_cast<char*>(readBuffer.data()), static_cast<std::streamsize>(readSize));
        if (static_cast<size_t>(input.gcount()) != readSize)
        {
            ERROR("Read CCU source file failed: %", file.name);
            return false;
        }
        remaining -= readSize;

        std::vector<uint8_t> completeData;
        completeData.reserve(state.tail.size() + readSize);
        completeData.insert(completeData.end(), state.tail.begin(), state.tail.end());
        completeData.insert(completeData.end(), readBuffer.begin(), readBuffer.begin() + readSize);
        const size_t completeSize = completeData.size() / recordSize * recordSize;
        for (size_t offset = 0; offset < completeSize; offset += recordSize)
        {
            if (!decode(completeData.data() + offset, recordSize))
            {
                ERROR("Decode CCU record failed in file: %", file.name);
                return false;
            }
        }
        state.tail.assign(completeData.begin() + completeSize, completeData.end());
    }
    return File::Size(file.path) == file.size;
}

template <typename DecodeFunc>
bool ParseSourceFiles(const std::string& devicePath, CcuDataKind dataKind, size_t recordSize, DecodeFunc decode,
                      std::vector<std::string>& completedFiles, bool& hasCompletedFiles)
{
    const std::string dataPath = File::PathJoin({devicePath, "data"});
    std::vector<CcuSourceFile> files;
    if (!DiscoverFiles(dataPath, dataKind, files))
    {
        return false;
    }
    std::map<SourceKey, CcuSourceState> states;
    hasCompletedFiles =
        std::any_of(files.begin(), files.end(), [](const CcuSourceFile& file) { return !file.shouldProcess; });
    if (!InitSourceStates(files, recordSize, states))
    {
        return false;
    }

    for (const auto& file : files)
    {
        if (!file.shouldProcess)
        {
            continue;
        }
        if (file.size == 0)
        {
            ERROR("CCU source file is empty: %", file.name);
            return false;
        }
        if (!DecodeFile(file, recordSize, states[file.source], decode))
        {
            return false;
        }
        completedFiles.emplace_back(file.name);
    }
    for (const auto& item : states)
    {
        if (!item.second.tail.empty())
        {
            ERROR("CCU source has % trailing bytes after parsing, die: %, source: %", item.second.tail.size(),
                  item.first.dieId, item.first.sourceId);
            return false;
        }
    }
    return true;
}

bool ResolveMissionStreamIds(const DeviceContext& context, std::vector<CcuMissionRecord>& records)
{
    for (auto& record : records)
    {
        record.streamId = DEFAULT_STREAM_ID;
    }
    if (records.empty())
    {
        return true;
    }

    DeviceInfo deviceInfo{};
    context.Getter(deviceInfo);
    const std::string profPath = File::PathJoin({context.GetDeviceFilePath(), ".."});
    const std::string runtimePath = File::PathJoin({profPath, Common::HOST, Common::SQLITE, "runtime.db"});
    if (!File::Exist(runtimePath))
    {
        WARN("Host runtime database does not exist; CCU mission stream id uses %", DEFAULT_STREAM_ID);
        return true;
    }

    DBRunner runner(runtimePath);
    std::vector<std::tuple<uint64_t>> tableCount;
    const std::string tableQuery = "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='HostTask'";
    if (!runner.QueryData(tableQuery, tableCount) || tableCount.size() != 1)
    {
        ERROR("Query HostTask table metadata failed");
        return false;
    }
    if (std::get<0>(tableCount[0]) == 0)
    {
        WARN("HostTask table does not exist; CCU mission stream id uses %", DEFAULT_STREAM_ID);
        return true;
    }
    const std::string sql =
        "SELECT task_id, stream_id FROM HostTask WHERE device_id=" + std::to_string(deviceInfo.deviceId) +
        " ORDER BY timestamp";
    std::vector<std::tuple<uint32_t, uint32_t>> hostRows;
    if (!runner.QueryData(sql, hostRows))
    {
        ERROR("Query HostTask failed");
        return false;
    }

    std::unordered_map<uint32_t, uint32_t> streamByTask;
    streamByTask.reserve(hostRows.size());
    for (const auto& row : hostRows)
    {
        const uint32_t taskId = std::get<0>(row);
        if (!streamByTask.emplace(taskId, std::get<1>(row)).second)
        {
            ERROR("Duplicate task_id found while resolving CCU stream id: %", taskId);
        }
    }
    for (auto& record : records)
    {
        const auto stream = streamByTask.find(record.taskId);
        if (stream != streamByTask.end())
        {
            record.streamId = stream->second;
        }
    }
    return true;
}

}  // namespace

bool CcuMissionParser::DecodeV6_1(const uint8_t* data, size_t size, std::vector<CcuMissionRecord>& records)
{
    if (data == nullptr || size != CCU_MISSION_RECORD_SIZE)
    {
        return false;
    }

    const size_t initialSize = records.size();
    std::array<uint16_t, 16> relativeTimes{};
    for (size_t i = 0; i < relativeTimes.size(); ++i)
    {
        relativeTimes[i] = ReadLittleEndian<uint16_t>(data + i * sizeof(uint16_t));
    }
    const uint64_t setckeBitStartTime = ReadLittleEndian<uint64_t>(data + 32);
    const uint16_t setckeBitInstrId = ReadLittleEndian<uint16_t>(data + 40);
    const uint64_t lpEndTime = ReadLittleEndian<uint64_t>(data + 42);
    const uint64_t lpStartTime = ReadLittleEndian<uint64_t>(data + 50);
    const uint16_t lpInstrId = ReadLittleEndian<uint16_t>(data + 58);
    const uint16_t rawStreamId = ReadLittleEndian<uint16_t>(data + 60);
    const uint16_t rawTaskId = ReadLittleEndian<uint16_t>(data + 62);
    const uint32_t taskId = (static_cast<uint32_t>(rawTaskId) << 16) | rawStreamId;

    if (setckeBitStartTime == 0)
    {
        records.push_back(
            {rawStreamId, taskId, lpInstrId, lpStartTime, lpEndTime, setckeBitInstrId, setckeBitStartTime, 0, 0});
        return true;
    }
    for (size_t i = 0; i < relativeTimes.size(); ++i)
    {
        const uint64_t relativeTime = static_cast<uint64_t>(relativeTimes[i]) * CCU_V6_1_RELATIVE_TIME_SCALE;
        if (setckeBitStartTime > std::numeric_limits<uint64_t>::max() - relativeTime)
        {
            ERROR("CCU mission relative end time overflow");
            records.resize(initialSize);
            return false;
        }
        const uint64_t relEndTime = setckeBitStartTime + relativeTime;
        records.push_back({rawStreamId, taskId, lpInstrId, lpStartTime, lpEndTime, setckeBitInstrId, setckeBitStartTime,
                           static_cast<uint32_t>(relativeTimes.size() - 1 - i), relEndTime});
    }
    return true;
}

uint32_t CcuMissionParser::ProcessEntry(DataInventory& dataInventory, const Infra::Context& context)
{
    if (context.GetChipID() != CHIP_V6_1_0)
    {
        ERROR("Unsupported chip id for CCU v6.1 mission parser: %", context.GetChipID());
        return ANALYSIS_ERROR;
    }
    const auto& deviceContext = static_cast<const DeviceContext&>(context);
    CcuMissionDataSet parsedData;
    const auto decoder = [&parsedData](const uint8_t* data, size_t size)
    { return CcuMissionParser::DecodeV6_1(data, size, parsedData.records); };
    if (!ParseSourceFiles(deviceContext.GetDeviceFilePath(), CcuDataKind::MISSION, CCU_MISSION_RECORD_SIZE, decoder,
                          parsedData.completedFiles, parsedData.hasCompletedFiles))
    {
        return ANALYSIS_ERROR;
    }
    if (!ResolveMissionStreamIds(deviceContext, parsedData.records))
    {
        return ANALYSIS_ERROR;
    }

    std::shared_ptr<CcuMissionDataSet> result;
    MAKE_SHARED0_RETURN_VALUE(result, CcuMissionDataSet, ANALYSIS_ERROR);
    *result = std::move(parsedData);
    if (!dataInventory.Inject(result))
    {
        ERROR("Inject CCU mission data failed");
        return ANALYSIS_ERROR;
    }
    INFO("Parsed % CCU mission rows from % files", result->records.size(), result->completedFiles.size());
    return ANALYSIS_OK;
}

bool CcuChannelParser::DecodeV6_1(const uint8_t* data, size_t size, std::vector<CcuChannelRecord>& records)
{
    if (data == nullptr || size != CCU_CHANNEL_RECORD_SIZE)
    {
        return false;
    }

    const size_t initialSize = records.size();
    size_t wordOffset = 0;
    for (uint32_t channelId = 0; channelId < CCU_V6_1_CHANNEL_COUNT; ++channelId)
    {
        const bool specialChannel = channelId == 120 || channelId == 124;
        const size_t dataOffset = wordOffset + (specialChannel ? 6 : 0);
        const uint64_t timestamp =
            (static_cast<uint64_t>(ReadLittleEndian<uint32_t>(data + (dataOffset + 1) * 4)) << 32) |
            ReadLittleEndian<uint32_t>(data + dataOffset * 4);
        const uint32_t avgBw = ReadLittleEndian<uint32_t>(data + (dataOffset + 2) * 4);
        const uint32_t minBw = ReadLittleEndian<uint32_t>(data + (dataOffset + 3) * 4);
        const uint32_t maxBw = ReadLittleEndian<uint32_t>(data + (dataOffset + 4) * 4);
        records.push_back({channelId, timestamp, maxBw, minBw, avgBw});

        if (channelId < 120)
        {
            wordOffset += channelId % 12 == 11 ? 9 : 5;
        }
        else
        {
            static const std::array<size_t, 8> tailIntervals = {{11, 5, 5, 11, 11, 5, 5, 11}};
            wordOffset += tailIntervals[channelId - 120];
        }
    }
    return records.size() - initialSize == CCU_V6_1_CHANNEL_COUNT;
}

uint32_t CcuChannelParser::ProcessEntry(DataInventory& dataInventory, const Infra::Context& context)
{
    if (context.GetChipID() != CHIP_V6_1_0)
    {
        ERROR("Unsupported chip id for CCU v6.1 channel parser: %", context.GetChipID());
        return ANALYSIS_ERROR;
    }
    const auto& deviceContext = static_cast<const DeviceContext&>(context);
    CcuChannelDataSet parsedData;
    const auto decoder = [&parsedData](const uint8_t* data, size_t size)
    { return CcuChannelParser::DecodeV6_1(data, size, parsedData.records); };
    if (!ParseSourceFiles(deviceContext.GetDeviceFilePath(), CcuDataKind::CHANNEL, CCU_CHANNEL_RECORD_SIZE, decoder,
                          parsedData.completedFiles, parsedData.hasCompletedFiles))
    {
        return ANALYSIS_ERROR;
    }

    std::shared_ptr<CcuChannelDataSet> result;
    MAKE_SHARED0_RETURN_VALUE(result, CcuChannelDataSet, ANALYSIS_ERROR);
    *result = std::move(parsedData);
    if (!dataInventory.Inject(result))
    {
        ERROR("Inject CCU channel data failed");
        return ANALYSIS_ERROR;
    }
    INFO("Parsed % CCU channel rows from % files", result->records.size(), result->completedFiles.size());
    return ANALYSIS_OK;
}

namespace CCU_MISSION_REGISTER
{
REGISTER_PROCESS_SEQUENCE(CcuMissionParser, true);
REGISTER_PROCESS_SUPPORT_CHIP(CcuMissionParser, CHIP_V6_1_0);
}  // namespace CCU_MISSION_REGISTER

namespace CCU_CHANNEL_REGISTER
{
REGISTER_PROCESS_SEQUENCE(CcuChannelParser, true);
REGISTER_PROCESS_SUPPORT_CHIP(CcuChannelParser, CHIP_V6_1_0);
}  // namespace CCU_CHANNEL_REGISTER

}  // namespace Domain
}  // namespace Analysis
