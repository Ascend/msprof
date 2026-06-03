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

#include <string>
#include <vector>
#include <set>
#include <stdint.h>

namespace Analysis {
namespace Infra {
class CsvWriter final {
public:
    CsvWriter();
    ~CsvWriter() = default;
    void WriteCsv(const std::string& filePath, const std::vector<std::string>& headers,
                  const std::vector<std::vector<std::string>>& res, const std::set<int>& maskCols);
private:
    template <typename Iterator>
    void DumpCsvFile(const std::string& filePath, const std::vector<std::string> &headers,
                     Iterator begin, Iterator end, std::set<int> maskCols);

    std::string GetSliceFileName(uint16_t sliceIndex, size_t resSize, const std::string& filePath);

private:
    std::string timestamp_;
};
}
}

#endif // ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_CSV_TOOL_INCLUDE_CSV_WRITER_H