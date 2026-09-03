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

#ifndef ANALYSIS_APPLICATION_AICPU_ASSEMBLER_H
#define ANALYSIS_APPLICATION_AICPU_ASSEMBLER_H

#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/aicpu_summary_data.h"

namespace Analysis
{
namespace Application
{
class AicpuAssembler : public SummaryAssembler
{
   public:
    AicpuAssembler() = default;
    AicpuAssembler(const std::string &name, const std::string &profPath);

   private:
    uint8_t AssembleData(DataInventory &dataInventory) override;
    bool WriteAicpuCsv(const std::vector<Domain::AicpuSummaryData> &data);
    bool WriteDpCsv(const std::vector<Domain::AicpuDpData> &data);
    bool WriteMiCsv(const std::vector<Domain::AicpuMiData> &data);
};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_AICPU_ASSEMBLER_H
