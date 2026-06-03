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
#ifndef MSPROF_ANALYSIS_UB_PROCESSOR_H
#define MSPROF_ANALYSIS_UB_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ub_data.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;

class UbProcessor : public DataProcessor
{
   public:
    using OriUbFormat = std::vector<std::tuple<uint16_t, uint16_t, uint64_t, uint32_t, uint32_t, uint32_t, uint32_t>>;
    UbProcessor() = default;
    explicit UbProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath, std::vector<UbData> &allProcessedData,
                             LocaltimeContext &localtimeContext);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_UB_PROCESSOR_H
