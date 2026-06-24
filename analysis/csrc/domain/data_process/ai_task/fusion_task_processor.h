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

#ifndef ANALYSIS_DOMAIN_FUSION_TASK_PROCESSOR_H
#define ANALYSIS_DOMAIN_FUSION_TASK_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/fusion_task_data.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis
{
namespace Domain
{
using OriFusionTaskFormat = std::vector<
    std::tuple<uint32_t, uint32_t, uint32_t, std::string, double, double, double, std::string, uint32_t, uint32_t>>;

class FusionTaskProcessor : public DataProcessor
{
   public:
    FusionTaskProcessor() = default;
    explicit FusionTaskProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    std::vector<FusionTaskTimelineData> FormatData(const OriFusionTaskFormat &oriData, uint16_t deviceId,
                                                   const Analysis::Utils::ProfTimeRecord &record) const;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_FUSION_TASK_PROCESSOR_H
