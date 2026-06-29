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

#include "analysis/csrc/application/timeline/biu_perf_assembler.h"

#include "analysis/csrc/domain/entities/viewer_data/system/include/biu_perf_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
using namespace Analysis::Domain;

namespace Analysis
{
namespace Application
{
BiuPerfAssembler::BiuPerfAssembler() : JsonAssembler(PROCESS_BIU_PERF, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

std::map<std::pair<uint16_t, std::string>, std::vector<BiuPerfData>> groupBiuPerfData(
    const std::vector<BiuPerfData> &biuPerfData)
{
    // 用 map 存储分组，天然保证键的有序性（先 groupId，后 core_type）
    std::map<std::pair<uint16_t, std::string>, std::vector<BiuPerfData>> groupedMap;

    if (biuPerfData.empty())
    {
        return groupedMap;
    }

    // 遍历所有元素，按组合键分组
    for (const auto &data : biuPerfData)
    {
        std::pair<uint16_t, std::string> key = {data.groupId, data.coreType};
        groupedMap[key].emplace_back(data);  // 同组元素加入对应列表
    }

    return groupedMap;
}

void GenerateBiuPerfTrace(std::vector<BiuPerfData> &biuPerfData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
                          std::vector<std::shared_ptr<TraceEvent>> &res)
{
    // group by (group_id, core_type)
    std::map<std::pair<uint16_t, std::string>, std::vector<BiuPerfData>> groupedData = groupBiuPerfData(biuPerfData);
    std::shared_ptr<BiuPerfEvent> event;
    std::shared_ptr<MetaDataNameEvent> threadNameEvent;
    uint16_t tid = 0;
    // if use device sep, get pid from pidMap through deviceId
    uint32_t pid = pidMap.begin()->second;
    for (auto it = groupedData.begin(); it != groupedData.end(); ++it)
    {
        const auto &key = it->first;
        MAKE_SHARED_RETURN_VOID(threadNameEvent, MetaDataNameEvent, pid, tid, META_DATA_THREAD_NAME,
                                "Group" + std::to_string(key.first) + "-" + key.second);
        res.emplace_back(std::move(threadNameEvent));
        // 后续打印逻辑同上
        for (const auto &data : it->second)
        {
            MAKE_SHARED_RETURN_VOID(event, BiuPerfEvent, pid, tid, data.duration / US_TO_MS,
                                    DivideByPowersOfTenWithPrecision(data.timestamp), data.instruction, data.coreType,
                                    data.blockId);
            res.emplace_back(std::move(event));
        }
        tid++;
    }
}

uint8_t BiuPerfAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto biuPerf = dataInventory.GetPtr<std::vector<BiuPerfData>>();
    if (biuPerf == nullptr)
    {
        WARN("Can't get biuPerf data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_BIU_PERF);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Environment::Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    if (pidMap.empty())
    {
        WARN("Can't found any device info.");
        return DATA_NOT_EXIST;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    GenerateBiuPerfTrace(*biuPerf, pidMap, res_);
    if (res_.empty())
    {
        ERROR("Can't Generate any biu perf process data");
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
