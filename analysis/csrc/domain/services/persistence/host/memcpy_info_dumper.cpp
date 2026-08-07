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
#include "analysis/csrc/domain/services/persistence/host/memcpy_info_dumper.h"

#include <map>

#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"
#include "analysis/csrc/domain/services/persistence/host/number_mapping.h"

namespace Analysis
{
namespace Domain
{
using HashData = Analysis::Domain::Host::Cann::HashData;
using TypeData = Analysis::Domain::Host::Cann::TypeData;
using namespace Analysis::Utils;
using HostTaskFormat = std::vector<std::tuple<uint32_t, uint64_t, uint32_t, uint32_t, uint32_t, uint16_t>>;
const std::string HOST_TASK_TABLE = "HostTask";
const uint16_t VALID_MEMCPY_OPERATION = 7;
struct MemcpyRecord
{
    uint64_t dataSize;    // memcpy的数据量
    uint64_t maxSize;     // 每个task能拷贝的最大数据量
    uint64_t remainSize;  // 还剩下多少数据量需要后面的task去拷贝
    uint16_t operation;   // 拷贝类型
};
using MEMCPY_ASYNC_FORMAT = std::map<std::pair<uint32_t, uint64_t>, MemcpyRecord>;

bool CheckBothPathAndTableExists(const std::string &path, DBRunner &dbRunner, const std::string &tableName)
{
    if (!File::Exist(path))
    {
        WARN("There is no %", path);
        return false;
    }
    if (!dbRunner.CheckTableExists(tableName))
    {
        WARN("There is no %", tableName);
        return false;
    }
    return true;
}

void GenerateMemcpyRecordMap(const MemcpyInfos &memcpyInfos, MEMCPY_ASYNC_FORMAT &memcpyRecordMap)
{
    for (const auto &memcpyInfo : memcpyInfos)
    {
        auto tmpKey = std::make_pair(memcpyInfo->threadId, memcpyInfo->timeStamp);
        memcpyRecordMap[tmpKey] = {memcpyInfo->data.memcpyInfo.dataSize, memcpyInfo->data.memcpyInfo.maxSize,
                                   memcpyInfo->data.memcpyInfo.dataSize, memcpyInfo->data.memcpyInfo.memcpyDirection};
    }
}

MemcpyRecord GenerateMemcpyRecordByTimestamp(MEMCPY_ASYNC_FORMAT &memcpyRecordMap,
                                             std::pair<uint32_t, uint64_t> &timestampPair)
{
    MemcpyRecord tmpMemcpyRecord{0, 0, 0, UINT16_MAX};
    auto it = memcpyRecordMap.find(timestampPair);
    if (it == memcpyRecordMap.end())
    {
        WARN("MEMCPY_ASYNC task lost memcpyInfo, threadId is %, timestamp is %", timestampPair.first,
             timestampPair.second);
        return tmpMemcpyRecord;
    }
    if (it->second.remainSize == 0)
    {
        WARN("Redundant memcpyAsync task, threadId is %, timestamp is %", timestampPair.first, timestampPair.second);
        return tmpMemcpyRecord;
    }
    else if (it->second.remainSize <= it->second.maxSize)
    {
        tmpMemcpyRecord.dataSize = it->second.remainSize;
        it->second.remainSize = 0;
    }
    else if (it->second.remainSize > it->second.maxSize)
    {
        tmpMemcpyRecord.dataSize = it->second.maxSize;
        it->second.remainSize = it->second.remainSize - tmpMemcpyRecord.dataSize;
    }
    if (it->second.operation <= VALID_MEMCPY_OPERATION)
    {
        tmpMemcpyRecord.operation = it->second.operation;
    }
    return tmpMemcpyRecord;
}

MemcpyInfoDumper::MemcpyInfoDumper(const std::string &hostPath) : BaseDumper<MemcpyInfoDumper>(hostPath, "MemcpyInfo")
{
    MAKE_SHARED0_NO_OPERATION(database_, RuntimeDB);
}

std::vector<MemcpyAsyncTask> MemcpyInfoDumper::GetMemcpyAsyncTasks()
{
    std::vector<MemcpyAsyncTask> MemcpyAsyncTasks;
    RuntimeDB runtimeDb;
    std::string runtimeDbPath = Utils::File::PathJoin({dbPath_, runtimeDb.GetDBName()});
    DBRunner hostRuntimeDBRunner(runtimeDbPath);
    if (!CheckBothPathAndTableExists(runtimeDbPath, hostRuntimeDBRunner, HOST_TASK_TABLE))
    {
        WARN("% not existed", HOST_TASK_TABLE);
        return MemcpyAsyncTasks;
    }
    std::string sql{
        "SELECT thread_id, timestamp, stream_id, batch_id, task_id, device_id FROM HostTask WHERE "
        "task_type='MEMCPY_ASYNC' ORDER BY batch_id ASC, task_id ASC;"};
    HostTaskFormat result(0);
    auto rc = hostRuntimeDBRunner.QueryData(sql, result);
    if (!rc)
    {
        ERROR("Failed to obtain data from the % table.", runtimeDb.GetDBName());
        return MemcpyAsyncTasks;
    }
    MemcpyAsyncTask task;
    for (const auto &row : result)
    {
        std::tie(task.threadId, task.timestamp, task.streamId, task.batchId, task.taskId, task.deviceId) = row;
        MemcpyAsyncTasks.push_back(task);
    }
    return MemcpyAsyncTasks;
}

MemcpyInfoData MemcpyInfoDumper::GenerateData(const MemcpyInfos &memcpyInfos)
{
    MemcpyInfoData data;
    if (!Utils::Reserve(data, memcpyInfos.size()))
    {
        ERROR("MemcpyInfos reserve failed");
        return data;
    }
    MEMCPY_ASYNC_FORMAT memcpyRecordMap;  // 存储时间戳与memcpy_info数据的映射
    GenerateMemcpyRecordMap(memcpyInfos, memcpyRecordMap);
    std::vector<MemcpyAsyncTask> MemcpyAsyncTasks = GetMemcpyAsyncTasks();
    std::string connectionId;
    for (auto &task : MemcpyAsyncTasks)
    {
        auto tmpKey = std::make_pair(task.threadId, task.timestamp);
        MemcpyRecord tmpMemcpyRecord = GenerateMemcpyRecordByTimestamp(memcpyRecordMap, tmpKey);
        data.emplace_back(task.streamId, task.batchId, task.taskId, UINT32_MAX, task.deviceId, tmpMemcpyRecord.dataSize,
                          tmpMemcpyRecord.operation);
    }
    // 落盘的不是原始数据，关联之后可能为空，所以这里补充一行避免出现空db以及后面日志报错
    if (data.empty())
    {
        data.emplace_back(UINT32_MAX, UINT16_MAX, UINT32_MAX, UINT32_MAX, UINT16_MAX, UINT64_MAX, UINT16_MAX);
    }
    return data;
}
}  // namespace Domain
}  // namespace Analysis
