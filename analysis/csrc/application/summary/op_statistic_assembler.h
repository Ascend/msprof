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

#ifndef ANALYSIS_APPLICATION_SUMMARY_OP_STATISTIC_ASSEMBLE_H
#define ANALYSIS_APPLICATION_SUMMARY_OP_STATISTIC_ASSEMBLE_H

#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/op_statistic_data.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Infra;
using namespace Analysis::Domain;

class OpStatisticAssembler : public SummaryAssembler
{
   public:
    OpStatisticAssembler() = default;
    OpStatisticAssembler(const std::string& name, const std::string& profPath);

   private:
    uint8_t AssembleData(DataInventory& dataInventory);
    // 入参为数据中心里的数据，只读：排序在函数内部对本地指针数组做
    void GenerateOpStatistic(const std::vector<OpStatisticData>& opStatisticData);
};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_SUMMARY_OP_STATISTIC_ASSEMBLE_H
