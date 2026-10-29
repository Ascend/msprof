/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include "analysis/csrc/infrastructure/dump_tools/csv_tool/include/csv_writer.h"

#include <cstdint>

namespace Analysis
{
namespace Infra
{
std::string CsvWriter::FormatHeader(const std::vector<std::string>& headers, const std::set<int>& mask)
{
    std::string text;
    bool firstColumn = true;
    for (size_t i = 0; i < headers.size(); ++i)
    {
        if (mask.find(static_cast<int>(i)) != mask.end())
        {
            continue;
        }
        // Separator depends on whether a column was already emitted, not on the source index: a
        // masked column 0 must not leave a leading comma behind.
        if (!firstColumn)
        {
            text.append(",");
        }
        firstColumn = false;
        text.append(headers[i]);
    }
    text.append("\n");
    return text;
}

void CsvWriter::AppendRow(std::string& text, const std::vector<std::string>& row, const std::set<int>& mask)
{
    uint16_t index = 0;
    bool firstColumn = true;
    for (const auto& value : row)
    {
        if (mask.find(index) != mask.end())
        {
            ++index;
            continue;
        }
        if (!firstColumn)
        {
            text.append(",");
        }
        firstColumn = false;
        text.append(value);
        ++index;
    }
    text.append("\n");
}

}  // namespace Infra
}  // namespace Analysis
