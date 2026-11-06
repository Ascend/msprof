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

#ifndef MSPROF_ANALYSIS_LOAD_STREAM_EXPAND_SPEC_DATA_H
#define MSPROF_ANALYSIS_LOAD_STREAM_EXPAND_SPEC_DATA_H

#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis
{
namespace Domain
{
class LoadStreamExpandSpec : public Infra::Process
{
   private:
    uint32_t ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &deviceContext) override;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // MSPROF_ANALYSIS_LOAD_STREAM_EXPAND_SPEC_DATA_H
