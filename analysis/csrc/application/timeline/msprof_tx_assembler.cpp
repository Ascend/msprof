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

#include "analysis/csrc/application/timeline/msprof_tx_assembler.h"

#include "analysis/csrc/application/timeline/connection_id_pool.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Application;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

std::string GetEventTypeStr(uint16_t eventType)
{
    for (const auto& node : MSTX_EVENT_TYPE_TABLE)
    {
        if (node.second == eventType)
        {
            return node.first;
        }
    }
    return "";
}

void MsprofTxTraceEvent::ProcessArgs(JsonWriter& ostream)
{
    ostream["Category"] << std::to_string(category_);
    ostream["Payload_type"] << payloadType_;
    ostream["Payload_value"] << payloadValue_;
    ostream["Message_type"] << messageType_;
    ostream["event_type"] << eventType_;
}

void MsprofTxExTraceEvent::ProcessArgs(JsonWriter& ostream)
{
    ostream["event_type"] << eventType_;
    ostream["domain"] << domain_;
}

MsprofTxAssembler::MsprofTxAssembler()
    : JsonAssembler(PROCESS_MSPROFTX,
                    {{MSPROF_JSON_FILE, FileCategory::MSPROF}, {MSPROF_TX_FILE, FileCategory::MSPROF_TX}})
{
}

void MsprofTxAssembler::GenerateTxExConnectionTrace(const MsprofTxHostData& data, uint32_t pid)
{
    auto connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::MSPROF_TX);
    auto name = MS_TX;
    name.append("_").append(connId);
    std::shared_ptr<FlowEvent> start;
    MAKE_SHARED_RETURN_VOID(start, FlowEvent, pid, data.tid, DivideByPowersOfTenWithPrecision(data.timestamp), MS_TX,
                            connId, name, FLOW_START);
    res_.push_back(start);
}

void MsprofTxAssembler::GenerateTxTrace(const std::vector<MsprofTxHostData>& txData, uint32_t pid)
{
    std::string eventTypeStr;
    for (const auto& data : txData)
    {
        eventTypeStr = GetEventTypeStr(data.eventType);
        hPidTidSet_.insert({pid, data.tid});
        if (data.connectionId == DEFAULT_CONNECTION_ID_MSTX)
        {  // tx数据没有connectionId
            std::shared_ptr<MsprofTxTraceEvent> tx;
            MAKE_SHARED_RETURN_VOID(tx, MsprofTxTraceEvent, pid, data.tid,
                                    static_cast<double>(data.end - data.timestamp) / Analysis::Common::NS_TO_US,
                                    DivideByPowersOfTenWithPrecision(data.timestamp), data.message, data.category,
                                    data.payloadType, data.messageType, data.payloadValue, eventTypeStr);
            res_.push_back(tx);
        }
        else
        {  // tx ex数据有markId字段可以作为connectionId
            std::shared_ptr<MsprofTxExTraceEvent> txEx;
            MAKE_SHARED_RETURN_VOID(txEx, MsprofTxExTraceEvent, pid, data.tid,
                                    static_cast<double>(data.end - data.timestamp) / Analysis::Common::NS_TO_US,
                                    DivideByPowersOfTenWithPrecision(data.timestamp), data.message, eventTypeStr,
                                    data.domain);
            res_.push_back(txEx);
            GenerateTxExConnectionTrace(data, pid);
        }
    }
}

void MsprofTxAssembler::GenerateHMetaDataEvent(const LayerInfo& layer, uint32_t pid)
{
    std::shared_ptr<MetaDataNameEvent> processName;
    MAKE_SHARED_RETURN_VOID(processName, MetaDataNameEvent, pid, DEFAULT_TID, META_DATA_PROCESS_NAME, layer.component);
    res_.push_back(processName);
    std::shared_ptr<MetaDataLabelEvent> processLabel;
    MAKE_SHARED_RETURN_VOID(processLabel, MetaDataLabelEvent, pid, DEFAULT_TID, META_DATA_PROCESS_LABEL,
                            GetLayerInfoLabelWithDeviceId(layer.label, pid));
    res_.push_back(processLabel);
    std::shared_ptr<MetaDataIndexEvent> proIndex;
    MAKE_SHARED_RETURN_VOID(proIndex, MetaDataIndexEvent, pid, DEFAULT_TID, META_DATA_PROCESS_INDEX, layer.sortIndex);
    res_.push_back(proIndex);
    std::string thName;
    for (const auto& it : hPidTidSet_)
    {
        thName = "Thread " + std::to_string(it.second);
        std::shared_ptr<MetaDataNameEvent> threadName;
        MAKE_SHARED_RETURN_VOID(threadName, MetaDataNameEvent, it.first, it.second, META_DATA_THREAD_NAME, thName);
        res_.push_back(threadName);
        std::shared_ptr<MetaDataIndexEvent> threadIndex;
        MAKE_SHARED_RETURN_VOID(threadIndex, MetaDataIndexEvent, it.first, it.second, META_DATA_THREAD_INDEX,
                                it.second);
        res_.push_back(threadIndex);
    }
}

uint8_t MsprofTxAssembler::AssembleData(DataInventory& dataInventory, JsonWriter& ostream, const std::string& profPath)
{
    auto txData = dataInventory.GetPtr<std::vector<MsprofTxHostData>>();
    if (txData == nullptr)
    {
        WARN("Can't get msprof_host_tx data from dataInventory");
        return DATA_NOT_EXIST;
    }
    auto hostLayer = GetLayerInfo(PROCESS_MSPROFTX);
    auto traceName = Context::GetInstance().GetPidNameFromInfoJson(HOST_ID, profPath);
    hostLayer.component = traceName;
    auto pid = Context::GetInstance().GetPidFromInfoJson(HOST_ID, profPath);
    auto formatPid = JsonAssembler::GetFormatPid(pid, hostLayer.sortIndex);
    GenerateTxTrace(*txData, formatPid);
    GenerateHMetaDataEvent(hostLayer, formatPid);
    if (res_.empty())
    {
        ERROR("Can't Generate any msprof tx process data");
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
