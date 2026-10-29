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

#include "analysis/csrc/application/timeline/hbm_assembler.h"

#include "analysis/csrc/domain/entities/viewer_data/system/include/hbm_data.h"
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
const std::unordered_map<std::string, std::string> CounterNameMap{{"read", "Read"}, {"write", "Write"}};
}

HBMAssembler::HBMAssembler() : JsonAssembler(PROCESS_HBM, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateHbmTrace(std::vector<HbmData> &hbmData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
                      JsonEventSink &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string counterName;
    std::string series;
    for (const auto &data : hbmData)
    {
        counterName.clear();
        counterName.append("HBM ")
            .append(std::to_string(data.hbmId))
            .append("/")
            .append(CounterNameMap.at(data.eventType));
        series.clear();
        series.append(CounterNameMap.at(data.eventType)).append("(MB/s)");
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pidMap.at(data.deviceId), DEFAULT_TID,
                                DivideByPowersOfTenWithPrecision(data.timestamp), counterName);
        event->SetSeriesDValue(series, data.bandWidth);
        res.push_back(event);
    }
}

uint8_t HBMAssembler::AssembleData(DataInventory &dataInventory, const std::string &profPath)
{
    auto hbmData = dataInventory.GetPtr<std::vector<HbmData>>();
    if (hbmData == nullptr)
    {
        WARN("Can't get hbmData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_HBM);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateHWMetaData(pidMap, layerInfo, events_);
    GenerateHbmTrace(*hbmData, pidMap, events_);
    if (events_.empty())
    {
        ERROR("Can't Generate any HBM process data");
        return ASSEMBLE_FAILED;
    }
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
