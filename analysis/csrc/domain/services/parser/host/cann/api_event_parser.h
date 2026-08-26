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

#ifndef ANALYSIS_PARSER_HOST_CANN_API_EVENT_PARSER_H
#define ANALYSIS_PARSER_HOST_CANN_API_EVENT_PARSER_H

#include <string>
#include <vector>

#include "analysis/csrc/domain/services/adapter/parser_struct_adapter.h"
#include "analysis/csrc/domain/services/parser/host/base_parser.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{
// 该类的作用是Api和Event数据的解析
class ApiEventParser final : public BaseParser<ApiEventParser>
{
   public:
    explicit ApiEventParser(const std::string &path);
    template <typename T>
    std::vector<std::shared_ptr<T>> GetData();

   private:
    int ProduceData() override;

   private:
    std::vector<std::shared_ptr<ParserApi>> apiData_;      // not owned
    std::vector<std::shared_ptr<ParserEvent>> eventData_;  // not owned
    std::vector<std::string> filePrefix_ = {
        "unaging.api_event.data.slice",
        "aging.api_event.data.slice",
    };
};  // class ApiEventParser
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_PARSER_HOST_CANN_API_EVENT_PARSER_H
