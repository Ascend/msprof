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

#ifndef ANALYSIS_APPLICATION_LOW_POWER_PROCESSOR_H
#define ANALYSIS_APPLICATION_LOW_POWER_PROCESSOR_H

#include <vector>

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/low_power_data.h"

namespace Analysis
{
namespace Domain
{

// 该类用于定义处理lowpower.db中LowPower表
class LowPowerProcessor : public DataProcessor
{
    // timestamp, dieId, freq
    using OriLowPowerDataFormat = std::vector<std::tuple<uint64_t, int32_t, double>>;

   public:
    LowPowerProcessor() = default;
    explicit LowPowerProcessor(const std::string& profPath);

   private:
    bool Process(DataInventory& dataInventory);
    bool ProcessData(const std::string& devicePath, OriLowPowerDataFormat& oriData);
    OriLowPowerDataFormat LoadData(const std::string& dbPath, DBInfo& lowPowerDB);
    bool FormatData(const uint16_t& deviceId, const Utils::ProfTimeRecord& timeRecord,
                    const OriLowPowerDataFormat& lowPowerData, std::vector<LowPowerData>& processedData);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_LOW_POWER_PROCESSOR_H
