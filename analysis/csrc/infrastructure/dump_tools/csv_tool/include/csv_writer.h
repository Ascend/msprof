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

#ifndef ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_CSV_TOOL_INCLUDE_CSV_WRITER_H
#define ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_CSV_TOOL_INCLUDE_CSV_WRITER_H

#include <set>
#include <string>
#include <vector>

namespace Analysis
{
namespace Infra
{
// Formats one CSV chunk. The caller writes the bytes.
class CsvWriter final
{
   public:
    static std::string FormatHeader(const std::vector<std::string>& headers, const std::set<int>& mask);
    static void AppendRow(std::string& text, const std::vector<std::string>& row, const std::set<int>& mask);
};
}  // namespace Infra
}  // namespace Analysis

#endif  // ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_CSV_TOOL_INCLUDE_CSV_WRITER_H
