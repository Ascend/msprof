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

#include "analysis/csrc/application/timeline/ub_assembler.h"

#include "analysis/csrc/domain/entities/viewer_data/system/include/ub_data.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Application
{
UbAssembler::UbAssembler() : JsonAssembler(PROCESS_UB, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateUbTrace(std::vector<UbData> &ubData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
                     std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    uint32_t pid;
    try
    {
        for (const auto &data : ubData)
        {
            time = DivideByPowersOfTenWithPrecision(data.timestamp);
            pid = pidMap.at(data.deviceId);
            std::string port = "Port" + FormatWithFixedLengthNumber(data.portId, 3);
            MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, "UB " + port);
            event->SetSeriesDValue("bandwidth_rx(MB/s)", data.udmaRxBind);
            event->SetSeriesDValue("bandwidth_tx(MB/s)", data.udmaTxBind);
            res.emplace_back(std::move(event));
        }
    }
    catch (const std::out_of_range &e)
    {
        // 捕获异常，程序不崩溃
        WARN("ub timeline exist invalid deviceId");
    }
}

uint8_t UbAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto ubData = dataInventory.GetPtr<std::vector<UbData>>();
    if (ubData == nullptr)
    {
        WARN("Can't get ub data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_UB);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Environment::Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    GenerateUbTrace(*ubData, pidMap, res_);
    if (res_.empty())
    {
        ERROR("Can't Generate any ub process data");
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
