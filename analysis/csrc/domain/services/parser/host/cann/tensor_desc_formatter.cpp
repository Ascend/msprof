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

#include "analysis/csrc/domain/services/parser/host/cann/tensor_desc_formatter.h"

#include "analysis/csrc/domain/services/persistence/host/number_mapping.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{
using namespace Analysis::Utils;
using MappingType = Analysis::Domain::NumberMapping::MappingType;

namespace
{
constexpr uint32_t kInputTensorType = 0;
constexpr uint32_t kOutputTensorType = 1;

std::string JoinOrNa(const std::vector<std::string> &values) { return values.empty() ? NA : Utils::Join(values, ";"); }

std::string JoinShapeOrNa(const std::vector<std::string> &values)
{
    return values.empty() ? NA : Utils::AddQuotation(Utils::Join(values, ";"));
}

std::string ShapeToString(const uint32_t *shape, uint32_t dim)
{
    std::vector<std::string> dims;
    for (uint32_t i = 0; i < dim; ++i)
    {
        if (shape[i] == 0)
        {
            break;
        }
        dims.emplace_back(std::to_string(shape[i]));
    }
    return Utils::Join(dims, ",");
}

ParserTensorData CopyTensor(uint32_t tensorType, uint32_t format, uint32_t dataType, const uint32_t *shape)
{
    ParserTensorData dst;
    dst.tensorType = tensorType;
    dst.format = format;
    dst.dataType = dataType;
    for (uint32_t i = 0; i < MSPROF_GE_TENSOR_DATA_SHAPE_LEN; ++i)
    {
        dst.shape[i] = shape[i];
    }
    return dst;
}
}  // namespace

std::string TensorDescFormatter::GetFormat(uint32_t oriFormat)
{
    auto format = (oriFormat == UINT32_MAX) ? UINT32_MAX : (oriFormat & 0xff);
    auto subFormat = (oriFormat == UINT32_MAX) ? 0 : (oriFormat & 0xffff00) >> 8;
    std::string enumFormat = NumberMapping::Get(MappingType::GE_FORMAT, format);
    if (subFormat > 0)
    {
        enumFormat = Utils::Join(std::vector<std::string>{enumFormat, std::to_string(subFormat)}, ":");
    }
    return enumFormat;
}

ParserTensorData TensorDescFormatter::ToParserTensor(const MsrofTensorData &src)
{
    return CopyTensor(src.tensorType, src.format, src.dataType, src.shape);
}

ParserTensorData TensorDescFormatter::ToParserTensor(const MsprofRuntimeOpTensor &src)
{
    return CopyTensor(src.tensorType, src.format, src.dataType, src.shape);
}

FormattedTensorFields TensorDescFormatter::Format(const ParserTensorData *tensors, uint32_t tensorNum)
{
    FormattedTensorFields fields{NA, NA, NA, NA, NA, NA};
    if (tensors == nullptr || tensorNum == 0)
    {
        return fields;
    }
    std::vector<std::string> inputFormats;
    std::vector<std::string> inputDataTypes;
    std::vector<std::string> inputShapes;
    std::vector<std::string> outputFormats;
    std::vector<std::string> outputDataTypes;
    std::vector<std::string> outputShapes;
    for (uint32_t i = 0; i < tensorNum; ++i)
    {
        const auto &tensor = tensors[i];
        auto shape = ShapeToString(tensor.shape, MSPROF_GE_TENSOR_DATA_SHAPE_LEN);
        if (tensor.tensorType == kInputTensorType)
        {
            inputFormats.emplace_back(GetFormat(tensor.format));
            inputDataTypes.emplace_back(NumberMapping::Get(MappingType::GE_DATA_TYPE, tensor.dataType));
            inputShapes.emplace_back(shape);
        }
        else if (tensor.tensorType == kOutputTensorType)
        {
            outputFormats.emplace_back(GetFormat(tensor.format));
            outputDataTypes.emplace_back(NumberMapping::Get(MappingType::GE_DATA_TYPE, tensor.dataType));
            outputShapes.emplace_back(shape);
        }
    }
    fields.inputFormats = JoinOrNa(inputFormats);
    fields.inputDataTypes = JoinOrNa(inputDataTypes);
    fields.inputShapes = JoinShapeOrNa(inputShapes);
    fields.outputFormats = JoinOrNa(outputFormats);
    fields.outputDataTypes = JoinOrNa(outputDataTypes);
    fields.outputShapes = JoinShapeOrNa(outputShapes);
    return fields;
}

FormattedTensorFields TensorDescFormatter::Format(const std::vector<ParserTensorData> &tensors, uint32_t tensorNum)
{
    if (tensors.empty())
    {
        return Format(nullptr, 0);
    }
    auto count = tensorNum < tensors.size() ? tensorNum : static_cast<uint32_t>(tensors.size());
    return Format(tensors.data(), count);
}
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis
