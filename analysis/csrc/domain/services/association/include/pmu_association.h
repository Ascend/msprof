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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PMU_ASSOCIATION_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PMU_ASSOCIATION_H

#include <map>
#include <set>

#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/domain/services/association/calculator/include/metric_calculator_factory.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;
class PmuAssociation : public Process
{
   private:
    void SplitPmu(std::vector<HalPmuData>& pmuData);
    void MergeContextPmuToDeviceTask(std::vector<HalPmuData*>& pmuVec, std::vector<DeviceTask>& deviceVec,
                                     DataInventory& dataInventory, const DeviceContext& context, uint64_t& filterEnd);
    size_t MergeBlockPmuToDeviceTask(std::vector<HalPmuData*>& pmuData, DeviceTask& deviceTask,
                                     DataInventory& dataInventory, const DeviceContext& context, uint64_t& filterEnd);
    void AssociationByPmuType(std::map<TaskId, std::vector<DeviceTask>>& deviceTask, DataInventory& dataInventory,
                              const DeviceContext& context);
    void CalculateContextPmu(HalPmuData& pmu, DeviceTask& task, DataInventory& dataInventory,
                             const DeviceContext& context);
    void CalculateContextPmuV6(HalPmuData& pmu, DeviceTask& task, DataInventory& dataInventory,
                               const DeviceContext& context);
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context) override;

   private:
    std::map<TaskId, std::vector<HalPmuData*>> contextPmuTask_;
    std::map<TaskId, std::vector<HalPmuData*>> blockPmuTask_;
    std::unique_ptr<MetricCalculator> aicCalculator_;
    std::unique_ptr<MetricCalculator> aivCalculator_;
    // context侧 syscnt 转 wall-clock 的换算参数，ProcessEntry 中按 deviceContext 计算一次
    Analysis::Utils::SyscntConversionParams syscntParams_;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PMU_ASSOCIATION_H
