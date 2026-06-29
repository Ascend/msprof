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

#include "analysis/csrc/application/timeline/acc_pmu_assembler.h"

#include "analysis/csrc/domain/entities/viewer_data/system/include/acc_pmu_data.h"
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
const std::vector<std::string> COUNTERS{"read_bandwidth", "read_ost", "write_bandwidth", "write_ost"};
const std::string VALUE = "value";
}  // namespace

AccPmuAssembler::AccPmuAssembler() : JsonAssembler(PROCESS_ACC_PMU, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void InjectAccPmuTraceToRes(std::vector<uint32_t> &dataList, std::string &time,
                            std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    const int pidIndex = 4;
    for (size_t i = 0; i < COUNTERS.size(); ++i)
    {
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, dataList[pidIndex], DEFAULT_TID, time, COUNTERS[i]);
        if (event != nullptr)
        {
            event->SetSeriesIValue(VALUE, dataList[i]);
            res.push_back(event);
        }
    }
}

void GenerateAccPmuTrace(std::vector<AccPmuData> &accPmuData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
                         std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::string time;
    std::string prevTime;
    uint32_t pid;
    std::vector<uint32_t> resultDataList = {0, 0, 0, 0, 0};
    // processor中sql语句按timestamp升序排序，故可保证time顺序
    for (const auto &data : accPmuData)
    {
        time = DivideByPowersOfTenWithPrecision(data.timestamp);
        pid = pidMap.at(data.deviceId);
        std::vector<uint32_t> dataList = {data.readBwLevel, data.readOstLevel, data.writeBwLevel, data.writeOstLevel,
                                          pid};
        if (prevTime.empty())
        {
            resultDataList = dataList;
            prevTime = time;
            continue;
        }
        if (prevTime == time)
        {
            for (size_t i = 0; i < resultDataList.size(); ++i)
            {
                resultDataList[i] = std::max(resultDataList[i], dataList[i]);
            }
        }
        else
        {
            InjectAccPmuTraceToRes(resultDataList, prevTime, res);
            resultDataList = {0, 0, 0, 0, 0};
            for (size_t i = 0; i < resultDataList.size(); ++i)
            {
                resultDataList[i] = std::max(resultDataList[i], dataList[i]);
            }
            prevTime = time;
        }
    }
    InjectAccPmuTraceToRes(resultDataList, prevTime, res);
}

uint8_t AccPmuAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto accPmuData = dataInventory.GetPtr<std::vector<AccPmuData>>();
    if (accPmuData == nullptr)
    {
        WARN("Can't get accPmuData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_ACC_PMU);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateAccPmuTrace(*accPmuData, pidMap, res_);
    GenerateHWMetaData(pidMap, layerInfo, res_);
    if (res_.empty())
    {
        ERROR("Can't Generate any Acc PMU process data");
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
