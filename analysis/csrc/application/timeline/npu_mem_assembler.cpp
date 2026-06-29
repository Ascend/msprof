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

#include "analysis/csrc/application/timeline/npu_mem_assembler.h"

#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
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
struct CounterName
{
    std::string hbm;
    std::string ddr;
    std::string memory;
};
const std::unordered_map<std::string, CounterName> CounterNameMap{{"0", {"APP/HBM", "APP/DDR", "APP/Memory"}},
                                                                  {"1", {"Device/HBM", "Device/DDR", "Device/Memory"}}};
}  // namespace

NpuMemAssembler::NpuMemAssembler() : JsonAssembler(PROCESS_NPU_MEM, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateNpuMemTrace(std::vector<NpuMemData> &npuMemData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
                         std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    uint32_t pid;
    for (const auto &data : npuMemData)
    {
        time = DivideByPowersOfTenWithPrecision(data.timestamp);
        pid = pidMap.at(data.deviceId);
        double hbmValue = static_cast<double>(data.hbm) / Analysis::Common::BYTE_SIZE;
        double ddrValue = static_cast<double>(data.ddr) / Analysis::Common::BYTE_SIZE;
        double memoryValue = static_cast<double>(data.memory) / Analysis::Common::BYTE_SIZE;
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, CounterNameMap.at(data.event).ddr);
        event->SetSeriesDValue("KB", ddrValue);
        res.push_back(event);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, CounterNameMap.at(data.event).hbm);
        event->SetSeriesDValue("KB", hbmValue);
        res.push_back(event);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, CounterNameMap.at(data.event).memory);
        event->SetSeriesDValue("KB", memoryValue);
        res.push_back(event);
    }
}

uint8_t NpuMemAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto npuMemData = dataInventory.GetPtr<std::vector<NpuMemData>>();
    if (npuMemData == nullptr)
    {
        WARN("Can't get npuMemData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_NPU_MEM);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    GenerateNpuMemTrace(*npuMemData, pidMap, res_);
    if (res_.empty())
    {
        ERROR("Can't Generate any NpuMem process data");
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
