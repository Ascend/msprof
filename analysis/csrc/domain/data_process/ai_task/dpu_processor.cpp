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

#include "analysis/csrc/domain/data_process/ai_task/dpu_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

DPUProcessor::DPUProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool DPUProcessor::Process(DataInventory &dataInventory)
{
    INFO("Start DPU Processor");
    ProfTimeRecord record;
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_))
    {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", profPath_);
        return false;
    }
    SyscntConversionParams params;
    if (!Context::GetInstance().GetSyscntConversionParams(params, HOST_ID, profPath_))
    {
        ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
        return false;
    }

    auto trackData = LoadTrackData();
    auto hcclTrackData = LoadHcclTrackData();

    if (trackData.empty() && hcclTrackData.empty())
    {
        WARN("No dpu data found.");
        return true;
    }

    std::vector<DPUData> dpuData;
    if (!Reserve(dpuData, trackData.size() + hcclTrackData.size()))
    {
        ERROR("Reserved for dpu track data failed, size is %", trackData.size() + hcclTrackData.size());
        return false;
    }

    FormatHcclTrackData(hcclTrackData, dpuData, record, params);
    FormatTrackData(trackData, dpuData, record, params);
    FilterDataByStartTime(dpuData, record.startTimeNs, PROCESSOR_NAME_DPU);

    if (!SaveToDataInventory<DPUData>(std::move(dpuData), dataInventory, PROCESSOR_NAME_DPU))
    {
        ERROR("Save dpu track data failed");
        return false;
    }
    INFO("End DPU Processor");
    return true;
}

DPUProcessor::TrackFormat DPUProcessor::LoadTrackData()
{
    TrackFormat oriData;
    DBInfo dpuDB("dpu.db", "DPUTaskTrack");
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, dpuDB.dbName});
    if (!dpuDB.ConstructDBRunner(dbPath))
    {
        return oriData;
    }

    auto status = CheckPathAndTable(dbPath, dpuDB);
    if (status != CHECK_SUCCESS)
    {
        return oriData;
    }

    std::string sql =
        "SELECT dpu_device_id, thread_id, start_time, end_time, task_type, stream_id, task_id, "
        "kernel_name FROM " +
        dpuDB.tableName;
    if (!dpuDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query dpu track data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

DPUProcessor::HcclTrackFormat DPUProcessor::LoadHcclTrackData()
{
    HcclTrackFormat oriData;
    DBInfo dpuDB("dpu.db", "DPUHcclTrack");
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, dpuDB.dbName});
    if (!dpuDB.ConstructDBRunner(dbPath))
    {
        return oriData;
    }

    auto status = CheckPathAndTable(dbPath, dpuDB);
    if (status != CHECK_SUCCESS)
    {
        return oriData;
    }

    std::string sql =
        "SELECT npu_device_id, dpu_device_id, thread_id, start_time, end_time, op_name, group_name, "
        "group_name_id, local_rank, remote_rank, rank_size, duration_estimated, src_addr, dst_addr, "
        "data_size, stream_id, task_id, aicpu_task_id, plane_id, op_type, data_type, link_type, "
        "transport_type, rdma_type, notify_id FROM " +
        dpuDB.tableName;
    if (!dpuDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query dpu hccl track data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

void DPUProcessor::FormatTrackData(const DPUProcessor::TrackFormat &oriData, std::vector<DPUData> &dpuData,
                                   const ProfTimeRecord &record, const SyscntConversionParams &params)
{
    for (const auto &row : oriData)
    {
        DPUData data;
        std::tie(data.dpuDeviceId, data.threadId, data.timestamp, data.endTime, data.taskType, data.streamId,
                 data.taskId, data.opName) = row;

        HPFloat startTimestamp = Utils::GetTimeFromHostCnt(data.timestamp, params);
        HPFloat endTimestamp = Utils::GetTimeFromHostCnt(data.endTime, params);

        data.timestamp = GetLocalTime(startTimestamp, record).Uint64();
        data.endTime = GetLocalTime(endTimestamp, record).Uint64();
        data.isHccl = false;

        dpuData.push_back(data);
    }
}

void DPUProcessor::FormatHcclTrackData(const DPUProcessor::HcclTrackFormat &oriData, std::vector<DPUData> &dpuData,
                                       const ProfTimeRecord &record, const SyscntConversionParams &params)
{
    for (const auto &row : oriData)
    {
        DPUData data;
        std::tie(data.npuDeviceId, data.dpuDeviceId, data.threadId, data.timestamp, data.endTime, data.opName,
                 data.groupName, data.groupNameId, data.localRank, data.remoteRank, data.rankSize,
                 data.durationEstimated, data.srcAddr, data.dstAddr, data.dataSize, data.streamId, data.taskId,
                 data.aicpuTaskId, data.planeId, data.opType, data.dataType, data.linkType, data.transportType,
                 data.rdmaType, data.notifyId) = row;

        HPFloat startTimestamp = Utils::GetTimeFromHostCnt(data.timestamp, params);
        HPFloat endTimestamp = Utils::GetTimeFromHostCnt(data.endTime, params);

        data.timestamp = GetLocalTime(startTimestamp, record).Uint64();
        data.endTime = GetLocalTime(endTimestamp, record).Uint64();
        data.isHccl = true;

        dpuData.push_back(data);
    }
}
}  // namespace Domain
}  // namespace Analysis
