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
// model_id, op_name, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, remote_rank,
// transport_type, size, data_type, link_type, context_id, notify_id, batch_id, rdma_type, timestamp, duration,
// connection_id, duration_estimated, bandwidth, is_master
using HcclTaskFormat =
    std::tuple<uint32_t, std::string, std::string, std::string, int32_t, uint64_t, uint32_t, uint32_t, uint32_t,
               std::string, uint64_t, std::string, std::string, uint32_t, std::string, uint32_t, std::string, double,
               double, uint32_t, double, double, uint16_t>;
using OriTaskDataFormat = std::vector<HcclTaskFormat>;
// connection_id, op_name, relay, retry, data_type, alg_type, count, group_name, op_type, model_id, rank_size
using HcclOpFormat = std::tuple<uint32_t, std::string, int32_t, int32_t, std::string, std::string, uint64_t,
                                std::string, std::string, uint32_t, uint32_t>;
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
    struct HcclTaskSingleDeviceData
    {
        uint32_t connectionId = UINT32_MAX;
        double timestamp = 0.0;
        double duration = 0.0;
        std::string opName;
        std::string HCCLName;
        std::string groupName;
        std::string transportType;
        std::string dataType;
        std::string linkType;
        std::string rdmaType;
    };
    enum class DeviceHcclOpSource
    {
        HCCL = 0,
        MC2 = 1,
        INVALID = 65535
    };
    CommunicationInfoProcessor() = default;
    explicit CommunicationInfoProcessor(const std::string& profPaths);
    virtual ~CommunicationInfoProcessor() = default;

   protected:
    std::unordered_map<uint32_t, size_t> GenOpInfoIdxMap(const OriOpDataFormat& oriOpData);
    bool FormatData(std::vector<CommunicationTaskData>& taskFormatData, std::vector<CommunicationOpData>& opFormatData,
                    CommunicationData& communicationData);

   private:
    bool Process(DataInventory& dataInventory) override;
    virtual OriOpDataFormat LoadOpData(const DBInfo& hcclSingleDeviceDB);
    virtual OriTaskDataFormat LoadTaskData(const DBInfo& hcclSingleDeviceDB);
    bool ProcessOneDevice(const std::string& devicePath, CommunicationData& communicationData);
    void Update(const HcclTaskFormat& oriData, HcclTaskSingleDeviceData& hcclData, CommunicationTaskData& taskData,
                CommunicationData& communicationData);
    void UpdateOpInfo(CommunicationOpData& opData, uint32_t connectionId,
                      const std::unordered_map<uint32_t, size_t>& opInfoIdxMap, const OriOpDataFormat& oriOpData,
                      CommunicationData& communicationData);
    bool FormatKfcData(std::vector<CommunicationTaskData>& taskFormatData,
                       std::vector<CommunicationOpData>& opFormatData, CommunicationData& communicationData);
    bool ProcessHcclData(const std::string& devicePath, std::vector<CommunicationTaskData>& taskData,
                         std::vector<CommunicationOpData>& opData, CommunicationData& communicationData);
    bool ProcessKfcData(const std::string& devicePath, std::vector<CommunicationTaskData>& taskData,
                        std::vector<CommunicationOpData>& opFormatData, CommunicationData& communicationData);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_COMMUNICATION_INFO_PROCESSOR_H
