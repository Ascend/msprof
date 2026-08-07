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

#ifndef ANALYSIS_APPLICATION_DEVICE_TX_ASSEMBLER_H
#define ANALYSIS_APPLICATION_DEVICE_TX_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"

namespace Analysis
{
namespace Application
{
class DeviceTxAssembler : public JsonAssembler
{
   public:
    DeviceTxAssembler();

   private:
    uint8_t AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    void GenerateDeviceTxTrace(const std::vector<MsprofTxDeviceData> &txData, const std::string &profPath,
                               const LayerInfo &layer, std::unordered_map<uint16_t, uint32_t> &pidMap);
    void GenerateTxConnectionTrace(const MsprofTxDeviceData &data, uint32_t formatPid);

   private:
    std::set<std::pair<uint32_t, uint32_t>> dPidTidSet_;
    std::unordered_map<uint64_t, std::string> h2dMessage_;
    std::vector<std::shared_ptr<TraceEvent>> res_;
};

class DeviceTxTraceEvent : public DurationEvent
{
   public:
    DeviceTxTraceEvent(uint32_t pid, uint32_t tid, double dur, const std::string &ts, const std::string &name,
                       uint32_t streamId, uint32_t taskId)
        : DurationEvent(pid, tid, dur, ts, name), streamId_(streamId), taskId_(taskId)
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    uint32_t streamId_;
    uint32_t taskId_;
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_DEVICE_TX_ASSEMBLER_H
