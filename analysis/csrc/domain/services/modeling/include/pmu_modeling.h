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

#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_PMU_MODELING_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_PMU_MODELING_H

#include <unordered_map>

#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis
{
namespace Domain
{
class PmuModeling : public Infra::Process
{
   private:
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context) override;

    template <typename T>
    static bool cmp(T* lData, T* rData)
    {
        return lData->hd.timestamp < rData->hd.timestamp;
    }
    void GenerateBatchId();
    void GroupDataByStream(std::vector<HalPmuData>& pmuData, std::vector<HalTrackData>& flipTrack);

   private:
    std::unordered_map<uint32_t, std::vector<HalPmuData*>> pmu_;
    std::unordered_map<uint32_t, std::vector<HalTrackData*>> flipGroup_;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_SERVICES_MODELING_PMU_MODELING_H
