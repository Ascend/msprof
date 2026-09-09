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

#include "analysis/csrc/domain/services/parser/host/cann/ccu_add_info_parser.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "analysis/csrc/domain/services/adapter/parser_struct_adapter.h"
#include "analysis/csrc/domain/services/persistence/host/number_mapping.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{
namespace
{
constexpr std::size_t CCU_ADD_INFO_RECORD_SIZE = 256;
constexpr std::size_t CHANNEL_COUNT = 16;
const std::regex TASK_FILE_PATTERN(R"(^(unaging|aging)\.additional\.ccu_task_info\.slice_[0-9]+)");
const std::regex WAIT_SIGNAL_FILE_PATTERN(R"(^(unaging|aging)\.additional\.ccu_wait_signal_info\.slice_[0-9]+)");
const std::regex GROUP_FILE_PATTERN(R"(^(unaging|aging)\.additional\.ccu_group_info\.slice_[0-9]+)");

struct SourceFile
{
    std::string path;

    uint64_t size;
};

bool DiscoverFiles(const std::string& dataPath, const std::regex& pattern, std::vector<SourceFile>& files,
                   bool& hasCompleted)
{
    using namespace Utils;
    hasCompleted = false;
    if (!File::CheckDir(dataPath))
    {
        ERROR("Invalid CCU add-info directory: %", dataPath);
        return false;
    }
    auto paths = File::GetOriginData(dataPath, {"unaging.additional.ccu_", "aging.additional.ccu_"},
                                     {".complete", ".done", ".zip"});
    paths.erase(std::remove_if(paths.begin(), paths.end(), [&pattern](const std::string& path)
                               { return !std::regex_search(File::BaseName(path), pattern); }),
                paths.end());
    for (const auto& path : paths)
    {
        uint32_t slice = 0;
        if (StrToU32(slice, Split(File::BaseName(path), "_").back()) != ANALYSIS_OK)
        {
            ERROR("Invalid CCU add-info slice number: %", path);
            return false;
        }
    }
    File::SortFilesByAgingAndSliceNum(paths);
    for (const auto& path : paths)
    {
        if (File::Exist(path + ".complete"))
        {
            hasCompleted = true;
            continue;
        }
        const auto size = File::Size(path);
        if (!FileReader::Check(path) || size == 0)
        {
            ERROR("Invalid CCU add-info file: %", path);
            return false;
        }
        files.push_back({path, size});
    }
    if (hasCompleted && !files.empty())
    {
        ERROR("Partially completed CCU add-info; clear parsed output and reimport the full capture");
        return false;
    }
    INFO("Found % pending CCU add-info files", files.size());
    return true;
}

bool AdaptCcuRecord(const uint8_t* rawData, std::size_t size, AdditionalInfoFormat format, ParserAdditionalInfo& parsed)
{
    static_assert(sizeof(MsprofAdditionalInfo) == CCU_ADD_INFO_RECORD_SIZE, "Unexpected additional record size");
    if (rawData == nullptr || size != CCU_ADD_INFO_RECORD_SIZE)
    {
        ERROR("Invalid CCU add-info record");
        return false;
    }
    // The raw slice need not be aligned for a MsprofAdditionalInfo object.
    MsprofAdditionalInfo source{};
    std::memcpy(&source, rawData, sizeof(source));
    return Adapter::ParserAdditionalInfoAdapter::AdapterAdditionalInfo(&source, &parsed, format);
}

template <typename Decoder>
bool ParseRecords(const std::string& dataPath, const std::regex& pattern, std::vector<std::string>& completedFiles,
                  bool& sourceParsed, bool& sourceCompleted, Decoder decoder)
{
    std::vector<SourceFile> files;
    if (!DiscoverFiles(dataPath, pattern, files, sourceCompleted))
    {
        return false;
    }
    sourceParsed = !files.empty();
    uint64_t leadingOffset = 0;
    for (const auto& file : files)
    {
        leadingOffset = (leadingOffset + file.size % CCU_ADD_INFO_RECORD_SIZE) % CCU_ADD_INFO_RECORD_SIZE;
    }
    std::array<uint8_t, CCU_ADD_INFO_RECORD_SIZE> record{};
    std::size_t cached = 0;
    for (const auto& file : files)
    {
        if (Utils::File::Size(file.path) != file.size)
        {
            return false;
        }
        std::stringstream input;
        Utils::FileReader reader(file.path, std::ios::in | std::ios::binary);
        if (reader.ReadBinary(input) != ANALYSIS_OK || input.tellp() != static_cast<std::streamoff>(file.size))
        {
            ERROR("Read CCU add-info failed or size changed: %", file.path);
            return false;
        }
        const auto offset = std::min(leadingOffset, file.size);
        leadingOffset -= offset;
        input.seekg(static_cast<std::streamoff>(offset));
        uint64_t remaining = file.size - offset;
        while (remaining != 0)
        {
            const auto size =
                static_cast<std::size_t>(std::min<uint64_t>(remaining, CCU_ADD_INFO_RECORD_SIZE - cached));
            input.read(reinterpret_cast<char*>(record.data() + cached), static_cast<std::streamsize>(size));
            if (static_cast<std::size_t>(input.gcount()) != size)
            {
                ERROR("Read CCU add-info failed: %", file.path);
                return false;
            }
            remaining -= size;
            cached += size;
            if (cached == CCU_ADD_INFO_RECORD_SIZE)
            {
                if (!decoder(record.data(), record.size()))
                {
                    return false;
                }
                cached = 0;
            }
        }
        if (!input || Utils::File::Size(file.path) != file.size)
        {
            return false;
        }
        completedFiles.push_back(file.path);
    }
    return cached == 0;
}

std::string GetReduceOpType(uint8_t value)
{
    return NumberMapping::Get(NumberMapping::MappingType::HCCL_OP_TYPE, value);
}

std::string GetDataType(uint8_t value) { return NumberMapping::Get(NumberMapping::MappingType::HCCL_DATA_TYPE, value); }
}  // namespace

