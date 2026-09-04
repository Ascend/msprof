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

#ifndef ANALYSIS_DOMAIN_ASSOCIATED_TASK_DATA_H
#define ANALYSIS_DOMAIN_ASSOCIATED_TASK_DATA_H

#include <memory>
#include <vector>

#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"

namespace Analysis
{
namespace Domain
{
struct AssociatedTaskData
{
    AssociatedTaskData() = default;
    AssociatedTaskData(const TaskInfoData *taskInfoData, const AscendTaskData *ascendTaskData,
                       bool isOpSummaryRequired = true)
        : taskInfo(taskInfoData), ascendTask(ascendTaskData), opSummaryRequired(isOpSummaryRequired)
    {
    }

    const TaskInfoData *taskInfo = nullptr;
    const AscendTaskData *ascendTask = nullptr;
    // This flag controls only the op_summary view; other consumers still receive every matched task.
    bool opSummaryRequired = true;
};

struct AssociatedTaskCollection
{
    // Keep source vectors alive after DataInventory releases their standalone entries.
    std::shared_ptr<const std::vector<TaskInfoData>> taskInfoData;
    std::shared_ptr<const std::vector<AscendTaskData>> ascendTaskData;
    std::vector<AssociatedTaskData> records;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_ASSOCIATED_TASK_DATA_H
