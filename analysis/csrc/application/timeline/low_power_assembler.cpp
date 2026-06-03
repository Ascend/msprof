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

#include "analysis/csrc/application/timeline/low_power_assembler.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

LowPowerAssembler::LowPowerAssembler() : JsonAssembler(PROCESS_LOW_POWER, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

std::unordered_map<uint16_t, uint32_t> LowPowerAssembler::GenerateFreqTrace(std::vector<LowPowerData> &freqData,
                                                                            uint32_t sortIndex,
                                                                            const std::string &profPath)
{
    std::unordered_map<uint16_t, uint32_t> pidMap;
    std::shared_ptr<CounterEvent> event;
    std::set<int32_t> dDieId{-1, 0, 1};
    for (const auto &data : freqData)
    {
        if (dDieId.find(data.dieId) == dDieId.end())
        {
            continue;
        }
        auto pid = GetDevicePid(pidMap, data.deviceId, profPath, sortIndex);
        std::string processName = "AI Core Freq" + (data.dieId != -1 ? " Die " + std::to_string(data.dieId) : "");

        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, pidMap, pid, DEFAULT_TID,
                                 DivideByPowersOfTenWithPrecision(data.timestamp), processName);
        event->SetSeriesIValue("MHz", static_cast<uint64_t>(data.freq));
        res_.emplace_back(event);
    }
    return pidMap;
}

uint8_t LowPowerAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    INFO("Begin to assemble % data.", PROCESS_LOW_POWER);
    auto freqData = dataInventory.GetPtr<std::vector<LowPowerData>>();
    if (freqData == nullptr)
    {
        WARN("Can't get low power Data from dataInventory");
        return DATA_NOT_EXIST;
    }
    auto layerInfo = GetLayerInfo(PROCESS_AI_CORE_FREQ);
    auto pidMap = GenerateFreqTrace(*freqData, layerInfo.sortIndex, profPath);
    if (res_.empty())
    {
        ERROR("Can't Generate any low power process data");
        return ASSEMBLE_FAILED;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    for (const auto &node : res_)
    {
        node->DumpJson(ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    INFO("Assemble % data success.", PROCESS_LOW_POWER);
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
