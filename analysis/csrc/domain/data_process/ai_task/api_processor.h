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

#ifndef ANALYSIS_DOMAIN_API_PROCESSOR_H
#define ANALYSIS_DOMAIN_API_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"

namespace Analysis
{
namespace Domain
{
// struct_type, id, level, thread_id, item_id, start, end, connection_id, event_id
using OriApiDataFormat = std::vector<
    std::tuple<std::string, std::string, std::string, uint32_t, std::string, uint64_t, uint64_t, uint64_t, uint64_t>>;
class ApiProcessor : public DataProcessor
{
   public:
    ApiProcessor() = default;
    explicit ApiProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    OriApiDataFormat LoadData(const DBInfo &apiDB, const std::string &dbPath);
    std::vector<ApiData> FormatData(const OriApiDataFormat &oriData, const Utils::ProfTimeRecord &record,
                                    const Utils::SyscntConversionParams &params);
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_API_PROCESSOR_H
