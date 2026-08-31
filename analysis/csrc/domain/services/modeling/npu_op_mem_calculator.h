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

#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_NPU_OP_MEM_CALCULATOR_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_NPU_OP_MEM_CALCULATOR_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/domain/entities/host/include/npu_op_mem.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"

namespace Analysis
{
namespace Domain
{
namespace Host
{

class NpuOpMemCalculator
{
   public:
    bool Calculate(const std::vector<std::shared_ptr<ParserAdditionalInfo>> &rawData,
                   const std::unordered_map<uint64_t, std::string> &hashData, NpuOpMemCalculationResult &result) const;
};

}  // namespace Host
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_MODELING_NPU_OP_MEM_CALCULATOR_H
