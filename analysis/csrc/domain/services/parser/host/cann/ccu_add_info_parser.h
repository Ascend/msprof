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

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_HOST_CANN_CCU_ADD_INFO_PARSER_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_HOST_CANN_CCU_ADD_INFO_PARSER_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{

struct CcuTaskInfoRecord
{
    uint32_t version;
    uint32_t workFlowMode;
    std::string itemId;
    std::string groupName;
    uint32_t rankId;
    uint32_t rankSize;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t dieId;
    uint32_t missionId;
    uint32_t instrId;
};

struct CcuWaitSignalInfoRecord
{
    uint32_t version;
    std::string itemId;
    std::string groupName;
    uint32_t rankId;
    uint32_t rankSize;
    uint32_t workFlowMode;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t dieId;
    uint32_t instrId;
    uint32_t missionId;
    uint32_t ckeId;
    uint32_t mask;
    uint32_t channelId;
    uint32_t remoteRankId;
};

struct CcuGroupInfoRecord
{
    uint32_t version;
    std::string itemId;
    std::string groupName;
    uint32_t rankId;
    uint32_t rankSize;
    uint32_t workFlowMode;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t dieId;
    uint32_t instrId;
    uint32_t missionId;
    std::string reduceOpType;
    std::string inputDataType;
    std::string outputDataType;
    uint64_t dataSize;
    uint32_t channelId;
    uint32_t remoteRankId;
};

struct CcuInfoData
{
    std::vector<CcuTaskInfoRecord> taskRecords;
    std::vector<CcuWaitSignalInfoRecord> waitSignalRecords;
    std::vector<CcuGroupInfoRecord> groupRecords;
    std::vector<std::string> completedFiles;
    bool taskSourceParsed = false;
    bool waitSignalSourceParsed = false;
    bool groupSourceParsed = false;
    bool taskSourceCompleted = false;
    bool waitSignalSourceCompleted = false;
    bool groupSourceCompleted = false;

    bool Empty() const { return taskRecords.empty() && waitSignalRecords.empty() && groupRecords.empty(); }
    bool HasInput() const;
};

class CcuAddInfoParser final
{
   public:
    explicit CcuAddInfoParser(std::string dataPath) : dataPath_(std::move(dataPath)) {}

    bool Parse(CcuInfoData& data) const;

    // GroupEvents consumes one validated batch, including source completion metadata.
    template <typename T>
    std::vector<std::shared_ptr<T>> ParseData() const
    {
        static_assert(std::is_same<T, CcuInfoData>::value, "CCU parser returns CcuInfoData");
        auto data = std::make_shared<T>();
        if (!Parse(*data) || !data->HasInput())
        {
            return {};
        }
        return {std::move(data)};
    }

   private:
    static bool DecodeTask(const uint8_t* rawData, std::size_t size, std::vector<CcuTaskInfoRecord>& records);
    static bool DecodeWaitSignal(const uint8_t* rawData, std::size_t size,
                                 std::vector<CcuWaitSignalInfoRecord>& records);
    static bool DecodeGroup(const uint8_t* rawData, std::size_t size, std::vector<CcuGroupInfoRecord>& records);

    std::string dataPath_;
};

}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PARSER_HOST_CANN_CCU_ADD_INFO_PARSER_H
