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

#ifndef ANALYSIS_APPLICATION_SUMMARY_ASSEMBLER_H
#define ANALYSIS_APPLICATION_SUMMARY_ASSEMBLER_H

#include <cstdint>
#include <map>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/dump_tools/csv_tool/include/csv_writer.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Infra;
class SummaryAssembler
{
   public:
    SummaryAssembler() = default;
    SummaryAssembler(const std::string &name, const std::string &profPath) : processorName_(name), profPath_(profPath)
    {
    }
    bool Run(DataInventory &dataInventory)
    {
        INFO("Begin to Assemble % data", processorName_);
        auto res = AssembleData(dataInventory);
        if (res == ASSEMBLE_SUCCESS)
        {
            INFO("Assemble and export % data success", processorName_);
            return true;
        }
        else if (res == DATA_NOT_EXIST)
        {
            WARN("No need to Export % data", processorName_);
            return true;
        }
        else
        {
            ERROR("Export % data failed", processorName_);
            return false;
        }
    }

   protected:
    std::string processorName_;
    std::string profPath_;
    std::vector<std::vector<std::string>> res_;
    std::vector<std::string> headers_;
    /**
     * write to csv file
     * @param name file name
     */
    virtual void WriteToFile(const std::string &filePath, const std::set<int> &maskCols)
    {
        CsvWriter csvWriter = CsvWriter();
        csvWriter.WriteCsv(filePath, headers_, res_, maskCols);
    }

   private:
    virtual uint8_t AssembleData(DataInventory &dataInventory) = 0;
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_SUMMARY_ASSEMBLER_H
