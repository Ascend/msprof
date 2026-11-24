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
#ifndef ANALYSIS_BLOCk_DETAILS_PROCESSOR_H
#define ANALYSIS_BLOCk_DETAILS_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/block_detail_data.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;
using OriBlockDetailDataFormat =
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, double, uint64_t, uint16_t, uint64_t>>;

class BlockDetailProcessor : public DataProcessor
{
   public:
    BlockDetailProcessor() = default;
    explicit BlockDetailProcessor(const std::string& profPath);

   private:
    bool Process(DataInventory& dataInventory) override;
    uint8_t ProcessData(const std::string& devicePath, OriBlockDetailDataFormat& oriData);
    OriBlockDetailDataFormat LoadData(const std::string& dbPath, DBInfo& freqDB);
    std::vector<BlockDetailData> FormatData(const OriBlockDetailDataFormat& oriData,
                                            const LocaltimeContext& localtimeContext);
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_BLOCk_DETAILS_PROCESSOR_H
