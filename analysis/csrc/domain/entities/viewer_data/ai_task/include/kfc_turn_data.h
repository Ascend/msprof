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
#ifndef ANALYSIS_DOMAIN_KFC_TURN_DATA_H
#define ANALYSIS_DOMAIN_KFC_TURN_DATA_H

#include <string>
#include <utility>

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis
{
namespace Domain
{
struct KfcTurnData : public BasicData
{
    std::string opName;
    uint16_t deviceId = UINT16_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    double duration = 0.0;

   public:
    KfcTurnData() = default;
    KfcTurnData(std::string _opName, uint16_t _deviceId, uint32_t _streamId, uint32_t _taskId, uint64_t _startTime,
                double _duration)
        : BasicData(_startTime),
          opName(std::move(_opName)),
          deviceId(_deviceId),
          streamId(_streamId),
          taskId(_taskId),
          duration(_duration)
    {
    }
};

struct KfcTaskData : public BasicData
{
    uint16_t isMaster = 0;
    uint16_t deviceId = UINT16_MAX;
    int32_t planeId = INT32_MAX;
    uint32_t modelId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t contextId = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    uint32_t srcRank = UINT32_MAX;
    uint32_t dstRank = UINT32_MAX;
    uint64_t transportType = UINT64_MAX;
    uint64_t size = UINT64_MAX;  // Byte
    uint64_t linkType = UINT64_MAX;
    uint64_t dataType = UINT64_MAX;
    uint64_t rdmaType = UINT64_MAX;
    double duration = 0.0;
    double durationEstimated;
    double bandwidth = 0.0;  // GB/S
    std::string hcclName;
    std::string notifyId;
    std::string taskType;
    std::string opName;
    std::string opKey;
    std::string groupName;
    KfcTaskData() = default;
    KfcTaskData(uint16_t _deviceId, uint32_t _modelId, const std::string &_hcclName, uint64_t _timestamp,
                double _duration, const std::string &_notifyId, double _durationEstimated, uint32_t _streamId,
                uint32_t _taskId, uint32_t _contextId, uint32_t _batchId, const std::string &_taskType,
                uint32_t _srcRank, uint32_t _dstRank, uint64_t _transportType, uint64_t _size, uint64_t _linkType,
                double _bandwidth, const std::string &_groupName, int _planeId, uint64_t _dataType, uint16_t _isMaster,
                const std::string &_opName, const std::string &_opKey, uint64_t _rdmaType)
        : BasicData(_timestamp),
          deviceId(_deviceId),
          modelId(_modelId),
          hcclName(_hcclName),
          duration(_duration),
          notifyId(_notifyId),
          durationEstimated(_durationEstimated),
          streamId(_streamId),
          taskId(_taskId),
          contextId(_contextId),
          batchId(_batchId),
          taskType(_taskType),
          srcRank(_srcRank),
          dstRank(_dstRank),
          transportType(_transportType),
          size(_size),
          linkType(_linkType),
          bandwidth(_bandwidth),
          groupName(_groupName),
          planeId(_planeId),
          dataType(_dataType),
          isMaster(_isMaster),
          opName(_opName),
          opKey(_opKey),
          rdmaType(_rdmaType) {};
};

struct KfcOpData : public BasicData
{
    uint16_t deviceId = UINT16_MAX;
    uint32_t modelId = UINT32_MAX;
    uint64_t connectionId = UINT64_MAX;
    uint64_t dataType = UINT64_MAX;
    uint64_t count = UINT64_MAX;
    uint64_t end = UINT64_MAX;
    uint64_t rankSize = UINT64_MAX;
    std::string groupName;
    std::string opName;
    std::string algType;
    int32_t relay = 0;
    int32_t retry = 0;
    std::string opType;
    std::string opKey;
    KfcOpData() = default;
    KfcOpData(uint16_t _deviceId, const std::string &_opName, uint64_t _timestamp, uint64_t _end,
              const std::string &_groupName, uint64_t _connectionId, uint32_t _modelId, uint64_t _dataType,
              uint64_t _count, const std::string &_algType, uint64_t _rankSize, int32_t _relay, int32_t _retry,
              std::string _opType, std::string _opKey)
        : BasicData(_timestamp),
          deviceId(_deviceId),
          opName(_opName),
          end(_end),
          groupName(_groupName),
          connectionId(_connectionId),
          modelId(_modelId),
          dataType(_dataType),
          count(_count),
          algType(_algType),
          rankSize(_rankSize),
          relay(_relay),
          retry(_retry),
          opType(_opType),
          opKey(_opKey) {};
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_KFC_TURN_DATA_H
