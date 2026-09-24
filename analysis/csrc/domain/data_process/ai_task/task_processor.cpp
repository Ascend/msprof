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
#include "analysis/csrc/domain/data_process/ai_task/task_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

TaskProcessor::TaskProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool TaskProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<AscendTaskData> allProcessedData;
    for (const auto &devicePath : deviceList)
    {
        flag = ProcessSingleDevice(devicePath, allProcessedData) && flag;
    }
    if (!SaveToDataInventory<AscendTaskData>(std::move(allProcessedData), dataInventory, PROCESSOR_NAME_TASK))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_TASK);
        flag = false;
    }
    return flag;
}

bool TaskProcessor::ProcessSingleDevice(const std::string &devicePath, std::vector<AscendTaskData> &allProcessedData)
{
    ProfTimeRecord record;
    DBInfo ascendTaskDB("ascend_task.db", "AscendTask");
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, ascendTaskDB.dbName});
    if (!ascendTaskDB.ConstructDBRunner(dbPath))
    {
        return false;
    }
    auto status = CheckPathAndTable(dbPath, ascendTaskDB);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
    if (deviceId == INVALID_DEVICE_ID)
    {
        ERROR("the invalid deviceId cannot to be identified.");
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_, deviceId))
    {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", profPath_);
        return false;
    }
    auto oriData = LoadData(ascendTaskDB, dbPath);
    if (oriData.empty())
    {
        WARN("AscendTask original data is empty. DBPath is %", dbPath);
        return true;
    }
    auto formatData = FormatData(oriData, record, deviceId);
    if (formatData.empty())
    {
        ERROR("AscendTask format data failed. DBPath is %", dbPath);
        return false;
    }
    FilterDataByStartTime<AscendTaskData>(
        formatData, record.startTimeNs, PROCESSOR_NAME_TASK, [](const AscendTaskData &data, uint64_t startTimeNs)
        { return static_cast<double>(data.timestamp) + data.duration < static_cast<double>(startTimeNs); });
    allProcessedData.insert(allProcessedData.end(), formatData.begin(), formatData.end());
    return true;
}

OriAscendTaskData TaskProcessor::LoadData(const DBInfo &ascendTaskDB, const std::string &dbPath)
{
    OriAscendTaskData oriData;
    if (ascendTaskDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql{
        "SELECT a.start_time, a.duration, a.model_id, a.index_id, a.stream_id, a.task_id, a.context_id,"
        "a.batch_id, a.connection_id, a.host_task_type, a.device_task_type FROM " +
        ascendTaskDB.tableName + " a WHERE a.device_task_type != 'UNKNOWN'"};
    if (!ascendTaskDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", ascendTaskDB.tableName);
    }
    return oriData;
}

std::vector<AscendTaskData> TaskProcessor::FormatData(const OriAscendTaskData &oriData,
                                                      const ProfTimeRecord &timeRecord, const uint16_t deviceId)
{
    std::vector<AscendTaskData> processedData;
    if (!Utils::Reserve(processedData, oriData.size()))
    {
        ERROR("Reserve for AscendTask data failed.");
        return processedData;
    }
    AscendTaskData data;
    data.deviceId = deviceId;
    double tmpStart;
    uint16_t platformVersion = Context::GetInstance().GetPlatformVersion(deviceId, profPath_);
    for (const auto &row : oriData)
    {
        std::tie(tmpStart, data.duration, data.modelId, data.indexId, data.streamId, data.taskId, data.contextId,
                 data.batchId, data.connectionId, data.hostType, data.deviceType) = row;
        HPFloat start{tmpStart};
        HPFloat end = start + HPFloat(data.duration);
        data.timestamp = GetLocalTime(start, timeRecord).Uint64();
        data.end = GetLocalTime(end, timeRecord).Uint64();
        data.taskType = GetTaskType(data.hostType, data.deviceType, platformVersion);
        processedData.push_back(data);
    }
    return processedData;
}

std::string TaskProcessor::GetTaskType(const std::string &hostType, const std::string &deviceType,
                                       uint16_t platformVersion)
{
    std::string taskType;
    std::map<std::string, std::string> sqeType;
    if (Context::IsStarsChip(platformVersion))
    {
        sqeType = STARS_SQE_TYPE_TABLE;
    }
    else
    {
        sqeType = HW_SQE_TYPE_TABLE;
    }
    if (hostType != "FFTS_PLUS" && hostType != "UNKNOWN")
    {
        taskType = hostType;
    }
    else if (hostType == "FFTS_PLUS")
    {
        taskType = deviceType;
    }
    else
    {
        if (!Utils::IsNumber(deviceType))
        {
            taskType = deviceType;
        }
        else if (platformVersion != static_cast<int>(Chip::CHIP_V2_1_0) && sqeType.find(deviceType) != sqeType.end())
        {
            taskType = sqeType[deviceType];
        }
        else
        {
            taskType = "Other";
        }
    }
    return taskType;
}
}  // namespace Domain
}  // namespace Analysis
