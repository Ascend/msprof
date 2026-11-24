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

#ifndef ANALYSIS_DOMAIN_ASCEND_TASK_PROCESSOR_H
#define ANALYSIS_DOMAIN_ASCEND_TASK_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"

namespace Analysis
{
namespace Domain
{
// start_time, duration, model_id, index_id, stream_id, task_id, context_id, batch_id, connection_id host_task_type,
// device_task_type
using OriAscendTaskData = std::vector<std::tuple<double, double, uint32_t, int32_t, uint32_t, uint32_t, uint32_t,
                                                 uint32_t, uint64_t, std::string, std::string, double>>;
class TaskProcessor : public DataProcessor
{
   public:
    TaskProcessor() = default;
    explicit TaskProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath, std::vector<AscendTaskData> &allProcessedData);
    OriAscendTaskData LoadData(const DBInfo &ascendTaskDB, const std::string &dbPath);
    std::vector<AscendTaskData> FormatData(const OriAscendTaskData &oriData, const Utils::ProfTimeRecord &timeRecord,
                                           const uint16_t deviceId);

    std::string GetTaskType(const std::string &hostType, const std::string &deviceType, uint16_t platformVersion);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_ASCEND_TASK_PROCESSOR_H
