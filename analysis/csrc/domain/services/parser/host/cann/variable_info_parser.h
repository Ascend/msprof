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

#ifndef ANALYSIS_PARSER_HOST_CANN_VARIABLE_INFO_PARSER_H
#define ANALYSIS_PARSER_HOST_CANN_VARIABLE_INFO_PARSER_H

#include <string>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/ascend_obj.h"
#include "analysis/csrc/domain/services/parser/host/base_parser.h"
#include "analysis/csrc/domain/services/parser/host/variable_chunk_generator.h"
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
// 该类的作用是 Variable 数据的解析（变长记录：24 字节头 + dataLen）
class VariableInfoParser : public BaseParser<VariableInfoParser>
{
   public:
    explicit VariableInfoParser(const std::string &path, const std::string &parserName) : BaseParser(path, parserName)
    {
    }
    void Init(const std::vector<std::string> &filePrefix);
    int Parse();
    template <typename T>
    std::vector<std::shared_ptr<T>> GetData();
    VariableInfoFormat parserType_ = VariableInfoFormat::VARIABLE_INFO_TYPE;

   protected:
    int ProduceData() override;
    std::shared_ptr<VariableChunkGenerator> variableProducer_;
    std::vector<std::shared_ptr<ParserVariableInfo>> variableData_;
};  // class VariableInfoParser

// capture_op_info：additional 定长 + variable 变长均在本类内分流
class RuntimeOpInfoParser final : public VariableInfoParser
{
   public:
    explicit RuntimeOpInfoParser(const std::string &path) : VariableInfoParser(path, "RuntimeOpInfoParser")
    {
        parserType_ = VariableInfoFormat::RUNTIME_OP_INFO_TYPE;
        Init(variablePrefix_);
    }
    int Parse();
    const std::vector<RuntimeOpInfo> &GetOpInfo() const;

   private:
    int ProduceData() override;
    int ParseAdditional(const std::vector<std::string> &filePrefix, uint16_t isDynamic);
    int ParseVariable();

    std::vector<std::string> unagingAdditionalPrefix_ = {"unaging.additional.capture_op_info.slice"};
    std::vector<std::string> agingAdditionalPrefix_ = {"aging.additional.capture_op_info.slice"};
    std::vector<std::string> variablePrefix_ = {"unaging.variable.capture_op_info.slice"};
    std::vector<RuntimeOpInfo> runtimeOpInfoData_;
};  // class RuntimeOpInfoParser
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_PARSER_HOST_CANN_VARIABLE_INFO_PARSER_H
