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
#include "analysis/csrc/application/timeline/sys_io_assembler.h"

#include "analysis/csrc/domain/entities/viewer_data/system/include/sys_io_data.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Application;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
namespace
{
const std::string RX_COUNTER = "Rx";
const std::string TX_COUNTER = "Tx";
const std::string RX_DROPPED_RATE = "Rx Dropped Rate";
const std::string RX_ERROR_RATE = "Rx Error Rate";
const std::string RX_PACKET_RATE = "Rx Packets";
const std::string RX_BANDWIDTH_EFFICIENCY = "Rx Bandwidth Efficiency";
const std::string TX_DROPPED_RATE = "Tx Dropped Rate";
const std::string TX_ERROR_RATE = "Tx Error Rate";
const std::string TX_PACKET_RATE = "Tx Packets";
const std::string TX_BANDWIDTH_EFFICIENCY = "Tx Bandwidth Efficiency";
}  // namespace

SysIOAssembler::SysIOAssembler(const std::string &processorName)
    : JsonAssembler(processorName, {{MSPROF_JSON_FILE, FileCategory::MSPROF}})
{
}
NicAssembler::NicAssembler() : SysIOAssembler(PROCESS_NIC) {}
RoCEAssembler::RoCEAssembler() : SysIOAssembler(PROCESS_ROCE) {}

void GenerateSysIOTrace(const std::vector<SysIOReceiveSendData> &sysIOReceiveSendData,
                        const std::unordered_map<uint16_t, uint32_t> &pidMap,
                        std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    std::string counterName;
    std::string funId;
    uint32_t pid;
    for (const auto &data : sysIOReceiveSendData)
    {
        time = DivideByPowersOfTenWithPrecision(data.timestamp);
        funId = std::to_string(data.funcId);
        pid = pidMap.at(data.deviceId);
        counterName.clear();
        counterName.append("Port ").append(funId).append("/").append(RX_COUNTER);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, counterName);
        event->SetSeriesDValue(RX_DROPPED_RATE, data.rxDroppedRate);
        event->SetSeriesDValue(RX_ERROR_RATE, data.rxErrorRate);
        event->SetSeriesDValue(RX_PACKET_RATE, data.rxPacketRate);
        event->SetSeriesDValue(RX_BANDWIDTH_EFFICIENCY, data.rxBandwidthEfficiency);
        res.push_back(event);
        counterName.clear();
        counterName.append("Port ").append(funId).append("/").append(TX_COUNTER);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, counterName);
        event->SetSeriesDValue(TX_DROPPED_RATE, data.txDroppedRate);
        event->SetSeriesDValue(TX_ERROR_RATE, data.txErrorRate);
        event->SetSeriesDValue(TX_PACKET_RATE, data.txPacketRate);
        event->SetSeriesDValue(TX_BANDWIDTH_EFFICIENCY, data.txBandwidthEfficiency);
        res.push_back(event);
    }
}

uint8_t SysIOAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    std::vector<SysIOReceiveSendData> sysIOReceiveSendData;
    if (processorName_ == PROCESSOR_NAME_NIC)
    {
        auto nicReceiveSendData = dataInventory.GetPtr<std::vector<NicReceiveSendData>>();
        if (nicReceiveSendData == nullptr)
        {
            WARN("Can't get SysIOReceiveSendData from dataInventory");
            return DATA_NOT_EXIST;
        }
        else
        {
            // 业务可以保证只要nicReceiveSendData不是nullptr，一定有元素
            sysIOReceiveSendData = std::move((*nicReceiveSendData)[0].sysIOReceiveSendData);
        }
    }
    else
    {
        auto roceReceiveSendData = dataInventory.GetPtr<std::vector<RoceReceiveSendData>>();
        if (roceReceiveSendData == nullptr)
        {
            WARN("Can't get SysIOReceiveSendData from dataInventory");
            return DATA_NOT_EXIST;
        }
        else
        {
            sysIOReceiveSendData = std::move((*roceReceiveSendData)[0].sysIOReceiveSendData);
        }
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(processorName_);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    GenerateSysIOTrace(sysIOReceiveSendData, pidMap, res_);
    if (res_.empty())
    {
        ERROR("Can't Generate any % process data", processorName_);
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
