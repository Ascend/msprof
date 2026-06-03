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

#include "analysis/csrc/domain/services/persistence/device/device_hccl_persistence.h"

#include "analysis/csrc/domain/services/association/calculator/hccl/include/hccl_calculator.h"
#include "analysis/csrc/domain/services/constant/default_value_constant.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Viewer::Database;
namespace
{
// modelId, opName, taskType, opType, timestamp, relay, retry, dataType, algType, count, groupName, connectionId,
// rank_size
using HcclOpDataFormat =
    std::vector<std::tuple<uint64_t, std::string, std::string, std::string, uint64_t, int32_t, int32_t, std::string,
                           std::string, int32_t, std::string, int64_t, int32_t>>;

// modelId, indexId, opName, iteration, hcclName, groupName, firstTimestamp, planeId, timestamp, duration, isDynamic,
// taskType, opType, connectionId, isMaster, streamId, taskId, durationEstimated, localRank, remoteRank, transportType,
// size, dataType, linkType, bandwidth, contextId, notifyId, batchId, rdmaType, rank_szie
using HcclTaskDataFormat =
    std::vector<std::tuple<uint64_t, int32_t, std::string, uint16_t, std::string, std::string, uint64_t, int32_t,
                           double, double, std::string, std::string, std::string, int64_t, uint16_t, uint32_t, uint16_t,
                           double, uint32_t, uint32_t, std::string, double, std::string, std::string, double, uint32_t,
                           std::string, uint16_t, std::string, int32_t>>;

// opType, count, totalTime, min, avg, max, ratio
using HcclStatisticsFormat = std::vector<std::tuple<std::string, uint32_t, double, double, double, double, double>>;
}  // namespace

bool SaveHcclOpData(DataInventory& dataInventory, const std::string& devicePath)
{
    auto opData = dataInventory.GetPtr<std::vector<HcclOp>>();
    if (opData == nullptr)
    {
        ERROR("hccl op data pointer is nullptr.");
        return false;
    }
    if (opData->empty())
    {
        WARN("Hccl op data is empty.");
        return true;
    }
    DBInfo hcclDB("hccl_single_device.db", "HCCLOpSingleDevice");
    MAKE_SHARED0_RETURN_VALUE(hcclDB.database, HCCLSingleDeviceDB, false);
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, hcclDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(hcclDB.dbRunner, DBRunner, false, dbPath);
    HcclOpDataFormat saveData;
    if (!Utils::Reserve(saveData, opData->size()))
    {
        ERROR("Reserve for % data failed.", hcclDB.tableName);
        return false;
    }
    for (const auto& data : *opData)
    {
        saveData.emplace_back(data.modelId, data.opName, data.taskType, data.opType, data.timestamp, data.relay,
                              data.retry, data.dataType, data.algType, data.count, data.groupName, data.connectionId,
                              data.rankSize);
    }
    return SaveData(saveData, hcclDB, dbPath);
}

bool SaveHcclTaskData(DataInventory& dataInventory, const std::string& devicePath)
{
    auto taskData = dataInventory.GetPtr<std::vector<DeviceHcclTask>>();
    if (taskData == nullptr)
    {
        ERROR("hccl task data pointer is nullptr.");
        return false;
    }
    if (taskData->empty())
    {
        WARN("Hccl task data is empty.");
        return true;
    }
    DBInfo hcclDB("hccl_single_device.db", "HCCLTaskSingleDevice");
    MAKE_SHARED0_RETURN_VALUE(hcclDB.database, HCCLSingleDeviceDB, false);
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, hcclDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(hcclDB.dbRunner, DBRunner, false, dbPath);
    HcclTaskDataFormat saveData;
    if (!Utils::Reserve(saveData, taskData->size()))
    {
        ERROR("Reserve for % data failed.", hcclDB.tableName);
        return false;
    }
    for (const auto& data : *taskData)
    {
        saveData.emplace_back(data.modelId, data.indexId, data.opName, data.iteration, data.hcclName, data.groupName,
                              data.firstTimestamp, data.planeId, data.timestamp, data.duration, data.isDynamic,
                              data.taskType, data.opType, data.connectionId, data.isMaster, data.streamId, data.taskId,
                              data.durationEstimated, data.localRank, data.remoteRank, data.transportType, data.size,
                              data.dataType, data.linkType, data.bandwidth, data.contextId, data.notifyId, data.batchId,
                              data.rdmaType, data.rankSize);
    }
    return SaveData(saveData, hcclDB, dbPath);
}

bool SaveHcclStatisticsData(DataInventory& dataInventory, const std::string& devicePath)
{
    auto statisticsData = dataInventory.GetPtr<std::vector<HcclStatistics>>();
    if (statisticsData == nullptr)
    {
        ERROR("hccl statistics data pointer is nullptr.");
        return false;
    }
    if (statisticsData->empty())
    {
        WARN("Hccl statistics data is empty.");
        return true;
    }
    DBInfo hcclDB("hccl_single_device.db", "HcclOpReport");
    MAKE_SHARED0_RETURN_VALUE(hcclDB.database, HCCLSingleDeviceDB, false);
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, hcclDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(hcclDB.dbRunner, DBRunner, false, dbPath);
    HcclStatisticsFormat saveData;
    if (!Utils::Reserve(saveData, statisticsData->size()))
    {
        ERROR("Reserve for % data failed.", hcclDB.tableName);
        return false;
    }
    for (auto& data : *statisticsData)
    {
        saveData.emplace_back(data.opType, data.count, data.totalTime, data.min, data.avg, data.max, data.ratio);
    }
    return SaveData(saveData, hcclDB, dbPath);
}

uint32_t DeviceHcclPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    INFO("Start to dump device hccl data.");
    const std::string devicePath = dynamic_cast<const DeviceContext&>(context).GetDeviceFilePath();
    bool flag = true;
    if (!SaveHcclOpData(dataInventory, devicePath))
    {
        flag = false;
        ERROR("Save hccl op single device data failed.");
    }
    if (!SaveHcclTaskData(dataInventory, devicePath))
    {
        flag = false;
        ERROR("Save hccl task single device data failed.");
    }
    if (!SaveHcclStatisticsData(dataInventory, devicePath))
    {
        flag = false;
        ERROR("Save hccl statistics single device data failed.");
    }
    return (flag) ? ANALYSIS_OK : ANALYSIS_ERROR;
}
REGISTER_PROCESS_SEQUENCE(DeviceHcclPersistence, true, HcclCalculator);
REGISTER_PROCESS_DEPENDENT_DATA(DeviceHcclPersistence, std::vector<HcclOp>, std::vector<DeviceHcclTask>,
                                std::vector<HcclStatistics>);
REGISTER_PROCESS_SUPPORT_CHIP(DeviceHcclPersistence, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
