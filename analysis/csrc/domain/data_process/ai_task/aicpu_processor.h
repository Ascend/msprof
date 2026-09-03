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

#ifndef ANALYSIS_DOMAIN_AICPU_PROCESSOR_H
#define ANALYSIS_DOMAIN_AICPU_PROCESSOR_H

#include <map>
#include <tuple>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/aicpu_summary_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis
{
namespace Domain
{
using OriAiCpuData =
    std::vector<std::tuple<uint32_t, uint32_t, double, double, std::string, double, double, double, double, double>>;
using OriAiCpuDpData = std::vector<std::tuple<double, std::string, std::string, uint64_t>>;
using OriAiCpuMiData = std::vector<std::tuple<std::string, double, double, uint64_t>>;

class AicpuProcessor : public DataProcessor
{
   public:
    AicpuProcessor() = default;
    explicit AicpuProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath, std::vector<AicpuSummaryData> &summaryData,
                             std::vector<AicpuDpData> &dpData, std::vector<AicpuMiData> &miData);
    bool LoadAiCpuData(const std::string &devicePath, uint16_t deviceId, const Utils::ProfTimeRecord &timeRecord,
                       std::vector<AicpuSummaryData> &summaryData);
    bool LoadDpData(const std::string &devicePath, const Utils::ProfTimeRecord &timeRecord,
                    std::vector<AicpuDpData> &dpData);
    bool LoadMiData(const std::string &devicePath, std::vector<AicpuMiData> &miData);
    void MatchBatchId(std::vector<AicpuSummaryData> &summaryData, const std::vector<AscendTaskData> &ascendTasks);
    void MatchNodeName(std::vector<AicpuSummaryData> &summaryData, const std::vector<TaskInfoData> &taskInfos,
                       bool isChipV6);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_AICPU_PROCESSOR_H
