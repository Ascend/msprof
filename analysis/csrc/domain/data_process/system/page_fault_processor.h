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

#ifndef ANALYSIS_DOMAIN_PAGE_FAULT_PROCESSOR_H
#define ANALYSIS_DOMAIN_PAGE_FAULT_PROCESSOR_H

#include <map>

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/page_fault_data.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis
{
namespace Domain
{
using OriPageFaultData = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, std::string>>;
using TaskOpInfo = std::vector<std::tuple<std::string, uint32_t, uint32_t, uint16_t>>;

class PageFaultProcessor : public DataProcessor
{
   public:
    PageFaultProcessor() = default;
    explicit PageFaultProcessor(const std::string& profPath);

   private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string& devicePath, const std::map<TaskId, std::string>& taskOpMap,
                             std::vector<PageFaultData>& res);
    OriPageFaultData LoadData(const DBInfo& socPmuDB, const std::string& dbPath);
    bool LoadTaskOpMap(std::map<TaskId, std::string>& taskOpMap);
    std::vector<PageFaultData> FormatData(const OriPageFaultData& oriData, uint16_t deviceId,
                                          const std::map<TaskId, std::string>& taskOpMap);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_PAGE_FAULT_PROCESSOR_H
