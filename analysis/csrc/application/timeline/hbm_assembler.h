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

#ifndef ANALYSIS_APPLICATION_HBM_ASSEMBLER_H
#define ANALYSIS_APPLICATION_HBM_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"

namespace Analysis
{
namespace Application
{
class HBMAssembler : public JsonAssembler
{
   public:
    HBMAssembler();

   private:
    uint8_t AssembleData(DataInventory& dataInventory, const std::string& profPath) override;

   private:
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_HBM_ASSEMBLER_H
