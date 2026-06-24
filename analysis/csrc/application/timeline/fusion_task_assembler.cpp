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

#include "analysis/csrc/application/timeline/fusion_task_assembler.h"

#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Utils;

void FusionTaskTraceEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["Task Id"] << data_.taskId;
    ostream["Acc Id"] << data_.accId;
    ostream["Task Type"] << data_.taskType;
    ostream["Fusion Task Type"] << data_.fusionTaskType;
    if (data_.isCcu)
    {
        ostream["Mission Id"] << data_.missionId;
        if (data_.hasCcuDie)
        {
            ostream["Ccu Die Id"] << data_.ccuDieId;
        }
    }
}

FusionTaskAssembler::FusionTaskAssembler()
    : JsonAssembler(PROCESSOR_NAME_FUSION_TASK, {{MSPROF_JSON_FILE, FileCategory::MSPROF}})
{
}

void FusionTaskAssembler::GenerateMetaData(std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layer)
{
    for (const auto &it : pidMap)
    {
        std::shared_ptr<MetaDataNameEvent> processName;
        MAKE_SHARED_RETURN_VOID(processName, MetaDataNameEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_NAME,
                                layer.component);
        res_.emplace_back(std::move(processName));
        std::shared_ptr<MetaDataLabelEvent> processLabel;
        MAKE_SHARED_RETURN_VOID(processLabel, MetaDataLabelEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_LABEL,
                                GetLayerInfoLabelWithDeviceId(layer.label, it.second));
        res_.emplace_back(std::move(processLabel));
        std::shared_ptr<MetaDataIndexEvent> processIndex;
        MAKE_SHARED_RETURN_VOID(processIndex, MetaDataIndexEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_INDEX,
                                layer.sortIndex);
        res_.emplace_back(std::move(processIndex));

        std::shared_ptr<MetaDataNameEvent> threadName;
        MAKE_SHARED_RETURN_VOID(threadName, MetaDataNameEvent, it.second, DEFAULT_TID, META_DATA_THREAD_NAME,
                                PROCESS_FUSION_TASK);
        res_.emplace_back(std::move(threadName));
        std::shared_ptr<MetaDataIndexEvent> threadIndex;
        MAKE_SHARED_RETURN_VOID(threadIndex, MetaDataIndexEvent, it.second, DEFAULT_TID, META_DATA_THREAD_INDEX,
                                DEFAULT_TID);
        res_.emplace_back(std::move(threadIndex));
    }
}

uint8_t FusionTaskAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream,
                                          const std::string &profPath)
{
    auto fusionData = dataInventory.GetPtr<std::vector<FusionTaskTimelineData>>();
    if (fusionData == nullptr)
    {
        WARN("Can't get fusion task data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> devicePid;
    auto layer = GetLayerInfo(PROCESS_FUSION_TASK);
    for (const auto &data : *fusionData)
    {
        auto pid = GetDevicePid(devicePid, data.deviceId, profPath, layer.sortIndex);
        std::shared_ptr<FusionTaskTraceEvent> event;
        MAKE_SHARED_RETURN_VALUE(event, FusionTaskTraceEvent, ASSEMBLE_FAILED, pid, static_cast<int>(data.streamId),
                                 data.duration / NS_TO_US, DivideByPowersOfTenWithPrecision(data.startTime), data);
        res_.emplace_back(std::move(event));
    }
    if (res_.empty())
    {
        WARN("No fusion task timeline events are generated.");
        return DATA_NOT_EXIST;
    }
    GenerateMetaData(devicePid, layer);
    for (const auto &node : res_)
    {
        node->DumpJson(ostream);
    }
    ostream << ",";
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
