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

#ifndef ANALYSIS_PARSER_HOST_CANN_TENSOR_DESC_FORMATTER_H
#define ANALYSIS_PARSER_HOST_CANN_TENSOR_DESC_FORMATTER_H

#include <string>
#include <vector>

#include "analysis/csrc/infrastructure/utils/parser_struct.h"
#include "analysis/csrc/infrastructure/utils/prof_struct.h"

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{
struct FormattedTensorFields
{
    std::string inputFormats;
    std::string inputDataTypes;
    std::string inputShapes;
    std::string outputFormats;
    std::string outputDataTypes;
    std::string outputShapes;
};

// Tensor 描述格式化
class TensorDescFormatter
{
   public:
    static std::string GetFormat(uint32_t oriFormat);
    static ParserTensorData ToParserTensor(const MsrofTensorData &src);
    static ParserTensorData ToParserTensor(const MsprofRuntimeOpTensor &src);
    static FormattedTensorFields Format(const ParserTensorData *tensors, uint32_t tensorNum);
    static FormattedTensorFields Format(const std::vector<ParserTensorData> &tensors, uint32_t tensorNum);
};
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_PARSER_HOST_CANN_TENSOR_DESC_FORMATTER_H
