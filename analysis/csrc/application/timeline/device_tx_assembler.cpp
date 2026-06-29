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

#include "analysis/csrc/application/timeline/device_tx_assembler.h"

#include "analysis/csrc/application/timeline/connection_id_pool.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Application;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

void DeviceTxTraceEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["Physic Stream Id"] << streamId_;
    ostream["Task Id"] << taskId_;
}

DeviceTxAssembler::DeviceTxAssembler()
    : JsonAssembler(PROCESS_MSPROFTX,
                    {{MSPROF_JSON_FILE, FileCategory::MSPROF}, {MSPROF_TX_FILE, FileCategory::MSPROF_TX}})
{
}

void DeviceTxAssembler::GenerateDeviceTxTrace(const std::vector<MsprofTxDeviceData> &txData,
                                              const std::string &profPath, const LayerInfo &layer,
                                              std::unordered_map<uint16_t, uint32_t> &pidMap)
{
    uint32_t formatPid;
    std::string traceName = NA;
    for (const auto &data : txData)
    {
        auto it = h2dMessage_.find(data.connectionId);
        if (it != h2dMessage_.end())
        {
            traceName = it->second;
        }
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, layer.sortIndex);
        int tid = static_cast<int>(data.streamId);
        // 存储pid，tid组合的最小集
        dPidTidSet_.insert({formatPid, tid});
        std::shared_ptr<DeviceTxTraceEvent> event;
        MAKE_SHARED_RETURN_VOID(
            event, DeviceTxTraceEvent, formatPid, tid, static_cast<double>(data.duration) / Analysis::Common::NS_TO_US,
            DivideByPowersOfTenWithPrecision(data.timestamp), traceName, data.streamId, data.taskId);
        res_.push_back(event);
        GenerateTxConnectionTrace(data, formatPid);
    }
}

void DeviceTxAssembler::GenerateTxConnectionTrace(const MsprofTxDeviceData &data, uint32_t formatPid)
{
    auto connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::MSPROF_TX);
    auto name = MS_TX;
    name.append("_").append(connId);
    int tid = static_cast<int>(data.streamId);
    std::shared_ptr<FlowEvent> end;
    MAKE_SHARED_RETURN_VOID(end, FlowEvent, formatPid, tid, DivideByPowersOfTenWithPrecision(data.timestamp), MS_TX,
                            connId, name, FLOW_END, FLOW_BP);
    res_.push_back(end);
}

uint8_t DeviceTxAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto deviceTxData = dataInventory.GetPtr<std::vector<MsprofTxDeviceData>>();
    if (deviceTxData == nullptr)
    {
        WARN("Can't get device_tx data from dataInventory");
        return DATA_NOT_EXIST;
    }
    auto txData = dataInventory.GetPtr<std::vector<MsprofTxHostData>>();
    if (txData != nullptr)
    {
        for (const auto &data : *txData)
        {
            if (data.connectionId != DEFAULT_CONNECTION_ID_MSTX)
            {  // tx ex数据才会有connectionId
                h2dMessage_.emplace(data.connectionId, data.message);
            }
        }
    }
    std::unordered_map<uint16_t, uint32_t> devicePid;
    auto deviceLayer = GetLayerInfo(PROCESS_TASK);
    GenerateDeviceTxTrace(*deviceTxData, profPath, deviceLayer, devicePid);
    GenerateTaskMetaData(devicePid, deviceLayer, res_, dPidTidSet_);
    if (res_.empty())
    {
        ERROR("Can't Generate any device tx process data");
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
