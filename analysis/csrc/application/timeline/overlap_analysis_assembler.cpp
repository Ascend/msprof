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
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#include "analysis/csrc/application/timeline/overlap_analysis_assembler.h"

#include <set>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/ai_task/overlap_analysis_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/utils/time_logger.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Utils;
using namespace Analysis::Common;
using namespace Analysis::Application;
namespace
{
const std::string COMP_NAME = "Computing";
const std::string COMM_NAME = "Communication";
const std::string COMM_NOT_OVERLAP_COMP_NAME = "Communication(Not Overlapped)";
const std::string FREE_NAME = "Free";
const std::vector<std::string> THREAD_ARGS_NAMES = {COMP_NAME, COMM_NAME, COMM_NOT_OVERLAP_COMP_NAME, FREE_NAME};
const std::vector<uint32_t> TIDS = {static_cast<uint32_t>(OverlapAnalysisType::COMPUTE),
                                    static_cast<uint32_t>(OverlapAnalysisType::COMMUNICATION),
                                    static_cast<uint32_t>(OverlapAnalysisType::COMM_NOT_OVERLAP_COMP),
                                    static_cast<uint32_t>(OverlapAnalysisType::FREE)};
const std::vector<uint32_t> THREAD_INDEXES = TIDS;
std::string GetOverlapName(OverlapAnalysisType type)
{
    switch (type)
    {
        case OverlapAnalysisType::COMMUNICATION:
            return COMM_NAME;
        case OverlapAnalysisType::COMM_NOT_OVERLAP_COMP:
            return COMM_NOT_OVERLAP_COMP_NAME;
        case OverlapAnalysisType::COMPUTE:
            return COMP_NAME;
        case OverlapAnalysisType::FREE:
            return FREE_NAME;
        default:
            return UNKNOWN;
    }
}
}  // namespace

OverlapAnalysisAssembler::OverlapAnalysisAssembler()
    : JsonAssembler(PROCESS_OVERLAP_ANALYSE, {{MSPROF_JSON_FILE, FileCategory::MSPROF}})
{
}

std::vector<std::shared_ptr<TraceEvent>> OverlapAnalysisAssembler::GenerateMetaData(uint16_t deviceId)
{
    std::vector<std::shared_ptr<TraceEvent>> metaEvents;
    uint32_t formatPid = pidMap_[deviceId];
    for (size_t i = 0; i < THREAD_ARGS_NAMES.size(); i++)
    {
        std::shared_ptr<MetaDataNameEvent> threadName;
        MAKE_SHARED_RETURN_VALUE(threadName, MetaDataNameEvent, {}, formatPid, TIDS[i], META_DATA_THREAD_NAME,
                                 THREAD_ARGS_NAMES[i]);
        metaEvents.push_back(threadName);
        std::shared_ptr<MetaDataIndexEvent> threadIndex;
        MAKE_SHARED_RETURN_VALUE(threadIndex, MetaDataIndexEvent, {}, formatPid, TIDS[i], META_DATA_THREAD_INDEX,
                                 THREAD_INDEXES[i]);
        metaEvents.push_back(threadIndex);
    }
    return metaEvents;
}

std::vector<std::shared_ptr<TraceEvent>> OverlapAnalysisAssembler::GenerateOverlapEvents(
    const std::vector<OverlapAnalysisData> &events)
{
    TimeLogger logger{"Generate overlap analysis events"};
    std::vector<std::shared_ptr<TraceEvent>> overlapEvents;
    for (auto &data : events)
    {
        std::shared_ptr<OverlapEvent> event;
        auto formatPid = pidMap_[data.deviceId];
        MAKE_SHARED_RETURN_VALUE(event, OverlapEvent, overlapEvents, formatPid, static_cast<int>(data.type),
                                 static_cast<double>(data.duration) / Analysis::Common::NS_TO_US,
                                 DivideByPowersOfTenWithPrecision(data.timestamp), GetOverlapName(data.type),
                                 data.type);
        overlapEvents.emplace_back(event);
    }
    return overlapEvents;
}

uint8_t OverlapAnalysisAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream,
                                               const std::string &profPath)
{
    auto overlapData = dataInventory.GetPtr<std::vector<OverlapAnalysisData>>();
    if (!overlapData)
    {
        OverlapAnalysisProcessor processor(profPath);
        if (!processor.Run(dataInventory, PROCESSOR_NAME_OVERLAP_ANALYSIS))
        {
            ERROR("Process overlap analysis data failed.");
            return ASSEMBLE_FAILED;
        }
        overlapData = dataInventory.GetPtr<std::vector<OverlapAnalysisData>>();
    }
    if (!overlapData || overlapData->empty())
    {
        WARN("No overlap analysis data found.");
        return DATA_NOT_EXIST;
    }

    auto layerInfo = GetLayerInfo(PROCESS_OVERLAP_ANALYSE);
    std::set<uint16_t> deviceIds;
    for (auto &data : *overlapData)
    {
        deviceIds.insert(data.deviceId);
    }
    for (auto &deviceId : deviceIds)
    {
        auto pid = Analysis::Domain::Environment::Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap_[deviceId] = formatPid;
    }

    std::vector<std::shared_ptr<TraceEvent>> metaEvents;
    GenerateHWMetaData(pidMap_, layerInfo, metaEvents);
    for (auto &deviceId : deviceIds)
    {
        auto oneDeviceMetaEvents = GenerateMetaData(deviceId);
        metaEvents.insert(metaEvents.end(), oneDeviceMetaEvents.begin(), oneDeviceMetaEvents.end());
    }
    auto overlapEvents = GenerateOverlapEvents(*overlapData);

    {
        TimeLogger logger("Dump overlap analysis events");
        for (const auto &node : metaEvents)
        {
            node->DumpJson(ostream);
        }
        for (const auto &node : overlapEvents)
        {
            node->DumpJson(ostream);
        }
    }
    ostream << ",";
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
