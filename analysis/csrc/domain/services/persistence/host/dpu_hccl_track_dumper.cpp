/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of the Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/services/persistence/host/dpu_hccl_track_dumper.h"

#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/domain/services/persistence/host/number_mapping.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
DpuHcclTrackDumper::DpuHcclTrackDumper(const std::string &hostPath)
    : BaseDumper<DpuHcclTrackDumper>(hostPath, "DPUHcclTrack")
{
    MAKE_SHARED0_NO_OPERATION(database_, DPUDB);
}

DpuHcclTrackRows DpuHcclTrackDumper::GenerateData(const std::vector<std::shared_ptr<ParserAdditionalInfo>> &records)
{
    DpuHcclTrackRows rows;
    if (!Utils::Reserve(rows, records.size()))
    {
        ERROR("DpuHcclTrackDumper: Reserve data failed.");
        return rows;
    }
    const auto &hash = Host::Cann::HashData::GetInstance().GetAll();
    for (const auto &info : records)
    {
        if (!info)
        {
            continue;
        }
        const auto &data = info->dpuHcclInfo;
        const std::string itemName = (data.itemId == 0) ? "0" : Host::Cann::HashData::GetInstance().Get(data.itemId);
        const auto groupIt = hash.find(data.groupName);
        const std::string &groupName = (groupIt == hash.end()) ? Analysis::Common::NA : groupIt->second;
        rows.emplace_back(
            static_cast<uint32_t>(data.npuDeviceId),
            static_cast<uint32_t>(data.filed24 & DPU_HCCL_TRACK_DEVICE_ID_MASK), info->threadId, data.startTime,
            info->timeStamp, itemName, groupName, std::to_string(data.groupName), data.localRank, data.remoteRank,
            data.rankSize, static_cast<double>(data.durationEstimated), std::to_string(data.srcAddr),
            std::to_string(data.dstAddr), data.dataSize, static_cast<uint32_t>(data.streamId), data.taskId,
            data.aicpuTaskId, static_cast<uint32_t>(data.planeId),
            NumberMapping::Get(NumberMapping::MappingType::HCCL_OP_TYPE, data.opType),
            NumberMapping::Get(NumberMapping::MappingType::HCCL_DATA_TYPE, data.dataType),
            NumberMapping::Get(NumberMapping::MappingType::HCCL_LINK_TYPE, data.linkType),
            NumberMapping::Get(NumberMapping::MappingType::HCCL_TRANSPORT_TYPE, data.transportType),
            NumberMapping::Get(NumberMapping::MappingType::HCCL_RDMA_TYPE, data.rdmaType),
            NumberMapping::Get(NumberMapping::MappingType::HCCL_ROLE_TYPE, data.role), std::to_string(data.cclTag),
            std::to_string(data.notifyId), std::to_string(data.workFlowMode), std::to_string(data.stage));
    }
    return rows;
}
}  // namespace Domain
}  // namespace Analysis
