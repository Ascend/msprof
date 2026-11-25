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

#include "analysis/csrc/application/timeline/ccu_assembler.h"

#include <bitset>

#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Utils;

namespace
{
void WriteInstructionArgs(JsonWriter &ostream, const Domain::CCUInstructionArgs &data)
{
    if (data.hasDieId)
    {
        ostream["Die Id"] << data.dieId;
    }
    if (data.hasDataSize)
    {
        ostream["Data Size"] << data.dataSize;
    }
    if (data.hasBandwidth)
    {
        ostream["Bandwidth (MB/s)"] << data.bandwidth;
    }
    if (data.hasReduceInfo)
    {
        ostream["Reduce Op Type"] << data.reduceOpType;
        ostream["Input Data Type"] << data.inputDataType;
        ostream["Output Data Type"] << data.outputDataType;
    }
    if (data.hasMask)
    {
        ostream["Mask"] << data.mask;
    }
    if (data.hasDelayChannel)
    {
        ostream["Maximum Delay Channel"] << data.maxDelayChannel;
        ostream["Maximum Channel Delay"] << data.maxChannelDelay;
    }
}
}  // namespace

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
    WriteInstructionArgs(ostream, data_);
}

std::string CCUV62TraceEvent::GetEventName(const CCUV62TimelineData &data)
{
    if (data.timeType == CCU_TIME_TYPE_SETCKB)
    {
        return data.lpFlg == 1 ? "SetCKB Loop" : "SetCKB Mission";
    }
    return data.timeType;
}

void CCUV62TraceEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["Task Id"] << data_.taskId;
    ostream["Instruction ID"] << data_.instructionId;
    WriteInstructionArgs(ostream, data_);
    if (data_.timeType != CCU_TIME_TYPE_SETCKB)
    {
        return;
    }
    ostream["Notify Instruction ID"] << data_.instructionId;
    if (data_.lpFlg == 0)
    {
        ostream["CKB Info"] << std::bitset<16>(static_cast<uint16_t>(data_.setckbCkbinfo)).to_string();
        if (data_.hasUdf)
        {
            ostream["UDF Profiling Data"] << data_.udfProfilingData;
        }
    }
    else if (data_.lpFlg == 1)
    {
        ostream["Earliest Loop Id"] << data_.earliestLoopId;
        ostream["Latest Loop Id"] << data_.latestLoopId;
        ostream["Longest Duration Loop Id"] << data_.longestLoopId;
        ostream["Longest Duration(us)"] << data_.longestDuration / Analysis::Common::NS_TO_US;
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
        events_.emplace_back(std::move(processName));
        std::shared_ptr<MetaDataLabelEvent> processLabel;
        MAKE_SHARED_RETURN_VOID(processLabel, MetaDataLabelEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_LABEL,
                                GetLayerInfoLabelWithDeviceId(layer.label, it.second));
        events_.emplace_back(std::move(processLabel));
        std::shared_ptr<MetaDataIndexEvent> processIndex;
        MAKE_SHARED_RETURN_VOID(processIndex, MetaDataIndexEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_INDEX,
                                layer.sortIndex);
        events_.emplace_back(std::move(processIndex));

        std::shared_ptr<MetaDataNameEvent> threadName;
        MAKE_SHARED_RETURN_VOID(threadName, MetaDataNameEvent, it.second, DEFAULT_TID, META_DATA_THREAD_NAME,
                                PROCESS_HCCL);
        events_.emplace_back(std::move(threadName));
        std::shared_ptr<MetaDataIndexEvent> threadIndex;
        MAKE_SHARED_RETURN_VOID(threadIndex, MetaDataIndexEvent, it.second, DEFAULT_TID, META_DATA_THREAD_INDEX,
                                DEFAULT_TID);
        events_.emplace_back(std::move(threadIndex));
    }
}

uint8_t CCUAssembler::AssembleData(DataInventory &dataInventory, const std::string &profPath)
{
    auto ccuData = dataInventory.GetPtr<std::vector<CCUMissionTimelineData>>();
    auto ccuDataV62 = dataInventory.GetPtr<std::vector<CCUV62TimelineData>>();
    if (ccuData == nullptr && ccuDataV62 == nullptr)
    {
        WARN("Can't get ccu mission data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> devicePid;
    auto layer = GetLayerInfo(PROCESS_CCU);
    if (ccuData != nullptr)
    {
        for (const auto &data : *ccuData)
        {
            auto pid = GetDevicePid(devicePid, data.deviceId, profPath, layer.sortIndex);
            std::shared_ptr<CCUMissionTraceEvent> event;
            MAKE_SHARED_RETURN_VALUE(event, CCUMissionTraceEvent, ASSEMBLE_FAILED, pid, DEFAULT_TID,
                                     static_cast<double>(data.duration) / Analysis::Common::NS_TO_US,
                                     DivideByPowersOfTenWithPrecision(data.timestamp), data);
            events_.emplace_back(std::move(event));
        }
    }
    if (ccuDataV62 != nullptr)
    {
        for (const auto &data : *ccuDataV62)
        {
            auto pid = GetDevicePid(devicePid, data.deviceId, profPath, layer.sortIndex);
            std::shared_ptr<CCUV62TraceEvent> event;
            MAKE_SHARED_RETURN_VALUE(event, CCUV62TraceEvent, ASSEMBLE_FAILED, pid, DEFAULT_TID,
                                     static_cast<double>(data.duration) / Analysis::Common::NS_TO_US,
                                     DivideByPowersOfTenWithPrecision(data.timestamp), data);
            events_.emplace_back(std::move(event));
        }
    }
    if (events_.empty())
    {
        WARN("No ccu timeline events are generated.");
        return DATA_NOT_EXIST;
    }
    GenerateMetaData(devicePid, layer);
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
