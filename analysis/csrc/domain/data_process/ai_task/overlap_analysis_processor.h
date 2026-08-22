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

#ifndef ANALYSIS_DOMAIN_OVERLAP_ANALYSIS_PROCESSOR_H
#define ANALYSIS_DOMAIN_OVERLAP_ANALYSIS_PROCESSOR_H

#include <map>
#include <set>
#include <unordered_map>

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/time/time_duration.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/mc2_comm_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/overlap_analysis_data.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis
{
namespace Domain
{
class OverlapAnalysisProcessor : public DataProcessor
{
   public:
    OverlapAnalysisProcessor() = default;
    explicit OverlapAnalysisProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    void RecordCompAndCommTaskTime(const std::shared_ptr<std::vector<AscendTaskData>> &ascendTasks,
                                   const std::shared_ptr<std::vector<TaskInfoData>> &compTasks,
                                   const std::shared_ptr<std::vector<CommunicationOpData>> &commOps,
                                   const std::shared_ptr<std::vector<MC2CommInfoData>> &mc2CommInfos);
    static void SepCompTaskAndKFCCommSections(std::map<TaskId, std::vector<TimeDuration>> &allTaskPool,
                                              const std::shared_ptr<std::vector<TaskInfoData>> &compTasks,
                                              const std::shared_ptr<std::vector<MC2CommInfoData>> &mc2CommInfos,
                                              std::unordered_map<uint16_t, std::vector<TimeDuration>> &compSections);
    template <typename T>
    void GetCommTaskSections(std::unordered_map<uint16_t, std::vector<TimeDuration>> &commOpSections,
                             const std::shared_ptr<std::vector<T>> &commOps);
    void UpdateTaskTimeExtremes(const std::shared_ptr<std::vector<AscendTaskData>> &ascendTasks);
    void UpdateTimeExtremesBySections(uint16_t deviceId, const std::vector<TimeDuration> &sections);
    std::vector<OverlapAnalysisData> BuildOverlapAnalysisData();
    static void AppendEvents(uint16_t deviceId, const std::vector<TimeDuration> &sections, OverlapAnalysisType type,
                             std::vector<OverlapAnalysisData> &events);

    static std::vector<TimeDuration> UnionOneSet(const std::vector<TimeDuration> &vecA);
    static std::vector<TimeDuration> UnionTwoSet(const std::vector<TimeDuration> &vecA,
                                                 const std::vector<TimeDuration> &vecB);
    static std::vector<TimeDuration> GetDifferenceSet(const std::vector<TimeDuration> &vecA,
                                                      const std::vector<TimeDuration> &vecB);
    static void ProcessSetAIsOnTheLeftOfSetB(const TimeDuration &durationA, uint64_t &lastRecordEnd,
                                             std::vector<TimeDuration> &res);
    static void ProcessLeftOfSetAIntersectRightOfSetBOrSetAContainsSetB(const TimeDuration &durationA,
                                                                        const TimeDuration &durationB,
                                                                        uint64_t &lastRecordEnd,
                                                                        std::vector<TimeDuration> &res);

   private:
    std::unordered_map<uint16_t, std::vector<TimeDuration>> compTaskRecords_;
    std::unordered_map<uint16_t, std::vector<TimeDuration>> commTaskRecords_;
    std::unordered_map<uint16_t, uint64_t> begin_;
    std::unordered_map<uint16_t, uint64_t> end_;
    std::set<uint16_t> deviceIds_;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_OVERLAP_ANALYSIS_PROCESSOR_H
