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

#include "analysis/csrc/domain/services/parser/host/cann/compact_info_parser.h"

#include <cstring>
#include <memory>

#include "analysis/csrc/domain/services/adapter/parser_struct_adapter.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{
using namespace Analysis::Domain::Adapter;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

namespace
{
enum class DevType : uint16_t
{
    NPU = 0,
    DPU = 1,
};

constexpr uint16_t kDevTypeShift = 12;
constexpr uint16_t kDevTypeMask = 0xF;

bool IsDpuTask(uint16_t deviceId)
{
    return static_cast<DevType>((deviceId >> kDevTypeShift) & kDevTypeMask) == DevType::DPU;
}
}  // namespace

const std::unordered_map<uint64_t, uint64_t> &TaskTrackParser::GetDpuKernelNameMap() const { return dpuKernelNameMap_; }

void CompactInfoParser::Init(const std::vector<std::string> &filePrefix)
{
    MAKE_SHARED_NO_OPERATION(chunkProducer_, ChunkGenerator, sizeof(MsprofCompactInfo), path_, filePrefix);
}

template <>
std::vector<std::shared_ptr<ParserCompactInfo>> CompactInfoParser::GetData()
{
    return compactData_;
}

template <>
std::vector<std::shared_ptr<FlipTask>> CompactInfoParser::GetData()
{
    return flipTaskData_;
}

