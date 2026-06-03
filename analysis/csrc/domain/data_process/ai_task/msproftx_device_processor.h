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

#ifndef ANALYSIS_DOMAIN_MSPROFTX_PROCESSOR_H
#define ANALYSIS_DOMAIN_MSPROFTX_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"

namespace Analysis
{
namespace Domain
{
// model_id, index_id, stream_id, task_id, timestamp, tag_id
using TxDeviceData = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, uint32_t>;
using OriMsprofTxDeviceData = std::vector<TxDeviceData>;

class MsprofTxDeviceProcessor : public DataProcessor
{
   public:
    MsprofTxDeviceProcessor() = default;
    explicit MsprofTxDeviceProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    OriMsprofTxDeviceData LoadData(const DBInfo &stepTraceDB, const std::string &dbPath);
    bool ProcessOneDevice(std::vector<MsprofTxDeviceData> &res, const std::string &devPath);
    std::vector<MsprofTxDeviceData> FormatData(OriMsprofTxDeviceData &oriData, const Utils::ProfTimeRecord &record,
                                               const uint16_t deviceId, const Utils::SyscntConversionParams &params);
    void FormatDataByLegacyRule(std::vector<MsprofTxDeviceData> &processedData, OriMsprofTxDeviceData &oriData,
                                const Utils::ProfTimeRecord &record, const uint16_t deviceId,
                                const Utils::SyscntConversionParams &params);
    void FormatDataByMarkData(std::vector<MsprofTxDeviceData> &processedData, OriMsprofTxDeviceData &oriData,
                              const Utils::ProfTimeRecord &record, const uint16_t deviceId,
                              const Utils::SyscntConversionParams &params);
    void FormatDataByRangeData(std::vector<MsprofTxDeviceData> &processedData, OriMsprofTxDeviceData &oriData,
                               const Utils::ProfTimeRecord &record, const uint16_t deviceId,
                               const Utils::SyscntConversionParams &params);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_MSPROFTX_PROCESSOR_H