bool CcuInfoData::HasInput() const
{
    return !Empty() || !completedFiles.empty() || taskSourceParsed || waitSignalSourceParsed || groupSourceParsed ||
           taskSourceCompleted || waitSignalSourceCompleted || groupSourceCompleted;
}

bool CcuAddInfoParser::Parse(CcuInfoData& data) const
{
    data = CcuInfoData{};
    bool result = ParseRecords(dataPath_, TASK_FILE_PATTERN, data.completedFiles, data.taskSourceParsed,
                               data.taskSourceCompleted, [&data](const uint8_t* rawData, std::size_t size)
                               { return DecodeTask(rawData, size, data.taskRecords); });
    if (!ParseRecords(dataPath_, WAIT_SIGNAL_FILE_PATTERN, data.completedFiles, data.waitSignalSourceParsed,
                      data.waitSignalSourceCompleted, [&data](const uint8_t* rawData, std::size_t size)
                      { return DecodeWaitSignal(rawData, size, data.waitSignalRecords); }))
    {
        result = false;
    }
    if (!ParseRecords(dataPath_, GROUP_FILE_PATTERN, data.completedFiles, data.groupSourceParsed,
                      data.groupSourceCompleted, [&data](const uint8_t* rawData, std::size_t size)
                      { return DecodeGroup(rawData, size, data.groupRecords); }))
    {
        result = false;
    }
    if (!result)
    {
        ERROR("Parse CCU host data failed; source files remain incomplete: %", dataPath_);
        data = CcuInfoData{};
    }
    return result;
}

bool CcuAddInfoParser::DecodeTask(const uint8_t* rawData, std::size_t size, std::vector<CcuTaskInfoRecord>& records)
{
    ParserAdditionalInfo parsed{};
    if (!AdaptCcuRecord(rawData, size, AdditionalInfoFormat::CCU_TASK_INFO_TYPE, parsed))
    {
        return false;
    }
    const auto& info = parsed.ccuInfo;
    records.push_back({info.version, info.workFlowMode, std::to_string(info.itemId), std::to_string(info.groupName),
                       info.rankId, info.rankSize, info.streamId, info.taskId, info.dieId, info.missionId,
                       info.instrId});
    return true;
}

bool CcuAddInfoParser::DecodeWaitSignal(const uint8_t* rawData, std::size_t size,
                                        std::vector<CcuWaitSignalInfoRecord>& records)
{
    ParserAdditionalInfo parsed{};
    if (!AdaptCcuRecord(rawData, size, AdditionalInfoFormat::CCU_WAIT_SIGNAL_INFO_TYPE, parsed))
    {
        return false;
    }
    const auto& info = parsed.ccuInfo;
    bool hasValidChannel = false;
    for (std::size_t index = 0; index < CHANNEL_COUNT; ++index)
    {
        if (info.channelIds[index] == std::numeric_limits<uint16_t>::max() ||
            info.remoteRankIds[index] == std::numeric_limits<uint32_t>::max())
        {
            continue;
        }
        hasValidChannel = true;
        records.push_back({info.version, std::to_string(info.itemId), std::to_string(info.groupName), info.rankId,
                           info.rankSize, info.workFlowMode, info.streamId, info.taskId, info.dieId, info.instrId,
                           info.missionId, info.ckeId, info.mask, info.channelIds[index], info.remoteRankIds[index]});
    }
    if (!hasValidChannel)
    {
        WARN("CCU wait-signal info has no valid channel, stream id: %, task id: %, instr id: %", info.streamId,
             info.taskId, info.instrId);
    }
    return true;
}

bool CcuAddInfoParser::DecodeGroup(const uint8_t* rawData, std::size_t size, std::vector<CcuGroupInfoRecord>& records)
{
    ParserAdditionalInfo parsed{};
    if (!AdaptCcuRecord(rawData, size, AdditionalInfoFormat::CCU_GROUP_INFO_TYPE, parsed))
    {
        return false;
    }
    const auto& info = parsed.ccuInfo;
    bool hasValidChannel = false;
    for (std::size_t index = 0; index < CHANNEL_COUNT; ++index)
    {
        if (info.channelIds[index] == std::numeric_limits<uint16_t>::max() ||
            info.remoteRankIds[index] == std::numeric_limits<uint32_t>::max())
        {
            continue;
        }
        hasValidChannel = true;
        records.push_back({info.version, std::to_string(info.itemId), std::to_string(info.groupName), info.rankId,
                           info.rankSize, info.workFlowMode, info.streamId, info.taskId, info.dieId, info.instrId,
                           info.missionId, GetReduceOpType(info.reduceOpType), GetDataType(info.inputDataType),
                           GetDataType(info.outputDataType), info.dataSize, info.channelIds[index],
                           info.remoteRankIds[index]});
    }
    if (!hasValidChannel)
    {
        WARN("CCU group info has no valid channel, stream id: %, task id: %, instr id: %", info.streamId, info.taskId,
             info.instrId);
    }
    return true;
}

}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis
