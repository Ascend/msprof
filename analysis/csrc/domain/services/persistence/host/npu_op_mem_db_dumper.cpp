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

#include "analysis/csrc/domain/services/persistence/host/npu_op_mem_db_dumper.h"

#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{

NpuOpMemRawDBDumper::NpuOpMemRawDBDumper(const std::string &hostFilePath)
    : BaseDumper<NpuOpMemRawDBDumper>(hostFilePath, "NpuOpMemRaw")
{
    MAKE_SHARED0_NO_OPERATION(database_, Infra::TaskMemoryDB);
}

NpuOpMemRawDBData NpuOpMemRawDBDumper::GenerateData(const std::vector<std::shared_ptr<ParserAdditionalInfo>> &rawData)
{
    NpuOpMemRawDBData data;
    if (!Utils::Reserve(data, rawData.size()))
    {
        ERROR("NpuOpMemRawDBDumper: Reserve data failed.");
        return data;
    }
    for (const auto &item : rawData)
    {
        if (!item)
        {
            ERROR("NpuOpMemRawDBDumper: Raw data is null.");
            return {};
        }
        const auto &memoryInfo = item->memoryInfo;
        data.emplace_back(std::to_string(memoryInfo.nodeId), std::to_string(memoryInfo.addr), memoryInfo.size,
                          item->timeStamp, item->threadId, memoryInfo.totalAllocateMemory,
                          memoryInfo.totalReserveMemory, static_cast<uint32_t>(item->level), item->type,
                          "NPU:" + std::to_string(memoryInfo.deviceId));
    }
    return data;
}

NpuOpMemRecordDBDumper::NpuOpMemRecordDBDumper(const std::string &hostFilePath)
    : BaseDumper<NpuOpMemRecordDBDumper>(hostFilePath, "NpuOpMemRec")
{
    MAKE_SHARED0_NO_OPERATION(database_, Infra::TaskMemoryDB);
}

NpuOpMemRecordDBData NpuOpMemRecordDBDumper::GenerateData(const std::vector<Host::NpuOpMemRecordData> &recordData)
{
    NpuOpMemRecordDBData data;
    if (!Utils::Reserve(data, recordData.size()))
    {
        ERROR("NpuOpMemRecordDBDumper: Reserve data failed.");
        return data;
    }
    for (const auto &item : recordData)
    {
        data.emplace_back(item.component, item.timestamp, item.totalReserveMemory, item.totalAllocateMemory,
                          item.deviceType);
    }
    return data;
}

NpuOpMemLifecycleDBDumper::NpuOpMemLifecycleDBDumper(const std::string &hostFilePath)
    : BaseDumper<NpuOpMemLifecycleDBDumper>(hostFilePath, "NpuOpMem")
{
    MAKE_SHARED0_NO_OPERATION(database_, Infra::TaskMemoryDB);
}

NpuOpMemLifecycleDBData NpuOpMemLifecycleDBDumper::GenerateData(
    const std::vector<Host::NpuOpMemLifecycleData> &lifecycleData)
{
    NpuOpMemLifecycleDBData data;
    if (!Utils::Reserve(data, lifecycleData.size()))
    {
        ERROR("NpuOpMemLifecycleDBDumper: Reserve data failed.");
        return data;
    }
    for (const auto &item : lifecycleData)
    {
        data.emplace_back(std::to_string(item.operatorId), item.size, item.allocationTime, item.releaseTime,
                          item.duration, item.allocationTotalAllocated, item.allocationTotalReserved,
                          item.releaseTotalAllocated, item.releaseTotalReserved, item.deviceType, item.name);
    }
    return data;
}

}  // namespace Domain
}  // namespace Analysis
