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

#ifndef ANALYSIS_DOMAIN_MC2_COMM_INFO_PROCESSOR_H
#define ANALYSIS_DOMAIN_MC2_COMM_INFO_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/mc2_comm_info_data.h"

namespace Analysis
{
namespace Domain
{
// "aicpu_kfc_stream_id", "comm_stream_ids"
using OriMc2InfoData = std::vector<std::tuple<uint32_t, std::string>>;

class Mc2CommInfoProcessor : public DataProcessor
{
   public:
    Mc2CommInfoProcessor() = default;
    explicit Mc2CommInfoProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    std::vector<MC2CommInfoData> LoadOriData(const DBInfo &mc2DB, const std::string &dbPath);
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_MC2_COMM_INFO_PROCESSOR_H
