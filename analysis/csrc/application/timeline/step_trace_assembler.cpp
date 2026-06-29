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

#include "analysis/csrc/application/timeline/step_trace_assembler.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Utils;
using namespace Analysis::Common;
namespace
{
}

void StepTraceEvent::ProcessArgs(Analysis::Infra::JsonWriter& ostream)
{
    ostream["Iteration ID"] << indexId_;
    ostream["FP Start"] << fpStart_;
    ostream["BP End"] << bpEnd_;
    ostream["Iteration End"] << iterEnd_;
    ostream["Iteration Time(us)"] << iterTime_;
}

void FpBpTraceEvent::ProcessArgs(Analysis::Infra::JsonWriter& ostream)
{
    ostream["Iteration ID"] << indexId_;
    ostream["FP Start"] << fpStart_;
    ostream["BP End"] << bpEnd_;
    ostream["FP_BP Time(us)"] << fpBpTime_;
}

void RefreshTraceEvent::ProcessArgs(Analysis::Infra::JsonWriter& ostream)
{
    ostream["Iteration ID"] << indexId_;
    ostream["BP End"] << bpEnd_;
    ostream["Iteration End"] << iterEnd_;
    ostream["Iteration Refresh(us)"] << refresh_;
}

void DataAug1TraceEvent::ToJson(Analysis::Infra::JsonWriter& ostream)
{
    TraceEvent::ToJson(ostream);
    ostream["ph"] << ph_;
    ostream["cat"] << cat_;
    ostream["id"] << id_;
    ostream["ts"] << ts_;
    ostream["args"];
    ostream.StartObject();
    ostream["Data_aug Bound(us)"] << arg_;
    ostream.EndObject();
}

void DataAug0TraceEvent::ToJson(Analysis::Infra::JsonWriter& ostream)
{
    TraceEvent::ToJson(ostream);
    ostream["ph"] << ph_;
    ostream["cat"] << cat_;
    ostream["id"] << id_;
    ostream["ts"] << ts_;
    ostream["args"];
    ostream.StartObject();
    ostream["Iteration ID"] << arg_;
    ostream.EndObject();
}

void ReduceTraceEvent::ProcessArgs(JsonWriter& ostream)
{
    ostream["Iteration ID"] << indexId_;
    for (const auto& it : args_)
    {
        ostream[it.first.c_str()] << it.second;
    }
}

void NextTraceEvent::ProcessArgs(Analysis::Infra::JsonWriter& ostream)
{
    ostream["GetNext Start"] << start_;
    ostream["GetNext End"] << end_;
    ostream["GetNext Time(us)"] << time_;
}

StepTraceAssembler::StepTraceAssembler()
    : JsonAssembler(PROCESS_STEP_TRACE,
                    {{MSPROF_JSON_FILE, FileCategory::MSPROF}, {STEP_TRACE_FILE, FileCategory::STEP}})
{
}

void StepTraceAssembler::GenerateTrainTrace(const std::vector<TrainTraceData>& trainData, const std::string& profPath,
                                            std::unordered_map<uint16_t, uint32_t>& pidMap, const LayerInfo& layer)
{
    uint32_t formatPid;
    std::string traceName;
    std::string fpStart;
    std::string bpEnd;
    for (const auto& data : trainData)
    {
        fpStart = data.fpStart == 0 ? NA : DivideByPowersOfTenWithPrecision(data.fpStart);
        bpEnd = data.bpEnd == 0 ? NA : DivideByPowersOfTenWithPrecision(data.bpEnd);
        traceName = "Iteration " + std::to_string(data.indexId);
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, layer.sortIndex);
        int tid = static_cast<int>(data.modelId) + SORT_INDEX_OFFSET;
        pidTidSet_.insert({formatPid, tid});
        std::shared_ptr<StepTraceEvent> event;
        MAKE_SHARED_RETURN_VOID(event, StepTraceEvent, formatPid, tid,
                                static_cast<double>(data.iterTime) / Analysis::Common::NS_TO_US,
                                DivideByPowersOfTenWithPrecision(data.timestamp), traceName, ITER_CAT, data.indexId,
                                static_cast<double>(data.iterTime) / Analysis::Common::NS_TO_US, fpStart, bpEnd,
                                DivideByPowersOfTenWithPrecision(data.iterEnd));
        res_.push_back(event);
        if (data.fpStart != 0 && data.bpEnd != 0)
        {
            GenerateFpBpTrace(data, formatPid, tid);
            GenerateRefreshTrace(data, formatPid, tid);
            GenerateDataAugTrace(data, formatPid, tid);
        }
    }
}

