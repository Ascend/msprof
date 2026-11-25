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

#ifndef ANALYSIS_DOMAIN_CCU_MISSION_PROCESSOR_H
#define ANALYSIS_DOMAIN_CCU_MISSION_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ccu_mission_data.h"

namespace Analysis
{
namespace Domain
{
using OriCCUMissionData =
    std::vector<std::tuple<uint32_t, uint32_t, uint16_t, uint16_t, uint32_t, uint64_t, uint64_t, std::string>>;
using OriCCUChannelData = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
using OriCCUWaitSignalData = std::vector<std::tuple<uint32_t, uint32_t, uint16_t, uint16_t, uint32_t, uint16_t>>;
using OriCCUGroupData =
    std::vector<std::tuple<uint32_t, uint32_t, uint16_t, uint16_t, std::string, std::string, std::string, uint64_t>>;
using OriCCULoopGroupV62Data = std::vector<std::tuple<uint32_t, uint16_t, uint64_t, uint64_t, uint16_t>>;
using OriCCUSetCKBV62Row = std::tuple<uint32_t, uint16_t, uint16_t, uint16_t, int64_t, uint64_t, std::string, uint16_t,
                                      uint16_t, uint16_t, uint16_t>;
using OriCCUSetCKBV62Data = std::vector<OriCCUSetCKBV62Row>;
using OriCCUChannelV62Data = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint16_t>>;

class CCUDataProcessor : public DataProcessor
{
   protected:
    using DataProcessor::DataProcessor;
    bool LoadHostAddInfo(OriCCUWaitSignalData &waitSignalData, OriCCUGroupData &groupData);
};

class CCUMissionProcessor : public CCUDataProcessor
{
   public:
    CCUMissionProcessor() = default;
    explicit CCUMissionProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    bool LoadMissionAndChannel(const std::string &devicePath, OriCCUMissionData &missionData,
                               OriCCUChannelData &channelData);
    std::vector<CCUMissionTimelineData> FormatTimelineData(const OriCCUMissionData &missionData,
                                                           const OriCCUWaitSignalData &waitSignalData,
                                                           const OriCCUGroupData &groupData,
                                                           const OriCCUChannelData &channelData, uint16_t deviceId,
                                                           const Utils::SyscntConversionParams &params,
                                                           const Utils::ProfTimeRecord &record) const;
};

class CCUMissionV62Processor : public CCUDataProcessor
{
   public:
    explicit CCUMissionV62Processor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    bool LoadV62Data(const std::string &devicePath, OriCCULoopGroupV62Data &loopgroupData,
                     OriCCUSetCKBV62Data &setckbData);
    bool LoadChannelData(const std::string &devicePath, OriCCUChannelV62Data &channelData);
    void FormatLoopGroupV62Timeline(const OriCCULoopGroupV62Data &loopgroupData,
                                    std::vector<CCUV62TimelineData> &result, uint16_t deviceId,
                                    const Utils::SyscntConversionParams &params, const Utils::ProfTimeRecord &record);
    void FormatSetCKBV62Timeline(const OriCCUSetCKBV62Data &setckbData, std::vector<CCUV62TimelineData> &result,
                                 uint16_t deviceId, const Utils::SyscntConversionParams &params,
                                 const Utils::ProfTimeRecord &record);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_CCU_MISSION_PROCESSOR_H
