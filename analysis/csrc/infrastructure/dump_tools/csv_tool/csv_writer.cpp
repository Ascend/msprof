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

#include "analysis/csrc/infrastructure/dump_tools/csv_tool/include/csv_writer.h"

#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/dump_tools/include/sync_utils.h"

namespace Analysis
{
namespace Infra
{
using namespace Application;
namespace
{
const std::string UNDERLINE = "_";
const std::string COMMA = ",";
}  // namespace

CsvWriter::CsvWriter() : timestamp_(GetTimeStampStr()) {}

template <typename Iterator>
void CsvWriter::DumpCsvFile(const std::string& filePath, const std::vector<std::string>& headers, Iterator begin,
                            Iterator end, std::set<int> maskCols)
{
    std::string textStr;
    for (size_t i = 0; i < headers.size(); ++i)
    {
        if (maskCols.find(i) != maskCols.end())
        {
            continue;
        }
        if (i != 0)
        {
            textStr.append(COMMA);
        }
        textStr.append(headers[i]);
    }
    textStr.append("\n");
    for (auto row = begin; row != end; row++)
    {
        uint16_t index = 0;
        for (const auto& valueStr : *row)
        {
            if (maskCols.find(index) != maskCols.end())
            {
                index++;
                continue;
            }
            if (index != 0)
            {
                textStr.append(COMMA);
            }
            textStr.append(valueStr);
            index++;
        }
        textStr.append("\n");
    }
    Utils::FileWriter fileWriter(filePath);
    fileWriter.WriteText(textStr);
}

std::string CsvWriter::GetSliceFileName(uint16_t sliceIndex, size_t resSize, const std::string& filePath)
{
    auto fileName = filePath;
    if (resSize > CSV_LIMIT)
    {
        fileName.append(UNDERLINE).append(SLICE).append(UNDERLINE).append(std::to_string(sliceIndex));
    }
    fileName.append(UNDERLINE).append(timestamp_).append(SUMMARY_SUFFIX);
    return fileName;
}

void CsvWriter::WriteCsv(const std::string& filePath, const std::vector<std::string>& headers,
                         const std::vector<std::vector<std::string>>& res, const std::set<int>& maskCols)
{
    // 使用线程池有额外开销,还不如直接串行写的快
    INFO("Start dump %.csv, timestamp is %.", filePath, timestamp_);
    for (size_t i = 0; i < (res.size() + CSV_LIMIT - 1) / CSV_LIMIT; i++)
    {
        uint64_t begin = i * CSV_LIMIT;
        uint64_t end = std::min((i + 1) * CSV_LIMIT, res.size());
        DumpCsvFile(GetSliceFileName(i, res.size(), filePath), headers, res.begin() + begin, res.begin() + end,
                    maskCols);
    }
    INFO("Generate %.csv finished, timestamp is %.", filePath, timestamp_);
}

}  // namespace Infra
}  // namespace Analysis
