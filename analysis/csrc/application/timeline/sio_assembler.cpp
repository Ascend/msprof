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

#include "analysis/csrc/application/timeline/sio_assembler.h"

#include "analysis/csrc/domain/entities/viewer_data/system/include/sio_data.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
namespace
{
const std::vector<std::string> COUNTERS{"req_rx", "rsp_rx", "snp_rx", "dat_rx", "req_tx", "rsp_tx", "snp_tx", "dat_tx"};
}

SioAssembler::SioAssembler() : JsonAssembler(PROCESS_SIO, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateSioTrace(std::vector<SioData> &sioData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
                      std::vector<std::shared_ptr<TraceEvent>> &res, const std::string &profPath)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    std::string seriesName;
    uint32_t pid;
    std::unordered_map<std::string, std::shared_ptr<CounterEvent>> eventMap;
    auto version = Context::GetInstance().GetPlatformVersion(DEFAULT_DEVICE_ID, profPath);
    for (const auto &data : sioData)
    {
        time = DivideByPowersOfTenWithPrecision(data.timestamp);
        pid = pidMap.at(data.deviceId);
        std::vector<double> bandwidth{data.reqRxBandwidth, data.rspRxBandwidth, data.snpRxBandwidth,
                                      data.datRxBandwidth, data.reqTxBandwidth, data.rspTxBandwidth,
                                      data.snpTxBandwidth, data.datTxBandwidth};
        for (size_t i = 0; i < bandwidth.size(); i++)
        {
            std::string tmpKey = COUNTERS[i] + time;
            auto it = eventMap.find(tmpKey);
            if (it != eventMap.end())
            {
                it->second->SetSeriesDValue(data.name, bandwidth[i]);
            }
            else
            {
                MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, COUNTERS[i]);
                event->SetSeriesDValue(data.name, bandwidth[i]);
                eventMap[tmpKey] = event;
            }
        }
    }
    for (auto &counterEvent : eventMap)
    {
        res.push_back(counterEvent.second);
    }
}

uint8_t SioAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto sioData = dataInventory.GetPtr<std::vector<SioData>>();
    if (sioData == nullptr)
    {
        WARN("Can't get sioData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_SIO);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }

    GenerateHWMetaData(pidMap, layerInfo, res_);
    GenerateSioTrace(*sioData, pidMap, res_, profPath);
    if (res_.empty())
    {
        ERROR("Can't Generate any SIO process data");
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
