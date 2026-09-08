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

#include "analysis/csrc/domain/services/modeling/include/pmu_modeling.h"

#include <algorithm>

#include "analysis/csrc/domain/services/modeling/batch_id/batch_id.h"
#include "analysis/csrc/domain/services/parser/pmu/include/ffts_profile_parser.h"
#include "analysis/csrc/domain/services/parser/track/include/ts_track_parser.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;
using namespace Analysis::Infra;
void PmuModeling::GroupDataByStream(std::vector<HalPmuData>& originPmuData, std::vector<HalTrackData>& flipTrack)
{
    for (auto& pmuData : originPmuData)
    {
        pmu_[pmuData.hd.taskId.streamId].push_back(&pmuData);
    }
    if (!flipTrack.empty())
    {
        auto flipIt = GetFlipData(flipTrack);
        flipGroup_.swap(flipIt);
    }
}

void PmuModeling::GenerateBatchId()
{
    for (auto& pmuVec : pmu_)
    {
        auto it = flipGroup_.find(pmuVec.first);
        if (it != flipGroup_.end())
        {
            std::sort(pmuVec.second.begin(), pmuVec.second.end(), cmp<HalPmuData>);
            std::sort(it->second.begin(), it->second.end(), cmp<HalTrackData>);
            ModelingComputeBatchIdBinary(ReinterpretConvert<HalUniData**>(pmuVec.second.data()), pmuVec.second.size(),
                                         ReinterpretConvert<HalUniData**>(it->second.data()), it->second.size());
        }
    }
    INFO("PMU modeling has done!");
}

uint32_t PmuModeling::ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context)
{
    auto originPmuData = dataInventory.GetPtr<std::vector<HalPmuData>>();
    auto flipTrack = dataInventory.GetPtr<std::vector<HalTrackData>>();
    if (!originPmuData)
    {
        INFO("There is no PMU data, can't supplement batchId");
        return Analysis::ANALYSIS_OK;
    }
    // 不依赖flip边界划分多波次，无需补batch id（flip补全逻辑假设V4的16位taskId语义，对V6不适用）。
    if (context.GetChipID() == CHIP_V6_1_0 || context.GetChipID() == CHIP_V6_2_0)
    {
        return Analysis::ANALYSIS_OK;
    }
    GroupDataByStream(*originPmuData, *flipTrack);
    GenerateBatchId();
    return Analysis::ANALYSIS_OK;
}

REGISTER_PROCESS_SEQUENCE(PmuModeling, true, FftsProfileParser, TsTrackParser);
REGISTER_PROCESS_DEPENDENT_DATA(PmuModeling, std::vector<HalPmuData>, std::vector<HalTrackData>);
REGISTER_PROCESS_SUPPORT_CHIP(PmuModeling, CHIP_V4_1_0, CHIP_V6_1_0, CHIP_V6_2_0);
}  // namespace Domain
}  // namespace Analysis
