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

#include "analysis/csrc/domain/services/association/include/ascend_task_association.h"

#include <algorithm>
#include <map>

#include "analysis/csrc/domain/entities/hal/include/ascend_obj.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/entities/hal/include/hal.h"
#include "analysis/csrc/domain/entities/hal/include/top_down_task.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/device_context/load_host_data.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Utils;
using namespace Analysis::Common;
namespace
{
const uint64_t DEFAULT_MODEL_ID = UINT32_MAX;
const int32_t DEFAULT_INDEX_ID = -1;
const int64_t DEFAULT_CONNECTION_ID = -1;
const uint64_t MILLI_SECOND = 1000;
const std::unordered_map<uint32_t, std::string> deviceTaskAcsqTypeMap{
    {0, "AI_CORE"},          {1, "AI_CPU"},         {2, "AIV_SQE"},           {3, "PLACE_HOLDER_SQE"},
    {4, "EVENT_RECORD_SQE"}, {5, "EVENT_WAIT_SQE"}, {6, "NOTIFY_RECORD_SQE"}, {7, "NOTIFY_WAIT_SQE"},
    {8, "WRITE_VALUE_SQE"},  {9, "VQ6_SQE"},        {10, "TOF_SQE"},          {11, "SDMA_SQE"},
    {12, "VPC_SQE"},         {13, "JPEGE_SQE"},     {14, "JPEGD_SQE"},        {15, "DSA_SQE"},
    {16, "ROCCE_SQE"},       {17, "PCIE_DMA_SQE"},  {18, "HOST_CPU_SQE"},     {19, "CDQM_SQE"},
    {20, "C_CORE_SQE"}};

const std::unordered_map<uint32_t, std::string> deviceTaskFftsPlusTypeMap{{0, "AIC"},
                                                                          {1, "AIV"},
                                                                          {3, "Notify Wait"},
                                                                          {4, "Notify Record"},
                                                                          {5, "Write Value"},
                                                                          {6, "MIX_AIC"},
                                                                          {7, "MIX_AIV"},
                                                                          {8, "SDMA"},
                                                                          {9, "Data Context"},
                                                                          {10, "Invalidate Data Context"},
                                                                          {11, "Writeback Data Context"},
                                                                          {12, "AI_CPU"},
                                                                          {13, "Load Context"},
                                                                          {15, "DSA"}};

SyscntConversionParams GetSyscntConversionParams(const DeviceContext& context)
{
    CpuInfo cpuInfo;
    context.Getter(cpuInfo);
    HostStartLog hostStartLog;
    context.Getter(hostStartLog);
    uint64_t hostMonotonic = hostStartLog.clockMonotonicRaw;
    DeviceInfo deviceInfo;
    context.Getter(deviceInfo);
    DeviceStartLog deviceStartLog;
    context.Getter(deviceStartLog);
    if (!IsDoubleEqual(cpuInfo.frequency, 0.0) && hostStartLog.cntVctDiff)
    {
        uint64_t diffTime = static_cast<uint64_t>(hostStartLog.cntVctDiff * MILLI_SECOND / cpuInfo.frequency);
        if (UINT64_MAX - hostStartLog.clockMonotonicRaw >= diffTime)
        {
            hostMonotonic = hostStartLog.clockMonotonicRaw + diffTime;
        }
    }
    SyscntConversionParams params{deviceInfo.hwtsFrequency, deviceStartLog.cntVct, hostMonotonic};
    return params;
}
}  // namespace

std::string GetDeviceTaskTypeStr(const DeviceTask& task)
{
    std::string res = UNKNOWN;
    if (task.logType == HalLogType::ACSQ_LOG)
    {
        auto it = deviceTaskAcsqTypeMap.find(task.taskType);
        if (it != deviceTaskAcsqTypeMap.end())
        {
            res = it->second;
        }
    }
    else if (task.logType == HalLogType::FFTS_LOG)
    {
        auto it = deviceTaskFftsPlusTypeMap.find(task.taskType);
        if (it != deviceTaskFftsPlusTypeMap.end())
        {
            res = it->second;
        }
    }
    return res;
}

void MergeByHostAndDeviceTask(std::vector<TopDownTask>& res, HostTask& hostTask, std::vector<DeviceTask>& deviceTask,
                              SyscntConversionParams& params)
{
    uint64_t count = 0;
    for (auto& task : deviceTask)
    {
        auto start = GetTimeFromSyscnt(task.taskStart, params);
        auto end = GetTimeFromSyscnt(task.taskEnd, params);
        // 有多个deviceTask且是第一个算子，标记记为true
        res.emplace_back((count == 0 && deviceTask.size() > 1), hostTask.taskId, hostTask.batchId, hostTask.streamId,
                         hostTask.contextId, hostTask.requestId, GetDeviceTaskTypeStr(task), hostTask.taskTypeStr,
                         hostTask.modelId, hostTask.connection_id, start.Double(), end.Double());
        count++;
    }
}

void MergeByOnlyHostTask(std::vector<TopDownTask>& res, std::vector<HostTask>& hostTask)
{
    for (auto& task : hostTask)
    {
        res.emplace_back(false, task.taskId, task.batchId, task.streamId, task.contextId, task.requestId, UNKNOWN,
                         task.taskTypeStr, task.modelId, task.connection_id, INVALID_TIME, INVALID_TIME);
    }
}

