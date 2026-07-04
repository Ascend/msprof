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
#include "analysis/csrc/domain/data_process/system/npu_mem_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;

NpuMemProcessor::NpuMemProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool NpuMemProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<NpuMemData> allProcessedData;
    for (const auto &devicePath : deviceList)
    {
        flag = ProcessSingleDevice(devicePath, allProcessedData) && flag;
    }
    if (!SaveToDataInventory<NpuMemData>(std::move(allProcessedData), dataInventory, PROCESSOR_NAME_NPU_MEM))
    {
        flag = false;
        ERROR("Save NpuMem Data To DataInventory failed, profPath is %.", profPath_);
    }
    return flag;
}

bool NpuMemProcessor::ProcessSingleDevice(const std::string &devicePath, std::vector<NpuMemData> &allProcessedData)
{
    LocaltimeContext localtimeContext;
    localtimeContext.deviceId = GetDeviceIdByDevicePath(devicePath);
    if (localtimeContext.deviceId == INVALID_DEVICE_ID)
    {
        ERROR("the invalid deviceId cannot to be identified, profPath is %", profPath_);
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(localtimeContext.timeRecord, profPath_,
                                                      localtimeContext.deviceId))
    {
        ERROR(
            "Failed to obtain the time in start_info and end_info, "
            "profPath is %, device id is %.",
            profPath_, localtimeContext.deviceId);
        return false;
    }
    DBInfo npuMemDB("npu_mem.db", "NpuMem");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, npuMemDB.dbName});
    if (!npuMemDB.ConstructDBRunner(dbPath) || npuMemDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    // 并不是所有场景都有NpuMem数据
    auto status = CheckPathAndTable(dbPath, npuMemDB);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    OriNpuMemData oriData = LoadData(npuMemDB);
    if (oriData.empty())
    {
        ERROR("Get % data failed in %.", npuMemDB.tableName, dbPath);
        return false;
    }
    auto processedData = FormatData(oriData, localtimeContext);
    if (processedData.empty())
    {
        ERROR("Format NpuMem data error, dbPath is %.", dbPath);
        return false;
    }
    FilterDataByStartTime(processedData, localtimeContext.timeRecord.startTimeNs, PROCESSOR_NAME_NPU_MEM);
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());
    return true;
}

OriNpuMemData NpuMemProcessor::LoadData(const DBInfo &npuMemDB)
{
    OriNpuMemData oriData;
    std::string sql{"SELECT event, ddr, hbm, timestamp, memory FROM " + npuMemDB.tableName};
    if (!npuMemDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", npuMemDB.tableName);
    }
    return oriData;
}

std::vector<NpuMemData> NpuMemProcessor::FormatData(const OriNpuMemData &oriData,
                                                    const LocaltimeContext &localtimeContext)
{
    std::vector<NpuMemData> formatData;
    if (!Reserve(formatData, oriData.size()))
    {
        ERROR("Reserve for NpuMem data failed, profPath is %, deviceId is %.", profPath_, localtimeContext.deviceId);
        return formatData;
    }
    NpuMemData tempData;
    double oriTimestamp;
    tempData.deviceId = localtimeContext.deviceId;
    for (const auto &row : oriData)
    {
        std::tie(tempData.event, tempData.ddr, tempData.hbm, oriTimestamp, tempData.memory) = row;
        // 原数据结果还未加device monotonic，无需减去, 且原始时间单位为us
        tempData.timestamp = HPFloat(oriTimestamp).Uint64();
        formatData.push_back(tempData);
    }
    return formatData;
}
}  // namespace Domain
}  // namespace Analysis
