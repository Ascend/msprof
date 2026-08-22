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

#ifndef ANALYSIS_DOMAIN_HCCL_STATISTIC_PROCESSOR_H
#define ANALYSIS_DOMAIN_HCCL_STATISTIC_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/hccl_statistic_data.h"

namespace Analysis
{
namespace Domain
{
// op_type, occurrences, total_time, min, avg, max
using OriHcclDataFormat = std::vector<std::tuple<std::string, std::string, double, double, double, double>>;

class HcclStatisticProcessor : public DataProcessor
{
   public:
    explicit HcclStatisticProcessor(const std::string& profPaths);

   private:
    bool Process(DataInventory& dataInventory) override;
    OriHcclDataFormat LoadData(const DBInfo& dbInfo, const std::string& dbPath);
    OriHcclDataFormat LoadReportData(const std::string& devicePath, const std::string& tableName, bool& flag);
    std::vector<HcclStatisticData> FormatData(const OriHcclDataFormat& oriData, const uint16_t deviceId);
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_HCCL_STATISTIC_PROCESSOR_H
