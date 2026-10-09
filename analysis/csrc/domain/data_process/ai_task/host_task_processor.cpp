/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include "host_task_processor.h"

#include "analysis/csrc/domain/services/device_context/load_host_data.h"

namespace Analysis
{
namespace Domain
{

HostTaskProcessor::HostTaskProcessor(const std::string& profPath) : DataProcessor(profPath) {}

bool HostTaskProcessor::Process(DataInventory& dataInventory)
{
    TaskId2HostTask hostRuntime;
    LoadHostData::ReadHostRuntimeFromDB(this->profPath_, hostRuntime, {});

    std::vector<HostTask> result;
    size_t totalTasks = 0;
    for (auto& it : hostRuntime)
    {
        totalTasks += it.second.size();
    }
    result.reserve(totalTasks);
    for (const auto& pair : hostRuntime)
    {
        const std::vector<HostTask>& tasks = pair.second;
        result.insert(result.end(), tasks.begin(), tasks.end());
    }

    if (!SaveToDataInventory<HostTask>(std::move(result), dataInventory, PROCESSOR_HOST_TASK))
    {
        ERROR("Save data failed, %.", PROCESSOR_HOST_TASK);
        return false;
    }
    return true;
}

}  // namespace Domain
}  // namespace Analysis
