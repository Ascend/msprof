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

#include "analysis/csrc/application/timeline/pcie_assembler.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Application;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
namespace
{
const std::string PCIe_PREFIX = "PCIe_";
const std::string CPL = PCIe_PREFIX + "cpl";
const std::string NONPOST = PCIe_PREFIX + "nonpost";
const std::string NONPOST_LATENCY = PCIe_PREFIX + "nonpost_latency";
const std::string POST = PCIe_PREFIX + "post";
const std::string TX = "Tx";
const std::string RX = "Rx";
}  // namespace

PCIeAssembler::PCIeAssembler() : JsonAssembler(PROCESS_PCIE, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

std::unordered_map<uint16_t, uint32_t> PCIeAssembler::GeneratePCIeTrace(std::vector<PCIeData> &pcieData,
                                                                        uint32_t sortIndex, const std::string &profPath)
{
    std::unordered_map<uint16_t, uint32_t> pidMap;
    std::shared_ptr<CounterEvent> event;
    for (const auto &data : pcieData)
    {
        auto pid = GetDevicePid(pidMap, data.deviceId, profPath, sortIndex);
        // 时延单位为us, 带宽单位为MB/s
        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, pidMap, pid, DEFAULT_TID,
                                 DivideByPowersOfTenWithPrecision(data.timestamp), CPL);
        event->SetSeriesDValue(TX, static_cast<double>(data.txCpl.avg) / BYTE_SIZE / BYTE_SIZE);
        event->SetSeriesDValue(RX, static_cast<double>(data.rxCpl.avg) / BYTE_SIZE / BYTE_SIZE);
        res_.push_back(event);

        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, pidMap, pid, DEFAULT_TID,
                                 DivideByPowersOfTenWithPrecision(data.timestamp), NONPOST);
        event->SetSeriesDValue(TX, static_cast<double>(data.txNonpost.avg) / BYTE_SIZE / BYTE_SIZE);
        event->SetSeriesDValue(RX, static_cast<double>(data.rxNonpost.avg) / BYTE_SIZE / BYTE_SIZE);
        res_.push_back(event);

        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, pidMap, pid, DEFAULT_TID,
                                 DivideByPowersOfTenWithPrecision(data.timestamp), NONPOST_LATENCY);
        event->SetSeriesDValue(TX, static_cast<double>(data.txNonpostLatency.avg) / MILLI_SECOND);
        event->SetSeriesDValue(RX, 0);
        res_.push_back(event);

        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, pidMap, pid, DEFAULT_TID,
                                 DivideByPowersOfTenWithPrecision(data.timestamp), POST);
        event->SetSeriesDValue(TX, static_cast<double>(data.txPost.avg) / BYTE_SIZE / BYTE_SIZE);
        event->SetSeriesDValue(RX, static_cast<double>(data.rxPost.avg) / BYTE_SIZE / BYTE_SIZE);
        res_.push_back(event);
    }
    return pidMap;
}

uint8_t PCIeAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    INFO("Begin to assemble % data.", PROCESS_PCIE);
    auto pcieData = dataInventory.GetPtr<std::vector<PCIeData>>();
    if (pcieData == nullptr)
    {
        WARN("Can't get PCIe Data from dataInventory");
        return DATA_NOT_EXIST;
    }
    auto layerInfo = GetLayerInfo(PROCESS_PCIE);
    auto pidMap = GeneratePCIeTrace(*pcieData, layerInfo.sortIndex, profPath);
    if (res_.empty())
    {
        ERROR("Can't Generate any PCIe process data");
        return ASSEMBLE_FAILED;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    for (const auto &node : res_)
    {
        node->DumpJson(ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    INFO("Assemble % data success.", PROCESS_PCIE);
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
