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

#include "analysis/csrc/domain/data_process/system/page_fault_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
namespace Domain
{
namespace
{
constexpr uint32_t DEFAULT_VALUE = 0;
const std::string PAGE_FAULT_DB_NAME = "soc_pmu.db";
const std::string PAGE_FAULT_TABLE_NAME = "PageFault";
const std::string TASK_INFO_DB_NAME = "ge_info.db";
const std::string TASK_INFO_TABLE_NAME = "TaskInfo";

uint32_t ParsePageFaultNum(const std::string& eventLists)
{
    if (eventLists.empty())
    {
        return DEFAULT_VALUE;
    }
    auto firstEvent = Utils::Split(eventLists, ",", 1);
    if (firstEvent.empty() || firstEvent.front().empty())
    {
        return DEFAULT_VALUE;
    }
    uint32_t pageFaultNum = DEFAULT_VALUE;
    if (Utils::StrToU32(pageFaultNum, firstEvent.front()) != ANALYSIS_OK)
    {
        ERROR("Parse page fault num failed, eventLists is %.", eventLists);
        return DEFAULT_VALUE;
    }
    return pageFaultNum;
}
}  // namespace

PageFaultProcessor::PageFaultProcessor(const std::string& profPath) : DataProcessor(profPath) {}

bool PageFaultProcessor::Process(DataInventory& dataInventory)
{
    bool flag = true;
    std::vector<PageFaultData> res;
    std::map<TaskId, std::string> taskOpMap;
    if (!LoadTaskOpMap(taskOpMap))
    {
        WARN("Load task op map failed, page fault data will be exported without op name.");
    }
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto& devicePath : deviceList)
    {
        flag = ProcessSingleDevice(devicePath, taskOpMap, res) && flag;
    }
    if (!SaveToDataInventory<PageFaultData>(std::move(res), dataInventory, PROCESSOR_NAME_PAGE_FAULT))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_PAGE_FAULT);
        flag = false;
    }
    return flag;
}

bool PageFaultProcessor::ProcessSingleDevice(const std::string& devicePath,
                                             const std::map<TaskId, std::string>& taskOpMap,
                                             std::vector<PageFaultData>& res)
{
    DBInfo socPmuDB(PAGE_FAULT_DB_NAME, PAGE_FAULT_TABLE_NAME);
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, socPmuDB.dbName});
    if (!socPmuDB.ConstructDBRunner(dbPath) || socPmuDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    auto status = CheckPathAndTable(dbPath, socPmuDB, false);
    if (status != CHECK_SUCCESS)
    {
        return status != CHECK_FAILED;
    }
    auto oriData = LoadData(socPmuDB, dbPath);
    if (oriData.empty())
    {
        WARN("Get % data failed in % or table is empty.", socPmuDB.tableName, dbPath);
        return true;
    }
    auto deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
    if (deviceId == Environment::INVALID_DEVICE_ID)
    {
        ERROR("The invalid deviceId cannot be identified.");
        return false;
    }
    auto processedData = FormatData(oriData, deviceId, taskOpMap);
    if (processedData.empty())
    {
        WARN("Format page fault data failed or no valid data exists, dbPath is %.", dbPath);
        return true;
    }
    res.insert(res.end(), processedData.begin(), processedData.end());
    return true;
}

OriPageFaultData PageFaultProcessor::LoadData(const DBInfo& socPmuDB, const std::string& dbPath)
{
    OriPageFaultData oriData;
    if (socPmuDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql{"SELECT task_type, stream_id, task_id, event_lists FROM " + socPmuDB.tableName};
    if (!socPmuDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", socPmuDB.tableName);
    }
    return oriData;
}

bool PageFaultProcessor::LoadTaskOpMap(std::map<TaskId, std::string>& taskOpMap)
{
    DBInfo taskInfoDB(TASK_INFO_DB_NAME, TASK_INFO_TABLE_NAME);
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, taskInfoDB.dbName});
    if (!taskInfoDB.ConstructDBRunner(dbPath) || taskInfoDB.dbRunner == nullptr)
    {
        WARN("Create % connection failed.", dbPath);
        return false;
    }
    auto status = CheckPathAndTable(dbPath, taskInfoDB, false);
    if (status != CHECK_SUCCESS)
    {
        return status == NOT_EXIST;
    }
    TaskOpInfo taskOpInfo;
    std::string sql{"SELECT op_name, stream_id, task_id, device_id FROM " + taskInfoDB.tableName};
    if (!taskInfoDB.dbRunner->QueryData(sql, taskOpInfo))
    {
        ERROR("Failed to obtain data from the % table.", taskInfoDB.tableName);
        return false;
    }
    for (const auto& row : taskOpInfo)
    {
        std::string opName;
        uint32_t streamId;
        uint32_t taskId;
        uint16_t deviceId;
        std::tie(opName, streamId, taskId, deviceId) = row;
        TaskId key(static_cast<uint16_t>(streamId), INVALID_BATCH_ID, taskId, INVALID_CONTEXT_ID, deviceId);
        taskOpMap.insert({key, opName});
    }
    return true;
}

std::vector<PageFaultData> PageFaultProcessor::FormatData(const OriPageFaultData& oriData, uint16_t deviceId,
                                                          const std::map<TaskId, std::string>& taskOpMap)
{
    std::vector<PageFaultData> processedData;
    if (!Utils::Reserve(processedData, oriData.size()))
    {
        ERROR("Reserve for page fault data failed.");
        return processedData;
    }
    PageFaultData data;
    for (const auto& row : oriData)
    {
        std::string eventLists;
        std::tie(data.taskType, data.streamId, data.taskId, eventLists) = row;
        data.deviceId = deviceId;
        data.pageFaultNum = ParsePageFaultNum(eventLists);
        TaskId key(static_cast<uint16_t>(data.streamId), INVALID_BATCH_ID, data.taskId, INVALID_CONTEXT_ID, deviceId);
        auto it = taskOpMap.find(key);
        data.opName = it == taskOpMap.end() ? NA : it->second;
        processedData.emplace_back(data);
    }
    return processedData;
}
}  // namespace Domain
}  // namespace Analysis