void StepTraceAssembler::GenerateFpBpTrace(const Analysis::Domain::TrainTraceData& data, uint32_t pid, int tid)
{
    std::shared_ptr<FpBpTraceEvent> fpEvent;
    std::string name = "FP_BP Time " + std::to_string(data.indexId);
    MAKE_SHARED_RETURN_VOID(
        fpEvent, FpBpTraceEvent, pid, tid, static_cast<double>(data.fpBpTime) / Analysis::Common::NS_TO_US,
        DivideByPowersOfTenWithPrecision(data.fpStart), name, FP_BP_CAT, data.indexId,
        static_cast<double>(data.fpBpTime) / Analysis::Common::NS_TO_US, DivideByPowersOfTenWithPrecision(data.fpStart),
        DivideByPowersOfTenWithPrecision(data.bpEnd));
    res_.push_back(fpEvent);
}

void StepTraceAssembler::GenerateRefreshTrace(const Analysis::Domain::TrainTraceData& data, uint32_t pid, int tid)
{
    std::shared_ptr<RefreshTraceEvent> refreshEvent;
    std::string name = "Iteration Refresh " + std::to_string(data.indexId);
    MAKE_SHARED_RETURN_VOID(refreshEvent, RefreshTraceEvent, pid, tid,
                            static_cast<double>(data.gradRefreshBound) / Analysis::Common::NS_TO_US,
                            DivideByPowersOfTenWithPrecision(data.bpEnd), name, REFRESH_CAT, data.indexId,
                            static_cast<double>(data.gradRefreshBound) / Analysis::Common::NS_TO_US,
                            DivideByPowersOfTenWithPrecision(data.bpEnd),
                            DivideByPowersOfTenWithPrecision(data.iterEnd));
    res_.push_back(refreshEvent);
}

void StepTraceAssembler::GenerateDataAugTrace(const Analysis::Domain::TrainTraceData& data, uint32_t pid, int tid)
{
    std::string id = "Data_aug Bound ";
    id.append(std::to_string(pid)).append("_").append(std::to_string(tid));
    std::string name = "Data_aug Bound " + std::to_string(data.indexId - 1);
    std::shared_ptr<DataAug0TraceEvent> aug0Event;
    MAKE_SHARED_RETURN_VOID(aug0Event, DataAug0TraceEvent, pid, tid, DivideByPowersOfTenWithPrecision(data.fpStart),
                            name, DATA_AUG, id, data.indexId);
    res_.push_back(aug0Event);
    uint64_t ts = data.bpEnd + data.dataAugBound / 2;
    std::shared_ptr<DataAug1TraceEvent> aug1Event;
    name = "Data_aug Bound " + std::to_string(data.indexId);
    MAKE_SHARED_RETURN_VOID(aug1Event, DataAug1TraceEvent, pid, tid, DivideByPowersOfTenWithPrecision(ts), name,
                            DATA_AUG, id, static_cast<double>(data.dataAugBound) / Analysis::Common::NS_TO_US);
    res_.push_back(aug1Event);
}

void StepTraceAssembler::GenerateMetaData(std::unordered_map<uint16_t, uint32_t>& pidMap, const LayerInfo& layer)
{
    for (const auto& it : pidMap)
    {
        std::shared_ptr<MetaDataNameEvent> processName;
        MAKE_SHARED_RETURN_VOID(processName, MetaDataNameEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_NAME,
                                layer.component);
        res_.push_back(processName);
        std::shared_ptr<MetaDataLabelEvent> processLabel;
        MAKE_SHARED_RETURN_VOID(processLabel, MetaDataLabelEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_LABEL,
                                GetLayerInfoLabelWithDeviceId(layer.label, it.second));
        res_.push_back(processLabel);
        std::shared_ptr<MetaDataIndexEvent> processIndex;
        MAKE_SHARED_RETURN_VOID(processIndex, MetaDataIndexEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_INDEX,
                                layer.sortIndex);
        res_.push_back(processIndex);
    }
    for (const auto& it : pidTidSet_)
    {
        std::string argName = std::string("Step Trace(Model ID:")
                                  .append(std::to_string(static_cast<uint32_t>(it.second - SORT_INDEX_OFFSET)))
                                  .append(")");
        std::shared_ptr<MetaDataNameEvent> threadName;
        MAKE_SHARED_RETURN_VOID(threadName, MetaDataNameEvent, it.first, it.second, META_DATA_THREAD_NAME, argName);
        res_.push_back(threadName);
        std::shared_ptr<MetaDataIndexEvent> threadIndex;
        MAKE_SHARED_RETURN_VOID(threadIndex, MetaDataIndexEvent, it.first, it.second, META_DATA_THREAD_INDEX,
                                it.second);
        res_.push_back(threadIndex);
    }
}

void StepTraceAssembler::GenerateReduceTrace(const std::vector<AllReduceData>& reduceData, const std::string& profPath,
                                             std::unordered_map<uint16_t, uint32_t>& pidMap,
                                             const Analysis::Domain::LayerInfo& layer)
{
    std::string traceName;
    uint32_t formatPid;
    std::size_t count = 0;
    std::string start;
    std::string end;
    for (const auto& data : reduceData)
    {
        traceName = REDUCE_CAT;
        traceName.append("_").append(std::to_string(data.indexId)).append("_").append(std::to_string(count));
        std::shared_ptr<ReduceTraceEvent> reduceEvent;
        std::unordered_map<std::string, std::string> args;
        start = "Reduce Start " + std::to_string(count);
        end = "Reduce End " + std::to_string(count);
        args.emplace(start, std::to_string(data.timestamp));
        args.emplace(end, std::to_string(data.end));
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, layer.sortIndex);
        int tid = static_cast<int>(data.modelId) + SORT_INDEX_OFFSET;
        MAKE_SHARED_RETURN_VOID(reduceEvent, ReduceTraceEvent, formatPid, tid,
                                static_cast<double>(data.end - data.timestamp) / Analysis::Common::NS_TO_US,
                                DivideByPowersOfTenWithPrecision(data.timestamp), traceName, REDUCE_CAT, data.indexId,
                                args);
        res_.push_back(reduceEvent);
        ++count;
    }
}

void StepTraceAssembler::GenerateNextTrace(const std::vector<GetNextData>& nextData, const std::string& profPath,
                                           std::unordered_map<uint16_t, uint32_t>& pidMap,
                                           const Analysis::Domain::LayerInfo& layer)
{
    std::string traceName = "GetNext";
    uint32_t formatPid;
    for (const auto& data : nextData)
    {
        if (data.timestamp == 0 || data.end == 0)
        {
            continue;
        }
        std::shared_ptr<NextTraceEvent> event;
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, layer.sortIndex);
        int tid = static_cast<int>(data.modelId) + SORT_INDEX_OFFSET;
        MAKE_SHARED_RETURN_VOID(event, NextTraceEvent, formatPid, tid,
                                static_cast<double>(data.end - data.timestamp) / Analysis::Common::NS_TO_US,
                                DivideByPowersOfTenWithPrecision(data.timestamp), traceName, GET_NEXT_CAT,
                                DivideByPowersOfTenWithPrecision(data.timestamp),
                                DivideByPowersOfTenWithPrecision(data.end),
                                static_cast<double>(data.end - data.timestamp) / Analysis::Common::NS_TO_US);
        res_.push_back(event);
    }
}

uint8_t StepTraceAssembler::AssembleData(DataInventory& dataInventory, JsonWriter& ostream, const std::string& profPath)
{
    auto trainData = dataInventory.GetPtr<std::vector<TrainTraceData>>();
    auto reduceData = dataInventory.GetPtr<std::vector<AllReduceData>>();
    auto nextData = dataInventory.GetPtr<std::vector<GetNextData>>();
    if (trainData == nullptr && reduceData == nullptr && nextData == nullptr)
    {
        WARN("Can't get step trace data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> devicePid;
    auto layer = GetLayerInfo(PROCESS_STEP_TRACE);
    if (trainData != nullptr)
    {
        GenerateTrainTrace(*trainData, profPath, devicePid, layer);
        GenerateMetaData(devicePid, layer);
    }
    if (reduceData != nullptr)
    {
        GenerateReduceTrace(*reduceData, profPath, devicePid, layer);
    }
    if (nextData != nullptr)
    {
        GenerateNextTrace(*nextData, profPath, devicePid, layer);
    }
    if (res_.empty())
    {
        ERROR("Can't Generate any Ascend process data");
        return ASSEMBLE_FAILED;
    }
    for (const auto& node : res_)
    {
        node->DumpJson(ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
