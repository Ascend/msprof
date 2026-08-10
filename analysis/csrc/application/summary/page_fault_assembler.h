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

#ifndef ANALYSIS_APPLICATION_PAGE_FAULT_ASSEMBLER_H
#define ANALYSIS_APPLICATION_PAGE_FAULT_ASSEMBLER_H

#include "analysis/csrc/application/summary/summary_assembler.h"

namespace Analysis
{
namespace Application
{
class PageFaultAssembler : public SummaryAssembler
{
   public:
    PageFaultAssembler() = default;
    PageFaultAssembler(const std::string& name, const std::string& profPath);

   private:
    uint8_t AssembleData(DataInventory& dataInventory) override;
};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_PAGE_FAULT_ASSEMBLER_H
