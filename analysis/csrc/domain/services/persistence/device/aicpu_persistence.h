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

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_AICPU_PERSISTENCE_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_AICPU_PERSISTENCE_H

#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/domain/entities/hccl/include/kfc_task.h"
#include "analysis/csrc/domain/services/parser/aicpu/include/aicpu_parser.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Infra;
using namespace Analysis::Utils;
class AicpuPersistence : public Process
{
   private:
    uint32_t ProcessEntry(DataInventory& dataInventory, const Context& context) override;
    bool ProcessAicpuDataByDataType(const std::vector<AicpuData>& aicpuData);
    uint32_t GenerateAndSaveData(const std::string& deviceFilePath);

    uint32_t GenerateAndSaveNode(const std::string& deviceFilePath);
    uint32_t GenerateAndSaveDp(const std::string& deviceFilePath);
    uint32_t GenerateAndSaveModel(const std::string& deviceFilePath);
    uint32_t GenerateAndSaveMi(const std::string& deviceFilePath);
    uint32_t GenerateAndSaveCommTurn(const std::string& deviceFilePath);
    uint32_t GenerateAndSaveComputeTurn(const std::string& deviceFilePath);
    uint32_t GenerateAndSaveOpInfo(const std::string& deviceFilePath);
    uint32_t GenerateAndSaveFlipTask(const std::string& deviceFilePath);
    uint32_t GenerateAndSaveMainStreamTask(const std::string& deviceFilePath);
    uint32_t GenerateAndSaveKfcInfos(const std::string& deviceFilePath);

    // halTrackData：device flip（TS_TASK_FLIP，TsTrackParser 注入），用于计算 aicpuStreamId/aicpuTaskId 的
    // aicpuBatchId， 可能为空指针（TsTrackParser 未注入时），此时只计算主流侧 batchId
    void ComputeAicpuBatchId(std::vector<HalTrackData>* halTrackData);
    // 构建注入 DataInventory 的 kfc 上游实体，与 kfc_info.db 落盘行同源同值，
    // 供同进程的 KfcCalculator 直接消费（越过 kfc_info.db 读取）
    std::vector<KfcInfoData> BuildKfcInfoData() const;
    std::vector<MasterStreamTaskData> BuildMasterStreamTaskData() const;

   private:
    std::vector<AicpuData> nodeData_;
    std::vector<AicpuData> dpData_;
    std::vector<AicpuData> modelData_;
    std::vector<AicpuData> miData_;
    std::vector<AicpuData> commTurnData_;
    std::vector<AicpuData> computeTurnData_;
    std::vector<AicpuData> opInfoData_;
    std::vector<AicpuData> flipTaskData_;
    std::vector<AicpuData> mainStreamTaskData_;
    std::vector<AicpuData> kfcInfosData_;
    DeviceStreamInfo deviceStreamInfo_;
    HostStreamInfo hostStreamInfo_;
    GeHashMap geHashMap_;
    SyscntConversionParams params_;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_AICPU_PERSISTENCE_H
