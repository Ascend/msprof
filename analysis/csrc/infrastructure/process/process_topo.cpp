/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */
#include "analysis/csrc/infrastructure/process/process_topo.h"

#include <algorithm>

#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{

namespace Infra
{

class ProcessTopo::Impl final
{
   public:
    ProcessCollection BuildProcessControlTopoByChip(uint32_t chipId) const
    {
        auto chipTopo = BuildRegChipTopo(chipId);
        Prune(chipTopo);
        return chipTopo;
    }

    explicit Impl(const ProcessCollection& processCollection) : allProcess_(processCollection) {}
    ~Impl() = default;

   private:
    ProcessCollection BuildRegChipTopo(uint32_t chipId) const
    {
        ProcessCollection chipRelatedProcess;
        for (const auto& processPair : allProcess_)
        {
            const auto& processType = processPair.first;
            const auto& regInfo = processPair.second;
            // 筛选注册的Process支持当前的chip 或 注册的Process支持的是所有芯片
            auto chipIdMatch =
                std::find_if(regInfo.chipIds.begin(), regInfo.chipIds.end(), [chipId](uint32_t regChip)
                             { return regChip == chipId || regChip == CHIP_ID_ALL; }) != std::end(regInfo.chipIds);
            if (chipIdMatch)
            {
                chipRelatedProcess[processType] = regInfo;
            }
        }
        return chipRelatedProcess;
    }
    void Prune(ProcessCollection& chipTopo) const  // 剪枝
    {
        for (auto& procPair : chipTopo)
        {
            auto& regDependence = procPair.second.processDependence;
            // Remove Invalid dependency: 删除所有前向依赖中，那些被chip id过滤的Process
            regDependence.erase(
                std::remove_if(regDependence.begin(), regDependence.end(), [&chipTopo](const std::type_index& typeIndex)
                               { return chipTopo.find(typeIndex) == chipTopo.end(); }),
                regDependence.end());
        }
    }

   private:
    const ProcessCollection& allProcess_;
};

ProcessCollection ProcessTopo::BuildProcessControlTopoByChip(uint32_t chipId) const
{
    ProcessCollection chipTopo;
    if (impl_)
    {
        chipTopo = impl_->BuildProcessControlTopoByChip(chipId);
    }
    else
    {
        ERROR("Make ProcessTopo Impl failed");
    }
    return chipTopo;
}

ProcessTopo::ProcessTopo(const ProcessCollection& processCollection) : impl_(new(std::nothrow) Impl(processCollection))
{
}

ProcessTopo::~ProcessTopo() = default;

}  // namespace Infra

}  // namespace Analysis
