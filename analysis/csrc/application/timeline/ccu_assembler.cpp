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

#include "analysis/csrc/application/timeline/ccu_assembler.h"

#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Utils;

void CCUMissionTraceEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["Physic Stream Id"] << data_.streamId;
    ostream["Task Id"] << data_.taskId;
    if (data_.timeType == CCU_TIME_TYPE_WAIT)
    {
        ostream["Notify Instruction ID"] << data_.instructionId;
        ostream["Notify Rank ID"] << data_.notifyRankId;
    }
    else
    {
        ostream["Instruction ID"] << data_.instructionId;
    }
    if (data_.hasDieId)
    {
        ostream["Die Id"] << data_.dieId;
    }
    if (data_.hasDataSize)
    {
        ostream["Data Size"] << data_.dataSize;
    }
    if (data_.hasBandwidth)
    {
        ostream["Bandwidth (MB/s)"] << data_.bandwidth;
    }
    if (data_.hasReduceInfo)
    {
        ostream["Reduce Op Type"] << data_.reduceOpType;
        ostream["Input Data Type"] << data_.inputDataType;
        ostream["Output Data Type"] << data_.outputDataType;
    }
    if (data_.hasMask)
    {
        ostream["Mask"] << data_.mask;
    }
    if (data_.hasDelayChannel)
    {
        ostream["Maximum Delay Channel"] << data_.maxDelayChannel;
        ostream["Maximum Channel Delay"] << data_.maxChannelDelay;
    }
}

CCUAssembler::CCUAssembler() : JsonAssembler(PROCESSOR_NAME_CCU_MISSION, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void CCUAssembler::GenerateMetaData(std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layer)
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
                                PROCESS_HCCL);
        res_.emplace_back(std::move(threadName));
        std::shared_ptr<MetaDataIndexEvent> threadIndex;
        MAKE_SHARED_RETURN_VOID(threadIndex, MetaDataIndexEvent, it.second, DEFAULT_TID, META_DATA_THREAD_INDEX,
                                DEFAULT_TID);
        res_.emplace_back(std::move(threadIndex));
    }
}

uint8_t CCUAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto ccuData = dataInventory.GetPtr<std::vector<CCUMissionTimelineData>>();
    if (ccuData == nullptr)
    {
        WARN("Can't get ccu mission data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> devicePid;
    auto layer = GetLayerInfo(PROCESS_CCU);
    for (const auto &data : *ccuData)
    {
        auto pid = GetDevicePid(devicePid, data.deviceId, profPath, layer.sortIndex);
        std::shared_ptr<CCUMissionTraceEvent> event;
        MAKE_SHARED_RETURN_VALUE(event, CCUMissionTraceEvent, ASSEMBLE_FAILED, pid, DEFAULT_TID,
                                 static_cast<double>(data.duration) / Analysis::Common::NS_TO_US,
                                 DivideByPowersOfTenWithPrecision(data.timestamp), data);
        res_.emplace_back(std::move(event));
    }
    if (res_.empty())
    {
        WARN("No ccu timeline events are generated.");
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
