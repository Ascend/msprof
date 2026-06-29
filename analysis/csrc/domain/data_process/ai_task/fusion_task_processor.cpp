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

#include "analysis/csrc/domain/data_process/ai_task/fusion_task_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

FusionTaskProcessor::FusionTaskProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool FusionTaskProcessor::Process(DataInventory &dataInventory)
{
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    if (deviceList.empty())
    {
        return false;
    }
    bool flag = true;
    std::vector<FusionTaskTimelineData> result;
    for (const auto &devicePath : deviceList)
    {
        uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
        ProfTimeRecord record;
        if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_, deviceId))
        {
            ERROR("GetProfTimeRecordInfo failed, profPath is %, device id is %.", profPath_, deviceId);
            flag = false;
            continue;
        }
        DBInfo dbInfo("fusion_task.db", "FusionTask");
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, dbInfo.dbName});
        if (!dbInfo.ConstructDBRunner(dbPath))
        {
            ERROR("Construct fusion task db runner failed.");
            flag = false;
            continue;
        }
        auto status = CheckPathAndTable(dbPath, dbInfo, false);
        if (status == CHECK_FAILED)
        {
            flag = false;
            continue;
        }
        else if (status != CHECK_SUCCESS)
        {
            continue;
        }
        OriFusionTaskFormat oriData;
        std::string sql =
            "SELECT stream_id, task_id, acc_id, task_type, start_time, end_time, "
            "task_time, fusion_task_type, mission_id, ccu_die_id FROM " +
            dbInfo.tableName + ";";
        if (!dbInfo.dbRunner->QueryData(sql, oriData))
        {
            ERROR("Query fusion task data failed, db path is %.", dbPath);
            flag = false;
            continue;
        }
        auto oneDeviceData = FormatData(oriData, deviceId, record);
        result.insert(result.end(), oneDeviceData.begin(), oneDeviceData.end());
    }
    if (!SaveToDataInventory<FusionTaskTimelineData>(std::move(result), dataInventory, PROCESSOR_NAME_FUSION_TASK))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_FUSION_TASK);
        return false;
    }
    return flag;
}

std::vector<FusionTaskTimelineData> FusionTaskProcessor::FormatData(const OriFusionTaskFormat &oriData,
                                                                    uint16_t deviceId,
                                                                    const ProfTimeRecord &record) const
{
    std::vector<FusionTaskTimelineData> result;
    if (oriData.empty())
    {
        return result;
    }
    // DB stores absolute nanosecond timestamps (converted by Python time_from_syscnt).
    // Only GetLocalTime is needed — do NOT apply GetTimeFromSyscnt again (double conversion).
    if (!Reserve(result, oriData.size()))
    {
        ERROR("Reserve fusion task formatted data failed.");
        return result;
    }
    for (const auto &row : oriData)
    {
        FusionTaskTimelineData oneData;
        std::tie(oneData.streamId, oneData.taskId, oneData.accId, oneData.taskType, oneData.startTime, oneData.endTime,
                 oneData.duration, oneData.fusionTaskType, oneData.missionId, oneData.ccuDieId) = row;
        oneData.deviceId = deviceId;
        oneData.isCcu = (oneData.fusionTaskType == "CCU");
        oneData.hasCcuDie = oneData.isCcu && (oneData.ccuDieId != UINT32_MAX);
        HPFloat absStart(static_cast<double>(oneData.startTime));
        HPFloat absEnd(static_cast<double>(oneData.endTime));
        oneData.startTime = GetLocalTime(absStart, record).Uint64();
        oneData.endTime = GetLocalTime(absEnd, record).Uint64();
        result.push_back(oneData);
    }
    return result;
}
}  // namespace Domain
}  // namespace Analysis
