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

void CompactInfoParser::Init(const std::vector<std::string> &filePrefix)
{
    MAKE_SHARED_NO_OPERATION(chunkProducer_, ChunkGenerator, sizeof(MsprofCompactInfo), path_, filePrefix);
}

template <>
std::vector<std::shared_ptr<MsprofCompactInfo>> CompactInfoParser::GetData()
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
        auto compactInfo = ReinterpretConvert<MsprofCompactInfo *>(chunkProducer_->Pop());
        if (!compactInfo)
        {
            ERROR("%: Pop chunk failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (compactInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM)
        {
            ERROR("%: The last %th data check failed.", parserName_, chunkProducer_->Size());
            delete compactInfo;
            continue;
        }
        compactData_.emplace_back(std::shared_ptr<MsprofCompactInfo>(compactInfo));
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
            auto compactInfo = ReinterpretConvert<MsprofCompactInfo *>(producer->Pop());
            if (!compactInfo)
            {
                ERROR("%: Pop chunk failed.", parserName_);
                return ANALYSIS_ERROR;
            }
            if (compactInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM)
            {
                ERROR("%: The last %th data check failed with opState %.", parserName_, producer->Size(), opState);
                delete compactInfo;
                continue;
            }
            compactInfo->data.nodeBasicInfo.opState = opState;
            compactData_.emplace_back(std::shared_ptr<MsprofCompactInfo>(compactInfo));
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
    while (!chunkProducer_->Empty())
    {
        auto compactInfo = ReinterpretConvert<MsprofCompactInfo *>(chunkProducer_->Pop());
        if (!compactInfo)
        {
            ERROR("%: Pop chunk failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (compactInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM)
        {
            ERROR("%: The last %th data check failed.", parserName_, chunkProducer_->Size());
            delete compactInfo;
            continue;
        }
        if (compactInfo->data.runtimeTrack.taskType == flipTaskType)
        {
            auto flipTask = Flip::CreateFlipTask(compactInfo);
            delete compactInfo;
            if (!flipTask)
            {
                ERROR("FlipTask is null.");
                return ANALYSIS_ERROR;
            }
            flipTaskData_.emplace_back(flipTask);
            continue;
        }
        // 因为Maintenance task标记流销毁，device侧并不会上报，也没有更多的含义，所以无需关联和展示
        // 同时Maintenance task可能会在流销毁的flip task后面，影响batch id的计算，所以过滤掉
        if (compactInfo->data.runtimeTrack.taskType == maintenanceTaskType)
        {
            delete compactInfo;
            continue;
        }
        compactData_.emplace_back(std::shared_ptr<MsprofCompactInfo>(compactInfo));
    }
    Sort<MsprofCompactInfo, uint64_t, &MsprofCompactInfo::timeStamp>(compactData_);
    Sort<FlipTask, uint64_t, &FlipTask::timeStamp>(flipTaskData_);
    if (Context::GetInstance().IsAllExport() &&
        !Environment::Context::GetInstance().IsChipV6(Environment::Context::GetInstance().GetPlatformVersion()))
    {
        Flip::ComputeBatchId(compactData_, flipTaskData_);
    }
    return ANALYSIS_OK;
}
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis
