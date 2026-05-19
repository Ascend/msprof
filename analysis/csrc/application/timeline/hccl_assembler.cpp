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

#include "analysis/csrc/application/timeline/hccl_assembler.h"

namespace Analysis
{
namespace Application
{

std::string HcclAssembler::TransEnumToType(uint64_t key, const std::unordered_map<std::string, uint16_t> &enumTable)
{
    for (const auto &node : enumTable)
    {
        if (node.second == key)
        {
            return node.first;
        }
    }
    return "";
}

HcclAssembler::HcclAssembler() : JsonAssembler(PROCESS_HCCL, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void HcclOpTraceEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["rank_size"] << rankSize_;
    ostream["connection_id"] << connectionId_;
    ostream["model id"] << modelId_;
    ostream["data_type"] << dataType_;
    ostream["alg_type"] << algType_;
    ostream["count"] << count_;
    ostream["relay"] << relay_;
    ostream["retry"] << retry_;
}

void HcclTaskTraceEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["notify_id"] << notifyId_;
    ostream["duration estimated(us)"] << esDur_;
    ostream["stream id"] << streamId_;
    ostream["task id"] << taskId_;
    ostream["context id"] << contextId_;
    ostream["task type"] << taskType;
    ostream["src rank"] << srcRank_;
    ostream["dst rank"] << dstRank_;
    ostream["transport type"] << transportType_;
    ostream["size(Byte)"] << size_;
    ostream["data type"] << dataType_;
    ostream["link type"] << linkType_;
    ostream["bandwidth(GB/s)"] << bandwidth_;
    ostream["model id"] << modelId_;
}

void HcclAssembler::GenerateMetaDataEvent(std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layerInfo,
                                          const std::string &profPath)
{
    uint32_t formatPid;
    int32_t index = 0;
    for (auto &it : groupIndex_)
    {
        formatPid = GetDevicePid(pidMap, it.first, profPath, layerInfo.sortIndex);
        std::shared_ptr<MetaDataNameEvent> processName;
        MAKE_SHARED_RETURN_VOID(processName, MetaDataNameEvent, formatPid, DEFAULT_TID, META_DATA_PROCESS_NAME,
                                layerInfo.component);
        res_.push_back(processName);
        std::shared_ptr<MetaDataLabelEvent> processLabel;
        MAKE_SHARED_RETURN_VOID(processLabel, MetaDataLabelEvent, formatPid, DEFAULT_TID, META_DATA_PROCESS_LABEL,
                                GetLayerInfoLabelWithDeviceId(layerInfo.label, formatPid));
        res_.push_back(processLabel);
        std::shared_ptr<MetaDataIndexEvent> processIndex;
        MAKE_SHARED_RETURN_VOID(processIndex, MetaDataIndexEvent, formatPid, DEFAULT_TID, META_DATA_PROCESS_INDEX,
                                layerInfo.sortIndex);
        res_.push_back(processIndex);
        for (auto &groupIt : it.second)
        {
            GenerateTMetaDataEvent(groupIt.second, index, formatPid);
        }
    }
}

void HcclAssembler::GenerateTMetaDataEvent(std::vector<HcclGroup> &groupInfo, int32_t &index, uint32_t formatPid)
{
    std::string traceName;
    std::string traceNameSuffix = "Communication";
    for (auto &group : groupInfo)
    {
        auto startIndex = index;
        traceName = group.groupName != NA ? ("Group " + group.groupName + " " + traceNameSuffix) : traceNameSuffix;
        group.startIndex = index;
        std::shared_ptr<MetaDataNameEvent> threadName;
        MAKE_SHARED_RETURN_VOID(threadName, MetaDataNameEvent, formatPid, index, META_DATA_THREAD_NAME, traceName);
        res_.push_back(threadName);
        std::shared_ptr<MetaDataIndexEvent> threadIndex;
        MAKE_SHARED_RETURN_VOID(threadIndex, MetaDataIndexEvent, formatPid, index, META_DATA_THREAD_INDEX, index);
        res_.push_back(threadIndex);
        for (const auto &plane : group.planes)
        {
            traceName = {"Plane " + std::to_string(plane)};
            index = startIndex + plane + 1;
            std::shared_ptr<MetaDataNameEvent> pThreadName;
            MAKE_SHARED_RETURN_VOID(pThreadName, MetaDataNameEvent, formatPid, index, META_DATA_THREAD_NAME, traceName);
            res_.push_back(pThreadName);
            std::shared_ptr<MetaDataIndexEvent> pThreadIndex;
            MAKE_SHARED_RETURN_VOID(pThreadIndex, MetaDataIndexEvent, formatPid, index, META_DATA_THREAD_INDEX, index);
            res_.push_back(pThreadIndex);
        }
        index++;
    }
}

int32_t HcclAssembler::GetTid(const std::string groupName, const uint16_t deviceId, const HcclType &type)
{
    int32_t tid = -1;
    auto it = groupIndex_.find(deviceId);
    if (it != groupIndex_.end())
    {
        auto groupIt = it->second.find(groupName);
        if (groupIt != it->second.end())
        {
            auto tmp = std::find_if(groupIt->second.begin(), groupIt->second.end(),
                                    [&type](const HcclGroup &op) { return op.type == type; });
            if (tmp != groupIt->second.end())
            {
                tid = tmp->startIndex;
            }
        }
    }
    return tid;
}

std::unordered_map<uint16_t, std::unordered_map<std::string, std::vector<HcclGroup>>> HcclAssembler::InitHcclGroup(
    std::shared_ptr<std::vector<CommunicationTaskData>> &hcclData, std::shared_ptr<std::vector<KfcTaskData>> &kfcData)
{
    std::unordered_map<uint16_t, std::unordered_map<std::string, std::vector<HcclGroup>>> groupTable;
    std::unordered_map<uint16_t, std::unordered_map<std::string, std::set<int32_t>>> planesTable;
    int32_t plainId;
    if (hcclData != nullptr)
    {
        for (const auto &it : *hcclData)
        {
            plainId = it.planeId == INVALID_PLANE ? 0 : it.planeId;
            planesTable[it.deviceId][it.groupName].emplace(plainId);
        }
    }
    for (const auto &item : planesTable)
    {
        for (const auto &groupInfo : item.second)
        {
            groupTable[item.first][groupInfo.first].emplace_back(groupInfo.first, HcclType::HCCL, groupInfo.second);
        }
    }
    planesTable.clear();
    if (kfcData != nullptr)
    {
        for (const auto &it : *kfcData)
        {
            plainId = it.planeId == INVALID_PLANE ? 0 : it.planeId;
            planesTable[it.deviceId][it.groupName].emplace(plainId);
        }
    }
    for (const auto &item : planesTable)
    {
        for (const auto &groupInfo : item.second)
        {
            groupTable[item.first][groupInfo.first].emplace_back(groupInfo.first, HcclType::MC2, groupInfo.second);
        }
    }
    return groupTable;
}

uint8_t HcclAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto taskData = dataInventory.GetPtr<std::vector<CommunicationTaskData>>();
    auto opData = dataInventory.GetPtr<std::vector<CommunicationOpData>>();
    auto kfcTask = dataInventory.GetPtr<std::vector<KfcTaskData>>();
    auto kfcOp = dataInventory.GetPtr<std::vector<KfcOpData>>();
    if (taskData == nullptr && opData == nullptr && kfcTask == nullptr && kfcOp == nullptr)
    {
        WARN("Can't get hccl(kfc) task data and hccl(kfc) op data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> devicePid;
    auto layerInfo = GetLayerInfo(PROCESS_HCCL);
    groupIndex_ = InitHcclGroup(taskData, kfcTask);
    GenerateMetaDataEvent(devicePid, layerInfo, profPath);
    if (taskData != nullptr)
    {
        GenerateCommTaskTrace<CommunicationTaskData>(*taskData, profPath, HcclType::HCCL, devicePid, layerInfo);
    }
    if (opData != nullptr)
    {
        GenerateCommOpTrace<CommunicationOpData>(*opData, profPath, HcclType::HCCL, devicePid, layerInfo);
    }
    if (kfcTask != nullptr)
    {
        GenerateCommTaskTrace<KfcTaskData>(*kfcTask, profPath, HcclType::MC2, devicePid, layerInfo);
    }
    if (kfcOp != nullptr)
    {
        GenerateCommOpTrace<KfcOpData>(*kfcOp, profPath, HcclType::MC2, devicePid, layerInfo);
    }
    if (res_.empty())
    {
        ERROR("Can't Generate any Ascend process data");
        return ASSEMBLE_FAILED;
    }
    for (const auto &node : res_)
    {
        node->DumpJson(ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
