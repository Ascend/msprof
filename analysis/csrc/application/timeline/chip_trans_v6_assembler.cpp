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

#include "analysis/csrc/application/timeline/chip_trans_v6_assembler.h"

#include "analysis/csrc/domain/entities/viewer_data/system/include/chip_trans_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

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
const std::vector<std::string> PCIE_COUNTERS{"PCIE Read Bandwidth", "PCIE Write Bandwidth"};
const int STARS_PCIE = 100010;
}  // namespace

ChipTransV6Assembler::ChipTransV6Assembler()
    : JsonAssembler(PROCESS_STARS_CHIP_TRANS, {{MSPROF_JSON_FILE, FileCategory::MSPROF}})
{
}

void GeneratePcieInfoV6Trace(std::vector<PcieInfoV6Data> &pcieInfoV6Data,
                             const std::unordered_map<uint16_t, uint32_t> &pidMap,
                             std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    std::string seriesName;
    uint32_t pid;
    for (const auto &data : pcieInfoV6Data)
    {
        time = DivideByPowersOfTenWithPrecision(data.timestamp);
        if (pidMap.find(data.deviceId) == pidMap.end())
        {
            continue;
        }
        pid = pidMap.at(data.deviceId);
        seriesName = "U-Die" + std::to_string(data.dieId);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, STARS_PCIE, time, PCIE_COUNTERS[0]);
        event->SetSeriesIValue(seriesName, data.pcieReadBandwidth);
        res.emplace_back(event);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, STARS_PCIE, time, PCIE_COUNTERS[1]);
        event->SetSeriesIValue(seriesName, data.pcieWriteBandwidth);
        res.emplace_back(event);
    }
}

uint8_t ChipTransV6Assembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream,
                                           const std::string &profPath)
{
    auto pcieInfoV6Data = dataInventory.GetPtr<std::vector<PcieInfoV6Data>>();
    if (pcieInfoV6Data == nullptr)
    {
        WARN("Can't get pcieInfoV6Data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_STARS_CHIP_TRANS);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap.insert({deviceId, formatPid});
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    GeneratePcieInfoV6Trace(*pcieInfoV6Data, pidMap, res_);
    if (res_.empty())
    {
        ERROR("Can't Generate any Stars Chip Trans data");
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
