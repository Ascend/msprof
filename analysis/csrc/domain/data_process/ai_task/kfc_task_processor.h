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

#ifndef ANALYSIS_DOMAIN_KFC_TASK_PROCESSOR_H
#define ANALYSIS_DOMAIN_KFC_TASK_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
// "device_id", "stream_id", "task_id", "comm_turn", "current_turn", "wait_notify_start_time", "kfc_alg_exe_start_time",
// "send_task_start_time", "wait_active_start_time",
// "active_start_time", "wait_exe_end_start_time", "rtsq_exe_end_time"
using OriKfcCommData = std::vector<std::tuple<uint16_t, uint32_t, uint32_t, uint16_t, uint16_t, uint64_t, uint64_t,
                                              uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
// "device_id", "stream_id", "task_id", "compute_turn", "current_turn", "wait_compute_start_time",
// "compute_start_time", "compute_exe_end_time"
using OriKfcComputeData =
    std::vector<std::tuple<uint16_t, uint32_t, uint32_t, uint16_t, uint16_t, uint64_t, uint64_t, uint64_t>>;

using namespace Analysis::Utils;

struct KfcCommTurn
{
    uint16_t deviceId = UINT16_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint16_t commTurn = UINT16_MAX;
    uint16_t currentTurn = UINT16_MAX;
    uint64_t serverStartTime = UINT64_MAX;
    uint64_t waitMsgStartTime = UINT64_MAX;
    uint64_t kfcAlgExeStartTime = UINT64_MAX;
    uint64_t sendTaskStartTime = UINT64_MAX;
    uint64_t sendSqeFinishTime = UINT64_MAX;
    uint64_t rtsqExeEndTime = UINT64_MAX;
    uint64_t serverEndTime = UINT64_MAX;
};

struct KfcComputeTurn
{
    uint16_t deviceId = UINT16_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint16_t computeTurn = UINT16_MAX;
    uint16_t currentTurn = UINT16_MAX;
    uint64_t waitComputeStartTime = UINT64_MAX;
    uint64_t computeStartTime = UINT64_MAX;
    uint64_t computeExeEndTime = UINT64_MAX;
};

class KfcTaskProcessor : public DataProcessor
{
   public:
    KfcTaskProcessor() = default;
    explicit KfcTaskProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    std::vector<KfcCommTurn> LoadCommData(const DBInfo &kfcDB, const std::string &dbPath, uint16_t deviceId);
    std::vector<KfcComputeTurn> LoadComputeData(const DBInfo &kfcDB, const std::string &dbPath);
    std::vector<KfcTurnData> FormatCommData(const std::vector<KfcCommTurn> &oriCommData,
                                            const SyscntConversionParams &params, const ProfTimeRecord &record);
    std::vector<KfcTurnData> FormatComputeData(const std::vector<KfcComputeTurn> &oriComputeData, uint16_t deviceId,
                                               const SyscntConversionParams &params, const ProfTimeRecord &record);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_KFC_TASK_PROCESSOR_H
