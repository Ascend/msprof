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
#ifndef ANALYSIS_DOMAIN_LLC_PROCESSOR_H
#define ANALYSIS_DOMAIN_LLC_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/llc_data.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;

// l3tid, timestamp, hitrate, throughput
using OriLLcData = std::vector<std::tuple<uint32_t, double, double, double>>;
// 该类用于定义处理llc.db中LLCMetrics表
class LLcProcessor : public DataProcessor
{
   public:
    LLcProcessor() = default;
    explicit LLcProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath, std::vector<LLcData> &allProcessedData,
                             std::vector<LLcSummaryData> &allSummaryData);
    std::vector<LLcSummaryData> ProcessSummaryData(uint16_t deviceId, const OriLLcData &oriData,
                                                   const std::string &mode);
    OriLLcData LoadData(const DBInfo &llcDB, uint16_t deviceId);
    std::vector<LLcData> FormatData(const OriLLcData &oriData, const LocaltimeContext &localtimeContext,
                                    const std::string &mode);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_LLC_PROCESSOR_H
