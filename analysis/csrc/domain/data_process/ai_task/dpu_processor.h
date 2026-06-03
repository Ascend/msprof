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

#ifndef ANALYSIS_DOMAIN_DPU_PROCESSOR_H
#define ANALYSIS_DOMAIN_DPU_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/dpu_data.h"

namespace Analysis
{
namespace Domain
{
class DPUProcessor : public DataProcessor
{
    // dpu_device_id, thread_id, start_time, end_time, task_type, stream_id, task_id, kernel_name(op_name)
    using TrackFormat =
        std::vector<std::tuple<uint16_t, uint32_t, uint64_t, uint64_t, std::string, uint16_t, uint32_t, std::string>>;
    // npu_device_id, dpu_device_id, thread_id, start_time, end_time, op_name, group_name, group_name_id
    // local_rank, remote_rank, rank_size, duration_estimated, src_addr, dst_addr, data_size
    // stream_id, task_id, aicpu_task_id, plane_id, op_type, data_type, link_type, transport_type
    // rdma_type, notify_id
    using HcclTrackFormat = std::vector<
        std::tuple<uint16_t, uint16_t, uint32_t, uint64_t, uint64_t, std::string, std::string, std::string, uint16_t,
                   uint16_t, uint32_t, double, std::string, std::string, uint64_t, uint16_t, uint32_t, uint32_t,
                   uint16_t, std::string, std::string, std::string, std::string, std::string, std::string>>;

   public:
    DPUProcessor() = default;
    explicit DPUProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    DPUProcessor::TrackFormat LoadTrackData();
    DPUProcessor::HcclTrackFormat LoadHcclTrackData();
    void FormatTrackData(const TrackFormat &oriData, std::vector<DPUData> &dpuData, const Utils::ProfTimeRecord &record,
                         const Utils::SyscntConversionParams &params);
    void FormatHcclTrackData(const HcclTrackFormat &oriData, std::vector<DPUData> &dpuData,
                             const Utils::ProfTimeRecord &record, const Utils::SyscntConversionParams &params);
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_DPU_PROCESSOR_H
