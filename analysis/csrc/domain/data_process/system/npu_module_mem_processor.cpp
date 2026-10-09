/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */
#include "analysis/csrc/domain/data_process/system/npu_module_mem_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;
using namespace Analysis::Domain::Environment;

NpuModuleMemProcessor::NpuModuleMemProcessor(const std::string& profPath) : DataProcessor(profPath) {}

bool NpuModuleMemProcessor::Process(DataInventory& dataInventory)
{
    LocaltimeContext localtimeContext;
    bool flag = true;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<NpuModuleMemData> allProcessedData;
    for (const auto& devicePath : deviceList)
    {
        localtimeContext.deviceId = GetDeviceIdByDevicePath(devicePath);
        SyscntConversionParams params;
        if (!Context::GetInstance().GetSyscntConversionParams(params, localtimeContext.deviceId, profPath_))
        {
            ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
            return false;
        }
        flag = ProcessSingleDevice(devicePath, localtimeContext, allProcessedData, params) && flag;
    }
    if (!SaveToDataInventory<NpuModuleMemData>(std::move(allProcessedData), dataInventory,
                                               PROCESSOR_NAME_NPU_MODULE_MEM))
    {
        flag = false;
        ERROR("Save NpuModuleMem Data To DataInventory failed, profPath is %.", profPath_);
    }
    return flag;
}

bool NpuModuleMemProcessor::ProcessSingleDevice(const std::string& devicePath, LocaltimeContext& localtimeContext,
                                                std::vector<NpuModuleMemData>& allProcessedData,
                                                SyscntConversionParams& params)
{
    DBInfo npuModuleMemDB("npu_module_mem.db", "NpuModuleMem");
    if (localtimeContext.deviceId == INVALID_DEVICE_ID)
    {
        ERROR("the invalid deviceId cannot to be identified, profPath is %", profPath_);
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(localtimeContext.timeRecord, profPath_,
                                                      localtimeContext.deviceId))
    {
        ERROR("Failed to obtain the time in start_info and end_info, profPath is %.", profPath_);
        return false;
    }
    std::string dbPath = File::PathJoin({devicePath, SQLITE, npuModuleMemDB.dbName});
    if (!npuModuleMemDB.ConstructDBRunner(dbPath) || npuModuleMemDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    // 并不是所有场景都有NpuModuleMem数据
    auto status = CheckPathAndTable(dbPath, npuModuleMemDB);
    if (status != CHECK_SUCCESS)
    {
        return status != CHECK_FAILED;
    }
    OriNpuModuleFormat oriData = LoadData(npuModuleMemDB);
    if (oriData.empty())
    {
        ERROR("Get % data failed in %.", npuModuleMemDB.tableName, dbPath);
        return false;
    }
    auto processedData = FormatData(oriData, localtimeContext, params);
    if (processedData.empty())
    {
        ERROR("Format NpuModuleMem data error, dbPath is %.", dbPath);
        return false;
    }
    FilterDataByStartTime(processedData, localtimeContext.timeRecord.startTimeNs, PROCESSOR_NAME_NPU_MODULE_MEM);
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());
    return true;
}

OriNpuModuleFormat NpuModuleMemProcessor::LoadData(const DBInfo& npuModuleMemDB)
{
    OriNpuModuleFormat oriData;
    std::string sql{"SELECT module_id, syscnt, total_size, device_type FROM " + npuModuleMemDB.tableName};
    if (!npuModuleMemDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", npuModuleMemDB.tableName);
    }
    return oriData;
}

std::vector<NpuModuleMemData> NpuModuleMemProcessor::FormatData(const OriNpuModuleFormat& oriData,
                                                                const LocaltimeContext& localtimeContext,
                                                                SyscntConversionParams& params)
{
    std::vector<NpuModuleMemData> processedData;
    if (!Reserve(processedData, oriData.size()))
    {
        ERROR("Reserve for NpuModuleMem data failed.");
        return processedData;
    }
    NpuModuleMemData data;
    data.deviceId = localtimeContext.deviceId;
    for (auto& row : oriData)
    {
        double syscnt = 0.0;
        std::tie(data.moduleId, syscnt, data.totalReserved, data.deviceType) = row;
        HPFloat timestamp{GetTimeFromHostCnt(static_cast<uint64_t>(syscnt), params)};
        data.timestamp = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        processedData.push_back(data);
    }
    return processedData;
}
}  // namespace Domain
}  // namespace Analysis
