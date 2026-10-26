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
#ifndef ANALYSIS_DOMAIN_NPU_OP_MEM_PROCESSOR_H
#define ANALYSIS_DOMAIN_NPU_OP_MEM_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_op_mem_data.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis
{
namespace Domain
{
using GeHashMap = std::unordered_map<std::string, std::string>;
class NpuOpMemProcessor : public DataProcessor
{
    // operator, addr, size, timestamp, thread_id, total_allocate_memory, total_reserve_memory, device_type
    using OriDataFormat =
        std::vector<std::tuple<std::string, std::string, int64_t, double, uint32_t, uint64_t, uint64_t, std::string>>;

   public:
    NpuOpMemProcessor() = default;
    explicit NpuOpMemProcessor(const std::string& profPath);

   private:
    bool Process(DataInventory& dataInventory) override;
    static OriDataFormat LoadData(const DBInfo& npuOpMemDB);
    std::vector<NpuOpMemData> FormatData(const OriDataFormat& oriData, const Utils::ProfTimeRecord& timeRecord,
                                         Utils::SyscntConversionParams& params, GeHashMap& hashMap) const;
    static uint16_t GetDeviceId(const std::string& deviceType);

   private:
    uint64_t stringReleaseId_ = 0;
    uint64_t stringAllocateId_ = 0;
};

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_NPU_OP_MEM_PROCESSOR_H
