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
#include "analysis/csrc/domain/data_process/system/npu_op_mem_processor.h"

#include "analysis/csrc/application//credential/id_pool.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Application::Credential;
namespace
{
const std::string RELEASE = "release";
const std::string ALLOCATE = "allocate";
}  // namespace

NpuOpMemProcessor::NpuOpMemProcessor(const std::string &profPath) : DataProcessor(profPath)
{
    stringReleaseId_ = IdPool::GetInstance().GetUint64Id(RELEASE);
    stringAllocateId_ = IdPool::GetInstance().GetUint64Id(ALLOCATE);
}

NpuOpMemProcessor::OriDataFormat NpuOpMemProcessor::LoadData(const DBInfo &npuOpMemDB)
{
    OriDataFormat oriData;
    std::string sql{
        "SELECT operator, addr, size, timestamp, thread_id, total_allocate_memory, total_reserve_memory, "
        "device_type FROM " +
        npuOpMemDB.tableName};
    if (!npuOpMemDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", npuOpMemDB.tableName);
    }
    return oriData;
}

std::vector<NpuOpMemData> NpuOpMemProcessor::FormatData(const OriDataFormat &oriData,
                                                        const Utils::ProfTimeRecord &timeRecord,
                                                        Utils::SyscntConversionParams &params, GeHashMap &hashMap) const
{
    std::vector<NpuOpMemData> formatData;
    if (!Reserve(formatData, oriData.size()))
    {
        ERROR("Reserve for NpuOpMem data failed, profPath is %.", profPath_);
        return formatData;
    }
    NpuOpMemData tempData;
    std::string addr;
    double oriTimestamp;
    std::string deviceType;
    for (auto &row : oriData)
    {
        std::tie(tempData.operatorName, addr, tempData.size, oriTimestamp, tempData.threadId,
                 tempData.totalAllocateMemory, tempData.totalReserveMemory, deviceType) = row;
        HPFloat timestamp{GetTimeFromSyscnt(static_cast<uint64_t>(oriTimestamp), params)};
        if (tempData.size < 0)
        {
            tempData.size = std::abs(tempData.size);
            tempData.type = stringReleaseId_;
        }
        else
        {
            tempData.type = stringAllocateId_;
        }
        if (Utils::StrToU64(tempData.addr, addr) == ANALYSIS_ERROR)
        {  // 转uint64失败默认值为UINT64_MAX
            WARN("Converting string(addr) to integer failed.");
        }
        auto it = hashMap.find(tempData.operatorName);
        // hash 可能为空，不能直接使用。如果有数据才取用
        if (it != hashMap.end())
        {
            tempData.operatorName = it->second;
        }
        tempData.timestamp = GetLocalTime(timestamp, timeRecord).Uint64();
        tempData.deviceId = GetDeviceId(deviceType);
        formatData.push_back(tempData);
    }
    return formatData;
}

bool NpuOpMemProcessor::Process(DataInventory &dataInventory)
{
    Utils::ProfTimeRecord timeRecord;
    Utils::SyscntConversionParams params;
    DBInfo npuOpMemDB("task_memory.db", "NpuOpMemRaw");
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, npuOpMemDB.dbName});
    if (!npuOpMemDB.ConstructDBRunner(dbPath) || npuOpMemDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    // 并不是所有场景都有NpuOpMemRaw数据
    auto status = CheckPathAndTable(dbPath, npuOpMemDB);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    if (!Context::GetInstance().GetSyscntConversionParams(params, HOST_ID, profPath_))
    {
        ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(timeRecord, profPath_))
    {
        ERROR("Failed to obtain the time in start_info and end_info.");
        return false;
    }
    auto oriData = LoadData(npuOpMemDB);
    if (oriData.empty())
    {
        ERROR("Get % data failed in %.", npuOpMemDB.tableName, dbPath);
        return false;
    }
    auto hashMapPtr = dataInventory.GetPtr<GeHashMap>();
    if (hashMapPtr == nullptr)
    {
        ERROR("Can't get hash data.");
        return false;
    }
    std::vector<NpuOpMemData> processedData = FormatData(oriData, timeRecord, params, *hashMapPtr);
    if (processedData.empty())
    {
        ERROR("Format NpuOpMem data error, dbPath is %.", dbPath);
        return false;
    }
    FilterDataByStartTime(processedData, timeRecord.startTimeNs, PROCESSOR_NAME_NPU_OP_MEM);
    if (!SaveToDataInventory<NpuOpMemData>(std::move(processedData), dataInventory, PROCESSOR_NAME_NPU_OP_MEM))
    {
        ERROR("Save NpuOpMem Data To DataInventory failed, profPath is %.", profPath_);
        return false;
    }
    return true;
}

uint16_t NpuOpMemProcessor::GetDeviceId(const std::string &deviceType)
{
    std::size_t pos = deviceType.find(':');
    if (pos != std::string::npos)
    {
        std::string numStr = deviceType.substr(pos + 1);
        uint16_t deviceId;
        if (StrToU16(deviceId, numStr) == ANALYSIS_OK)
        {
            return deviceId;
        }
        ERROR("Bad input: std::invalid_argument thrown");
    }
    return UINT16_MAX;  // 获取deviceId失败返回UINT16_MAX
}

}  // namespace Domain
}  // namespace Analysis
