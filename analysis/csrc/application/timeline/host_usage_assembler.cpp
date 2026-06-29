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
#include "analysis/csrc/application/timeline/host_usage_assembler.h"

#include <unordered_map>

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Application;
using namespace Analysis::Infra;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
namespace
{
const std::string USAGE = "Usage(%)";
}

OSRuntimeApiAssembler::OSRuntimeApiAssembler() : HostUsageAssembler(PROCESS_OS_RUNTIME_API) {}
uint8_t OSRuntimeApiAssembler::GenerateDataTrace(DataInventory &dataInventory, uint32_t pid)
{
    auto runtimeApiData = dataInventory.GetPtr<std::vector<OSRuntimeApiData>>();
    if (runtimeApiData == nullptr)
    {
        WARN("Can't get host runtime api data from dataInventory.");
        return DATA_NOT_EXIST;
    }
    std::set<int> tidMap;
    double dur;
    std::shared_ptr<DurationEvent> event;
    for (const auto &data : *runtimeApiData)
    {
        dur = static_cast<double>(data.endTime - data.timestamp) / Analysis::Common::NS_TO_US;
        MAKE_SHARED_RETURN_VALUE(event, DurationEvent, ASSEMBLE_FAILED, pid, static_cast<int>(data.tid), dur,
                                 DivideByPowersOfTenWithPrecision(data.timestamp), data.name);
        res_.push_back(event);
        tidMap.insert(static_cast<int>(data.tid));
    }

    std::shared_ptr<MetaDataNameEvent> threadNameEvent;
    for (const auto &tid : tidMap)
    {
        MAKE_SHARED_RETURN_VALUE(threadNameEvent, MetaDataNameEvent, ASSEMBLE_FAILED, pid, tid, META_DATA_THREAD_NAME,
                                 "Thread " + std::to_string(tid));
        res_.push_back(threadNameEvent);
    }
    return ASSEMBLE_SUCCESS;
}

NetworkUsageAssembler::NetworkUsageAssembler() : HostUsageAssembler(PROCESS_NETWORK_USAGE) {}
uint8_t NetworkUsageAssembler::GenerateDataTrace(DataInventory &dataInventory, uint32_t pid)
{
    auto usageData = dataInventory.GetPtr<std::vector<NetWorkUsageData>>();
    if (usageData == nullptr)
    {
        WARN("Can't get host network data from dataInventory.");
        return DATA_NOT_EXIST;
    }
    std::shared_ptr<CounterEvent> event;
    for (const auto &data : *usageData)
    {
        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, ASSEMBLE_FAILED, pid, DEFAULT_TID,
                                 DivideByPowersOfTenWithPrecision(data.timestamp), "Network Usage");
        event->SetSeriesDValue(USAGE, data.usage);
        res_.push_back(event);
    }
    return ASSEMBLE_SUCCESS;
}

DiskUsageAssembler::DiskUsageAssembler() : HostUsageAssembler(PROCESS_DISK_USAGE) {}
uint8_t DiskUsageAssembler::GenerateDataTrace(DataInventory &dataInventory, uint32_t pid)
{
    auto usageData = dataInventory.GetPtr<std::vector<DiskUsageData>>();
    if (usageData == nullptr)
    {
        WARN("Can't get host disk data from dataInventory.");
        return DATA_NOT_EXIST;
    }
    std::shared_ptr<CounterEvent> event;
    for (const auto &data : *usageData)
    {
        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, ASSEMBLE_FAILED, pid, DEFAULT_TID,
                                 DivideByPowersOfTenWithPrecision(data.timestamp), "Disk Usage");
        event->SetSeriesDValue(USAGE, data.usage);
        res_.push_back(event);
    }
    return ASSEMBLE_SUCCESS;
}

MemUsageAssembler::MemUsageAssembler() : HostUsageAssembler(PROCESS_MEMORY_USAGE) {}
uint8_t MemUsageAssembler::GenerateDataTrace(DataInventory &dataInventory, uint32_t pid)
{
    auto usageData = dataInventory.GetPtr<std::vector<MemUsageData>>();
    if (usageData == nullptr)
    {
        WARN("Can't get host memory data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::shared_ptr<CounterEvent> event;
    for (const auto &data : *usageData)
    {
        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, ASSEMBLE_FAILED, pid, DEFAULT_TID,
                                 DivideByPowersOfTenWithPrecision(data.timestamp), "Memory Usage");
        event->SetSeriesDValue(USAGE, data.usage);
        res_.push_back(event);
    }
    return ASSEMBLE_SUCCESS;
}

CpuUsageAssembler::CpuUsageAssembler() : HostUsageAssembler(PROCESS_CPU_USAGE) {}
uint8_t CpuUsageAssembler::GenerateDataTrace(DataInventory &dataInventory, uint32_t pid)
{
    auto usageData = dataInventory.GetPtr<std::vector<CpuUsageData>>();
    if (usageData == nullptr)
    {
        WARN("Can't get host cpu data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::shared_ptr<CounterEvent> event;
    for (const auto &data : *usageData)
    {
        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, ASSEMBLE_FAILED, pid, DEFAULT_TID,
                                 DivideByPowersOfTenWithPrecision(data.timestamp), "CPU " + data.cpuNo);
        event->SetSeriesDValue(USAGE, data.usage);
        res_.push_back(event);
    }
    return ASSEMBLE_SUCCESS;
}

HostUsageAssembler::HostUsageAssembler(const std::string &assembleName)
    : JsonAssembler(assembleName, {{MSPROF_JSON_FILE, FileCategory::MSPROF}})
{
}

uint8_t HostUsageAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    INFO("Begin to assemble % data.", processorName_);
    auto layerInfo = GetLayerInfo(processorName_);
    auto pid = Context::GetInstance().GetPidFromInfoJson(HOST_ID, profPath);
    auto formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex);
    auto generateFlag = GenerateDataTrace(dataInventory, formatPid);
    if (generateFlag != ASSEMBLE_SUCCESS)
    {
        return generateFlag;
    }
    if (res_.empty())
    {
        ERROR("Can't Generate any % data.", processorName_);
        return ASSEMBLE_FAILED;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap = {{HOST_ID, formatPid}};
    GenerateHWMetaData(pidMap, layerInfo, res_);
    for (const auto &node : res_)
    {
        node->DumpJson(ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    INFO("Assemble % data success.", processorName_);
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
