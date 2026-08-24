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

#include "analysis/csrc/domain/data_process/include/data_processor_factory.h"

#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
namespace Domain
{
uint32_t DataProcessorProcess::ProcessEntry(Infra::DataInventory& inventory, const Infra::Context&)
{
    if (processor_ == nullptr)
    {
        ERROR("Create processor % failed.", name_);
        return ANALYSIS_ERROR;
    }
    return processor_->Run(inventory, name_) ? ANALYSIS_OK : ANALYSIS_ERROR;
}

}  // namespace Domain
}  // namespace Analysis
