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

#ifndef MSPROF_ANALYSIS_TASK_ID_H
#define MSPROF_ANALYSIS_TASK_ID_H

#include <cstdint>
#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>

constexpr uint32_t INVALID_BATCH_ID = 0;
constexpr uint32_t INVALID_CONTEXT_ID = UINT32_MAX;

namespace Analysis
{
namespace Domain
{

struct TaskId
{
    TaskId() = default;

    TaskId(uint32_t _streamId, uint32_t _batchId, uint32_t _taskId, uint32_t _contextId, uint16_t _deviceId = 0)
        : streamId(_streamId), batchId(_batchId), taskId(_taskId), contextId(_contextId), deviceId(_deviceId)
    {
    }

    uint16_t deviceId = 0;
    mutable uint32_t streamId = 0;
    uint32_t batchId = 0;
    uint32_t taskId = 0;
    uint32_t contextId = INVALID_CONTEXT_ID;  // 有效值：0~65535，无效值：INVALID_CONTEXT_ID/UINT32_MAX/全f/-1

    bool operator==(const TaskId &other) const
    {
        // Compare all four member variables; return true if they are equal, otherwise return false
        return deviceId == other.deviceId && taskId == other.taskId && streamId == other.streamId &&
               contextId == other.contextId && batchId == other.batchId;
    }

    bool operator<(const TaskId &other) const
    {
        // 按照taskId,streamId,contextId,batchId的顺序进行比较
        return std::tie(deviceId, taskId, streamId, contextId, batchId) <
               std::tie(other.deviceId, other.taskId, other.streamId, other.contextId, other.batchId);
    }
};

// 自定义哈希函数
struct IDHasher
{
    std::size_t operator()(const TaskId &id) const
    {
        using std::hash;
        return ((hash<uint16_t>()(id.deviceId) ^ (hash<uint32_t>()(id.streamId) << 1)) >> 1) ^
               ((hash<uint32_t>()(id.taskId) << 1) ^ (hash<uint32_t>()(id.batchId) << 1)) ^
               (hash<uint32_t>()(id.contextId) << 1);
    }
};

struct StreamIdInfo
{
    // key: taskId, value: streamId
    std::unordered_map<uint32_t, uint32_t> streamIdMap;
};
struct HostStreamInfo : StreamIdInfo
{
};  // for RTS HostTask
struct DeviceStreamInfo : StreamIdInfo
{
};  // for AICPU KfcInfo
using GeHashMap = std::unordered_map<std::string, std::string>;
}  // namespace Domain
}  // namespace Analysis
#endif  // MSPROF_ANALYSIS_TASK_ID_H
