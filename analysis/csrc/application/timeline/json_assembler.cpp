/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include "analysis/csrc/application/timeline/json_assembler.h"

#include "analysis/csrc/application/deliverable_consumer.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain::Environment;
JsonAssembler::JsonAssembler(const std::string &name, std::unordered_map<std::string, FileCategory> &&files)
    : fileMap_(files), processorName_(name)
{
}

bool JsonAssembler::Run(DataInventory &dataInventory, const std::string &profPath)
{
    INFO("Begin to Assemble % data", processorName_);
    events_.Open(fileMap_, profPath);
    auto res = AssembleData(dataInventory, profPath);
    if (ASSEMBLE_SUCCESS == res)
    {
        events_.Flush();
        events_.Sync();
        if (!DeliverableConsumer::Instance().Ok())
        {
            ERROR("Assemble % data failed, can't export deliverable!", processorName_);
            return false;
        }
        INFO("Assemble % data success, begin to export", processorName_);
        return true;
    }
    events_.Discard();
    if (DATA_NOT_EXIST == res)
    {
        WARN("No % data, don't export deliverable!", processorName_);
        return true;
    }
    ERROR("Assemble % data failed, can't export deliverable!", processorName_);
    return false;
}

uint32_t JsonAssembler::GetFormatPid(uint32_t pid, uint32_t index, uint32_t deviceId)
{
    return (pid << HIGH_BIT_OFFSET) | (index << MIDDLE_BIT_OFFSET) | deviceId;
}

uint32_t JsonAssembler::GetDeviceIdFromPid(uint32_t pid) { return (pid & ((1 << MIDDLE_BIT_OFFSET) - 1)); }

uint32_t JsonAssembler::GetDisplaySortIndex(const struct LayerInfo &layerInfo)
{
    if (layerInfo.component == PROCESS_CPU_FREQUENCY)
    {
        return LAYER_CPU_USAGE_SORT + 1;
    }
    if (layerInfo.sortIndex >= LAYER_MEMORY_USAGE_SORT && layerInfo.sortIndex <= LAYER_FUSION_TASK)
    {
        return layerInfo.sortIndex + 1;
    }
    return layerInfo.sortIndex;
}

std::string JsonAssembler::GetLayerInfoLabelWithDeviceId(const std::string &label, uint32_t pid)
{
    uint32_t deviceId = GetDeviceIdFromPid(pid);
    return (deviceId == HOST_PID) ? label : label + " " + std::to_string(deviceId);
}

uint32_t JsonAssembler::GetDevicePid(std::unordered_map<uint16_t, uint32_t> &pidMap, uint16_t deviceId,
                                     const std::string &profPath, uint32_t index)
{
    auto it = pidMap.find(deviceId);
    if (it == pidMap.end())
    {
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        auto formatPid = JsonAssembler::GetFormatPid(pid, index, deviceId);
        pidMap[deviceId] = formatPid;
        return formatPid;
    }
    return it->second;
}

void JsonAssembler::GenerateHWMetaData(const std::unordered_map<uint16_t, uint32_t> &pidMap,
                                       const struct LayerInfo &layerInfo, JsonEventSink &res)
{
    std::shared_ptr<MetaDataNameEvent> processName;
    std::shared_ptr<MetaDataLabelEvent> processLabel;
    std::shared_ptr<MetaDataIndexEvent> processIndex;
    for (const auto &kv : pidMap)
    {
        MAKE_SHARED_RETURN_VOID(processName, MetaDataNameEvent, kv.second, DEFAULT_TID, META_DATA_PROCESS_NAME,
                                layerInfo.component);
        res.push_back(processName);

        MAKE_SHARED_RETURN_VOID(processLabel, MetaDataLabelEvent, kv.second, DEFAULT_TID, META_DATA_PROCESS_LABEL,
                                GetLayerInfoLabelWithDeviceId(layerInfo.label, kv.second));
        res.push_back(processLabel);

        MAKE_SHARED_RETURN_VOID(processIndex, MetaDataIndexEvent, kv.second, DEFAULT_TID, META_DATA_PROCESS_INDEX,
                                GetDisplaySortIndex(layerInfo));
        res.push_back(processIndex);
    }
}

void JsonAssembler::GenerateTaskMetaData(const std::unordered_map<uint16_t, uint32_t> &pidMap,
                                         const struct LayerInfo &layer, JsonEventSink &res,
                                         std::set<std::pair<uint32_t, uint32_t>> &pidTidSet)
{
    GenerateHWMetaData(pidMap, layer, res);

    std::shared_ptr<MetaDataNameEvent> threadName;
    std::shared_ptr<MetaDataIndexEvent> threadIndex;
    for (const auto &it : pidTidSet)
    {
        MAKE_SHARED_RETURN_VOID(threadName, MetaDataNameEvent, it.first, it.second, META_DATA_THREAD_NAME,
                                "Stream " + std::to_string(it.second));
        res.push_back(threadName);

        MAKE_SHARED_RETURN_VOID(threadIndex, MetaDataIndexEvent, it.first, it.second, META_DATA_THREAD_INDEX,
                                it.second);
        res.push_back(threadIndex);
    }
}
}  // namespace Application
}  // namespace Analysis
