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

#ifndef ANALYSIS_APPLICATION_SUMMARY_TASK_TIME_ASSEMBLER_H
#define ANALYSIS_APPLICATION_SUMMARY_TASK_TIME_ASSEMBLER_H

#include <string>
#include <utility>

#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/domain/entities/hal/include/ascend_obj.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Infra;
using namespace Analysis::Domain;

class TaskTimeAssembler : public SummaryAssembler
{
   public:
    TaskTimeAssembler() = default;
    TaskTimeAssembler(const std::string &name, const std::string &profPath);

   private:
    uint8_t AssembleData(DataInventory &dataInventory);
    void FormatTaskInfoData(const std::vector<TaskInfoData> &taskInfoData);
    void FormatHostTaskData(const std::vector<HostTask> &hostTaskData);
    uint8_t AssembleTaskTime(const std::vector<AscendTaskData> &ascendTaskData);

   private:
    // pair.first is op_name, pair.second is task_type
    std::map<TaskId, std::pair<std::string, std::string>> formatedTaskInfo_;
    std::map<TaskId, std::string> opNameMap;
};

}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_SUMMARY_TASK_TIME_ASSEMBLER_H
