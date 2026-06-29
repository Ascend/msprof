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

#include "analysis/csrc/application/timeline/llc_assembler.h"

#include "analysis/csrc/domain/entities/viewer_data/system/include/llc_data.h"
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
const std::string HIT_RATE_SERIES = "Hit Rate(%)";
const std::string THROUGHPUT_SERIES = "Throughput(MB/s)";
const int8_t PERCENT_FACTOR = 100;
struct CounterName
{
    std::string hitRate;
    std::string throughput;
};
const std::unordered_map<std::string, CounterName> CounterNameMap{{"read", {"Read/Hit Rate", "Read/Throughput"}},
                                                                  {"write", {"Write/Hit Rate", "Write/Throughput"}}};
}  // namespace

LLcAssembler::LLcAssembler() : JsonAssembler(PROCESS_LLC, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateLLcTrace(const std::vector<LLcData> &llcData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
                      std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    std::string llcId;
    std::string counterName;
    uint32_t pid;
    for (const auto &data : llcData)
    {
        time = DivideByPowersOfTenWithPrecision(data.timestamp);
        llcId = std::to_string(data.llcID);
        pid = pidMap.at(data.deviceId);  // 业务可以保证此处一定可以找到
        // hitRate
        counterName.clear();
        counterName.append("LLC ").append(llcId).append(" ").append(CounterNameMap.at(data.mode).hitRate);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, counterName);
        event->SetSeriesDValue(HIT_RATE_SERIES, data.hitRate / PERCENT_FACTOR);
        res.push_back(event);
        // throughput
        counterName.clear();
        counterName.append("LLC ").append(llcId).append(" ").append(CounterNameMap.at(data.mode).throughput);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, counterName);
        event->SetSeriesDValue(THROUGHPUT_SERIES, data.throughput);
        res.push_back(event);
    }
}

uint8_t LLcAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto LlcData = dataInventory.GetPtr<std::vector<LLcData>>();
    if (LlcData == nullptr)
    {
        WARN("Can't get LLcData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_LLC);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    GenerateLLcTrace(*LlcData, pidMap, res_);
    if (res_.empty())
    {
        ERROR("Can't Generate any LLc process data");
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
