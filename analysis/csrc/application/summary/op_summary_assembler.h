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

#ifndef ANALYSIS_APPLICATION_OP_SUMMARY_ASSEMBLE_H
#define ANALYSIS_APPLICATION_OP_SUMMARY_ASSEMBLE_H

#include <cstdint>
#include <unordered_map>

#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/associated_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/metric_summary.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Infra;
using namespace Analysis::Domain;
class OpSummaryAssembler : public SummaryAssembler
{
   public:
    OpSummaryAssembler() = default;
    OpSummaryAssembler(const std::string &name, const std::string &profPath);

   protected:
    void WriteToFile(const std::string &fileName, const std::set<int> &maskCols);

   private:
    uint8_t AssembleData(DataInventory &dataInventory);
    void GenerateOpBody(const AssociatedTaskCollection &associatedTasks, std::shared_ptr<MetricSummary> &pmu);
    void GenerateHcclBody(std::vector<CommunicationOpData> &opData);
    std::vector<std::string> GenerateOneTaskRow(const TaskInfoData &computeTask, const AscendTaskData &task);
    void MergeTaskAndPmu(std::shared_ptr<MetricSummary> &pmu, std::vector<std::string> &row, const TaskId &id,
                         std::unordered_map<std::string, int> &indexTable);
    void AddCubeUsage(std::vector<std::string> &data, std::unordered_map<std::string, int> &indexTable);
    void CalculateWaitTime();
    std::set<int> GetMaskCols();
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_OP_SUMMARY_ASSEMBLE_H
