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
#ifndef ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_STRUCT_H
#define ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_STRUCT_H

#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis
{

namespace Infra
{

using ProcessCreator = std::function<std::unique_ptr<Process>()>;

template <typename Key>
struct BasicProcessInfo
{
    BasicProcessInfo() = default;
    BasicProcessInfo(ProcessCreator create, std::vector<std::type_index> inputs, std::vector<Key> dependencies,
                     std::vector<uint32_t> chips, std::string name, bool required)
        : creator(std::move(create)),
          paramTypes(std::move(inputs)),
          processDependence(std::move(dependencies)),
          chipIds(std::move(chips)),
          processName(std::move(name)),
          mandatory(required)
    {
    }
    ProcessCreator creator;                   // 本Process的创建函数
    std::vector<std::type_index> paramTypes;  // 本Process需要的数据类型
    std::vector<Key> processDependence;       // 本Process依赖的前向Process
    std::vector<Key> completionDependence;    // Wait for termination, without requiring success.
    std::vector<uint32_t> chipIds;            // 本Process支持的ChipId
    std::string processName;                  // 本Process的类名for Dfx
    bool mandatory = false;
    std::string scope;
    std::shared_ptr<const Context> context;
    std::shared_ptr<DataInventory> inventory;
    std::vector<std::string> resources;
    bool opensDeviceScope = false;
    // Wait for all predecessors, but allow execution when they failed or were blocked.
    bool runOnDependencyFailure = false;
};

using RuntimeProcessKey = uint64_t;
using RuntimeProcessInfo = BasicProcessInfo<RuntimeProcessKey>;
using RuntimeProcessCollection = std::unordered_map<RuntimeProcessKey, RuntimeProcessInfo>;
}  // namespace Infra
}  // namespace Analysis

#endif
