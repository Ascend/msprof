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
#ifndef ANALYSIS_DOMAIN_SIO_PROCESSOR_H
#define ANALYSIS_DOMAIN_SIO_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/sio_data.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;

// 该类用于定义处理sio.db中Sio表
class SioProcessor : public DataProcessor
{
    // die_id, timestamp, req_rx, rsp_rx, snp_rx, dat_rx, req_tx, rsp_tx, snp_tx, dat_tx
    using OriSioData = std::vector<
        std::tuple<uint32_t, double, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>>;

   public:
    SioProcessor() = default;
    explicit SioProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath, std::vector<SioData> &allProcessedData);
    OriSioData LoadData(const DBInfo &sioDB);
    std::vector<SioData> FormatData(const OriSioData &oriData, const LocaltimeContext &localtimeContext);

   private:
    std::unordered_map<uint16_t, std::string> seriesMap_;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SIO_PROCESSOR_H
