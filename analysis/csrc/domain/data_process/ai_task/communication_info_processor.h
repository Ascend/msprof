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
#ifndef ANALYSIS_DOMAIN_COMMUNICATION_INFO_PROCESSOR_H
#define ANALYSIS_DOMAIN_COMMUNICATION_INFO_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"

namespace Analysis
{
namespace Domain
{
using GeHashMap = std::unordered_map<std::string, std::string>;
// model_id, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, remote_rank,
// transport_type, size, data_type, link_type, context_id, notify_id, batch_id, rdma_type, timestamp, duration,
// op_id, bandwidth, is_master, iter_id, source
// 末尾 source 仅 KfcTask/KfcOP 表有该列；HCCL 表 SELECT 不含 source 时该元素越界读恒为 0(=HCCL)
using HcclTaskFormat = std::tuple<uint32_t, std::string, std::string, int32_t, uint32_t, uint32_t, int64_t, int64_t,
                                  std::string, uint64_t, std::string, std::string, uint32_t, std::string, uint32_t,
                                  std::string, double, double, int64_t, double, uint16_t, uint32_t, int32_t>;
using OriTaskDataFormat = std::vector<HcclTaskFormat>;
// connection_id, op_name, relay, retry, data_type, alg_type, count, group_name, op_type, model_id, rank_size, start,
// end, iter_id, source（source 语义同 HcclTaskFormat 末尾）
using HcclOpFormat = std::tuple<int64_t, std::string, int32_t, int32_t, std::string, std::string, uint64_t, std::string,
                                std::string, uint32_t, int64_t, double, double, uint32_t, int32_t>;
using OriOpDataFormat = std::vector<HcclOpFormat>;

// 该类用于依据HCCLSingleDevice库生成COMMUNICATION_TASK_INFO(通信小算子)和COMMUNICATION_OP表(通信大算子)
class CommunicationInfoProcessor : public DataProcessor
{
   public:
    struct CommunicationData
    {
        std::vector<CommunicationTaskData> resTaskData;
        std::vector<CommunicationOpData> resOpData;
        OriTaskDataFormat oriTaskData;
        OriOpDataFormat oriOpData;
        OriTaskDataFormat oriKfcTaskData;
        OriOpDataFormat oriKfcOpData;
        uint16_t deviceId = UINT16_MAX;
        Utils::ProfTimeRecord timeRecord;
        GeHashMap hashMap;
    };
    explicit CommunicationInfoProcessor(const std::string& profPaths);
    virtual ~CommunicationInfoProcessor() = default;

   protected:
    bool FormatTaskData(const OriTaskDataFormat& oriTaskData, std::vector<CommunicationTaskData>& taskFormatData,
                        CommunicationData& communicationData, HcclType type);
    bool FormatOpData(const OriOpDataFormat& oriOpData, std::vector<CommunicationOpData>& opFormatData,
                      CommunicationData& communicationData, HcclType type);

   private:
    bool Process(DataInventory& dataInventory) override;
    OriOpDataFormat LoadOpData(const DBInfo& hcclSingleDeviceDB, bool needSource);
    OriTaskDataFormat LoadTaskData(const DBInfo& hcclSingleDeviceDB, bool needSource);
    bool ProcessOneDevice(const std::string& devicePath, CommunicationData& communicationData);
    CommunicationTaskData UpdateTaskInfo(const HcclTaskFormat& oriData, CommunicationData& communicationData,
                                         HcclType type);
    CommunicationOpData UpdateOpInfo(const HcclOpFormat& oriData, CommunicationData& communicationData, HcclType type);
    // 单张数据表独立处理：谁有谁处理；缺表/空表不影响后续业务；损坏才视为失败
    // needSource=true 时 SELECT 末尾追加 source 列（KfcTask/KfcOP 才有），false 时越界读为 0(=HCCL)
    bool ProcessTaskTable(const std::string& devicePath, const std::string& dbName, const std::string& tableName,
                          OriTaskDataFormat& oriTaskData, std::vector<CommunicationTaskData>& taskFormatData,
                          CommunicationData& communicationData, HcclType type, bool needSource);
    bool ProcessOpTable(const std::string& devicePath, const std::string& dbName, const std::string& tableName,
                        OriOpDataFormat& oriOpData, std::vector<CommunicationOpData>& opFormatData,
                        CommunicationData& communicationData, HcclType type, bool needSource);
    bool ProcessHcclData(const std::string& devicePath, std::vector<CommunicationTaskData>& taskData,
                         std::vector<CommunicationOpData>& opData, CommunicationData& communicationData);
    bool ProcessKfcData(const std::string& devicePath, std::vector<CommunicationTaskData>& taskData,
                        std::vector<CommunicationOpData>& opFormatData, CommunicationData& communicationData);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_COMMUNICATION_INFO_PROCESSOR_H