int CompactInfoParser::ProduceData()
{
    if (chunkProducer_->Empty())
    {
        return ANALYSIS_OK;
    }
    if (!Reserve(compactData_, chunkProducer_->Size()))
    {
        ERROR("%: Reserve data failed", parserName_);
        return ANALYSIS_ERROR;
    }
    while (!chunkProducer_->Empty())
    {
        // Pop 内部按 new char[chunkSize] 分配，用 unique_ptr<char[]> 接管以保证按数组规则释放
        std::unique_ptr<char[]> chunk(chunkProducer_->Pop());
        auto compactInfo = ReinterpretConvert<MsprofCompactInfo *>(chunk.get());
        if (!compactInfo)
        {
            ERROR("%: Pop chunk failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (compactInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM)
        {
            ERROR("%: The last %th data check failed.", parserName_, chunkProducer_->Size());
            continue;
        }
        auto parserCompactInfo = std::make_shared<ParserCompactInfo>();
        if (!ParserCompactInfoAdapter::AdapterCompactInfo(compactInfo, parserCompactInfo.get(), parserType_))
        {
            ERROR("%: copy compactInfo data failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        compactData_.emplace_back(std::move(parserCompactInfo));
    }
    return ANALYSIS_OK;
}

int NodeBasicInfoParser::ProduceData()
{
    std::shared_ptr<ChunkGenerator> staticChunkProducer;
    MAKE_SHARED_RETURN_VALUE(staticChunkProducer, ChunkGenerator, ANALYSIS_ERROR, sizeof(MsprofCompactInfo), path_,
                             staticFilePrefix_);
    if (staticChunkProducer->ReadChunk() != ANALYSIS_OK)
    {
        ERROR("%: Read Chunk failed.", parserName_);
        return ANALYSIS_ERROR;
    }
    if (chunkProducer_->Empty() && staticChunkProducer->Empty())
    {
        return ANALYSIS_OK;
    }
    if (!Reserve(compactData_, chunkProducer_->Size() + staticChunkProducer->Size()))
    {
        ERROR("%: Reserve data failed", parserName_);
        return ANALYSIS_ERROR;
    }
    std::map<OpType, std::shared_ptr<ChunkGenerator>> producerMap = {
        {OpType::STATIC_OP, staticChunkProducer},
        {OpType::DYNAMIC_OP, chunkProducer_},
    };
    for (const auto &item : producerMap)
    {
        const auto &opState = static_cast<uint8_t>(item.first);
        auto &producer = item.second;
        while (!producer->Empty())
        {
            // Pop 内部按 new char[chunkSize] 分配，用 unique_ptr<char[]> 接管以保证按数组规则释放
            std::unique_ptr<char[]> chunk(producer->Pop());
            auto compactInfo = ReinterpretConvert<MsprofCompactInfo *>(chunk.get());
            if (!compactInfo)
            {
                ERROR("%: Pop chunk failed.", parserName_);
                return ANALYSIS_ERROR;
            }
            if (compactInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM)
            {
                ERROR("%: The last %th data check failed with opState %.", parserName_, producer->Size(), opState);
                continue;
            }
            auto parserCompactInfo = std::make_shared<ParserCompactInfo>();
            if (!ParserCompactInfoAdapter::AdapterCompactInfo(compactInfo, parserCompactInfo.get(), parserType_))
            {
                ERROR("%: copy nodeBasic info data failed.", parserName_);
                return ANALYSIS_ERROR;
            }
            parserCompactInfo->data.nodeBasicInfo.opState = opState;
            compactData_.emplace_back(std::move(parserCompactInfo));
        }
    }
    return ANALYSIS_OK;
}

int TaskTrackParser::ProduceData()
{
    if (chunkProducer_->Empty())
    {
        return ANALYSIS_OK;
    }
    const uint64_t flipTaskType = 98;
    const uint64_t maintenanceTaskType = 6;
    if (!Reserve(compactData_, chunkProducer_->Size()))
    {
        ERROR("%: Reserve data failed", parserName_);
        return ANALYSIS_ERROR;
    }
    auto runtimeTrackFormat = GetRuntimeTrackFormat();
    while (!chunkProducer_->Empty())
    {
        // Pop 内部按 new char[chunkSize] 分配，用 unique_ptr<char[]> 接管以保证按数组规则释放
        std::unique_ptr<char[]> chunk(chunkProducer_->Pop());
        auto compactInfo = ReinterpretConvert<MsprofCompactInfo *>(chunk.get());
        if (!compactInfo)
        {
            ERROR("%: Pop chunk failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (compactInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM)
        {
            ERROR("%: The last %th data check failed.", parserName_, chunkProducer_->Size());
            continue;
        }
        auto parserCompactInfo = std::make_shared<ParserCompactInfo>();
        if (!ParserCompactInfoAdapter::AdapterRuntimeTrack(compactInfo, runtimeTrackFormat, parserCompactInfo.get()))
        {
            ERROR("%: copy runtimeTrack data failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (parserCompactInfo->data.runtimeTrack.taskType == flipTaskType)
        {
            auto flipTask = Flip::CreateFlipTask(parserCompactInfo.get());
            if (!flipTask)
            {
                ERROR("FlipTask is null.");
                return ANALYSIS_ERROR;
            }
            flipTaskData_.emplace_back(flipTask);
            continue;
        }

        if (parserCompactInfo->data.runtimeTrack.taskType == maintenanceTaskType)
        {
            continue;
        }

        // DPU 记录不进建树，只收集 kernelName
        if (IsDpuTask(parserCompactInfo->data.runtimeTrack.deviceId))
        {
            uint16_t devId = parserCompactInfo->data.runtimeTrack.deviceId;
            uint32_t taskId = parserCompactInfo->data.runtimeTrack.taskId;
            uint64_t key = (static_cast<uint64_t>(devId) << 32) | taskId;
            dpuKernelNameMap_[key] = parserCompactInfo->data.runtimeTrack.kernelName;
            continue;
        }
        compactData_.emplace_back(std::move(parserCompactInfo));
    }
    Sort<ParserCompactInfo, uint64_t, &ParserCompactInfo::timeStamp>(compactData_);
    Sort<FlipTask, uint64_t, &FlipTask::timeStamp>(flipTaskData_);
    if (Context::GetInstance().IsAllExport() &&
        !Environment::Context::GetInstance().IsChipV6(Environment::Context::GetInstance().GetPlatformVersion()))
    {
        Flip::ComputeBatchId(compactData_, flipTaskData_, runtimeTrackFormat);
    }
    return ANALYSIS_OK;
}
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis
