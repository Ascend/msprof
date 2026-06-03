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
#ifndef ANALYSIS_DOMAIN_COMPUTE_TASK_INFO_PROCESSOR_H
#define ANALYSIS_DOMAIN_COMPUTE_TASK_INFO_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"

namespace Analysis
{
namespace Domain
{
// hashid, model_id, op_name, stream_id, task_id, block_num, mix_block_num, task_type, op_type,
// op_flag, batch_id, input_formats, input_data_types, input_shapes, output_formats,
// output_data_types, output_shapes, device_id, context_id, op_state, grid_dim, block_dim
using TaskInfoFormat =
    std::tuple<std::string, uint32_t, std::string, uint32_t, uint32_t, uint32_t, uint32_t, std::string, std::string,
               std::string, uint32_t, std::string, std::string, std::string, std::string, std::string, std::string,
               uint16_t, uint32_t, std::string, std::string, std::string>;
class ComputeTaskInfoProcessor : public DataProcessor
{
   public:
    ComputeTaskInfoProcessor() = default;
    explicit ComputeTaskInfoProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    std::vector<TaskInfoData> LoadData(const DBInfo &taskInfoDB, const std::string &dbPath,
                                       std::unordered_map<std::string, std::string> &hashMap);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_COMPUTE_TASK_INFO_PROCESSOR_H