void MergeByOnlyDeviceTask(std::vector<TopDownTask>& res, std::vector<DeviceTask>& deviceTask, const TaskId& key,
                           SyscntConversionParams& params)
{
    for (auto& task : deviceTask)
    {
        auto start = GetTimeFromSyscnt(task.taskStart, params);
        auto end = GetTimeFromSyscnt(task.taskEnd, params);
        res.emplace_back(false, key.taskId, key.batchId, key.streamId, key.contextId, DEFAULT_INDEX_ID,
                         GetDeviceTaskTypeStr(task), UNKNOWN, DEFAULT_MODEL_ID, DEFAULT_CONNECTION_ID, start.Double(),
                         end.Double());
    }
}

std::vector<TopDownTask> GenerateTopDownTask(std::map<TaskId, std::vector<HostTask>>& hostTasks,
                                             std::map<TaskId, std::vector<DeviceTask>>& deviceTasks,
                                             SyscntConversionParams& params)
{
    std::set<TaskId> totalTaskId;
    std::transform(hostTasks.begin(), hostTasks.end(), std::inserter(totalTaskId, totalTaskId.begin()),
                   [](const std::pair<TaskId, std::vector<HostTask>>& p) { return p.first; });
    for (auto& it : deviceTasks)
    {
        totalTaskId.insert(it.first);
    }
    std::vector<TopDownTask> res;
    size_t matchSize = 0;
    for (auto& key : totalTaskId)
    {
        auto hostIt = hostTasks.find(key);
        auto deviceIt = deviceTasks.find(key);
        if (hostIt != hostTasks.end() && deviceIt != deviceTasks.end())
        {
            if (hostIt->second.size() > 1 && deviceIt->second.size() > 1)
            {
                ERROR(
                    "There are % hostTask and % deviceTask, can't match streamId is: %, taskId is: %, ctxId is: %,"
                    "batchId is: %",
                    hostIt->second.size(), deviceIt->second.size(), key.streamId, key.taskId, key.contextId,
                    key.batchId);
                continue;
            }
            if (hostIt->second.size() == 1)
            {
                MergeByHostAndDeviceTask(res, hostIt->second[0], deviceIt->second, params);
                matchSize += deviceIt->second.size();
            }
            else
            {
                ERROR("Duplicate HostTask found, streamId is: %, taskId is: %, ctxId is: %, batchId is: %",
                      key.streamId, key.taskId, key.contextId, key.batchId);
            }
        }
        else if (hostIt == hostTasks.end())
        {
            MergeByOnlyDeviceTask(res, deviceIt->second, key, params);
        }
        else
        {  // 因为所有的key都是从hostTasks与deviceTasks中取的，所以一定不会有两者都找不到key的情况，该分支为device为空
            MergeByOnlyHostTask(res, hostIt->second);
        }
    }
    INFO("Found % host device matched task, generate % topDownTask", matchSize, res.size());
    return res;
}

void FillDeviceTaskStreamId(std::shared_ptr<StreamIdInfo> streamIdInfo,
                            std::shared_ptr<std::map<TaskId, std::vector<DeviceTask>>> deviceTasks)
{
    if (deviceTasks == nullptr || deviceTasks->empty())
    {
        WARN("There is no data in DeviceTask, fill task stream id failed.", deviceTasks);
        return;
    }
    for (auto& task : *deviceTasks)
    {
        auto streamIdPair = streamIdInfo->streamIdMap.find(task.first.taskId);
        if (streamIdPair != streamIdInfo->streamIdMap.end())
        {
            // If multiple threads are processing the stream ID, please ensure that the operations are atomic.
            task.first.streamId = streamIdPair->second;
        }
    }
}

uint32_t AscendTaskAssociation::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);
    auto params = GetSyscntConversionParams(deviceContext);
    auto hostTasks = dataInventory.GetPtr<std::map<TaskId, std::vector<HostTask>>>();
    auto deviceTasks = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    if (context.GetChipID() == CHIP_V6_1_0)
    {
        auto streamIdInfo = dataInventory.GetPtr<StreamIdInfo>();
        FillDeviceTaskStreamId(streamIdInfo, deviceTasks);
    }
    std::shared_ptr<std::vector<TopDownTask>> data;
    std::vector<TopDownTask> res;
    if (hostTasks->empty() && deviceTasks->empty())
    {
        WARN("No HostTasks and DeviceTasks, don't need to generate AscendTask!");
        MAKE_SHARED_RETURN_VALUE(data, std::vector<TopDownTask>, ANALYSIS_ERROR, std::move(res));
        dataInventory.Inject(data);
        return ANALYSIS_OK;
    }
    if (hostTasks->empty() || deviceTasks->empty())
    {
        WARN("There is no HostTask or DeviceTask, can't generate AscendTask!");
        return ANALYSIS_ERROR;
    }
    res = GenerateTopDownTask(*hostTasks, *deviceTasks, params);
    MAKE_SHARED_RETURN_VALUE(data, std::vector<TopDownTask>, ANALYSIS_ERROR, std::move(res));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

REGISTER_PROCESS_SEQUENCE(AscendTaskAssociation, true, LoadHostData);
REGISTER_PROCESS_DEPENDENT_DATA(AscendTaskAssociation, std::map<TaskId, std::vector<DeviceTask>>,
                                std::map<TaskId, std::vector<HostTask>>, StreamIdInfo);
REGISTER_PROCESS_SUPPORT_CHIP(AscendTaskAssociation, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
