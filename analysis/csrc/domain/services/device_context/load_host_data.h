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

#ifndef MSPROF_ANALYSIS_LOAD_HOST_DATA_H
#define MSPROF_ANALYSIS_LOAD_HOST_DATA_H

#include "analysis/csrc/domain/entities/hal/include/ascend_obj.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"

namespace Analysis
{
namespace Domain
{

using TaskId2HostTask = std::map<TaskId, std::vector<HostTask>>;

class LoadHostData : public Infra::Process
{
   public:
    static bool ReadHostRuntimeFromDB(std::string& profPath, TaskId2HostTask& hostRuntime,
                                      const std::vector<std::string>& deviceIds);

   private:
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& deviceContext) override;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // MSPROF_ANALYSIS_LOAD_HOST_DATA_H
