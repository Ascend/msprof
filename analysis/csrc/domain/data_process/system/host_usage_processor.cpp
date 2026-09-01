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
#include "analysis/csrc/domain/data_process/system/host_usage_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
HostUsageProcessor::HostUsageProcessor(const std::string &profPath_, const std::string &processorName, DBInfo &&dbInfo)
    : usageDb_(dbInfo), processorName_(processorName), DataProcessor(profPath_)
{
}

bool HostUsageProcessor::Process(DataInventory &dataInventory)
{
    INFO("Start process %.", processorName_);
    ProfTimeRecord record;
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_))
    {
        ERROR("Failed to obtain the time in start_info and end_info.");
        return false;
    }
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, usageDb_.dbName});
    if (!usageDb_.ConstructDBRunner(dbPath) || usageDb_.dbRunner == nullptr)
    {
        return false;
    }
    auto status = CheckPathAndTable(dbPath, usageDb_);
    if (status == CHECK_FAILED)
    {
        return false;
    }
    else if (status == NOT_EXIST)
    {
        return true;
    }
    return ProcessData(dataInventory, record, dbPath);
}

HostCpuUsageProcessor::HostCpuUsageProcessor(const std::string &profPath_)
    : HostUsageProcessor(profPath_, PROCESSOR_NAME_CPU_USAGE, DBInfo("host_cpu_usage.db", "CpuUsage"))
{
}

bool HostCpuUsageProcessor::ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record,
                                        const std::string &dbPath)
{
    OriCpuUsage oriData;
    std::vector<CpuUsageData> res;
    std::string sql{"SELECT start_time, cpu_no, usage FROM " + usageDb_.tableName +
                    " ORDER BY CAST(cpu_no AS DECIMAL)"};
    if (!usageDb_.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query cpu usage data failed, db path is %.", dbPath);
    }
    if (oriData.empty())
    {
        ERROR("cpu usage original data is empty. db path is %", dbPath);
        return false;
    }
    if (!Reserve(res, oriData.size()))
    {
        ERROR("Reserved for cpu usage data failed.");
        return false;
    }
    CpuUsageData data;
    for (const auto &row : oriData)
    {
        std::tie(data.timestamp, data.cpuNo, data.usage) = row;
        HPFloat startTimestamp{data.timestamp};
        data.timestamp = GetLocalTime(startTimestamp, record).Uint64();
        res.push_back(data);
    }
    FilterDataByStartTime(res, record.startTimeNs, processorName_);
    if (!SaveToDataInventory<CpuUsageData>(std::move(res), dataInventory, processorName_))
    {
        ERROR("Save data failed, %.", processorName_);
        return false;
    }
    return true;
}

HostCpuFreqProcessor::HostCpuFreqProcessor(const std::string &profPath_)
    : HostUsageProcessor(profPath_, PROCESSOR_NAME_CPU_FREQ, DBInfo("host_cpu_freq.db", "CpuFreq"))
{
}

bool HostCpuFreqProcessor::ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record,
                                       const std::string &dbPath)
{
    OriCpuFreq oriData;
    std::vector<CpuFreqData> res;
    std::string sql{"SELECT timestamp, cpu_no, freq FROM " + usageDb_.tableName +
                    " ORDER BY CAST(cpu_no AS DECIMAL), timestamp"};
    if (!usageDb_.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query cpu freq data failed, db path is %.", dbPath);
    }
    if (oriData.empty())
    {
        ERROR("cpu freq original data is empty. db path is %", dbPath);
        return false;
    }
    if (!Reserve(res, oriData.size()))
    {
        ERROR("Reserved for cpu freq data failed.");
        return false;
    }
    CpuFreqData data;
    for (const auto &row : oriData)
    {
        std::tie(data.timestamp, data.cpuNo, data.freq) = row;
        if (!IsNumber(data.cpuNo))
        {
            WARN("Skip invalid cpu freq data because cpu_no is not numeric, cpu_no is %.", data.cpuNo);
            continue;
        }
        data.freq /= static_cast<double>(FREQ_TO_MHz);
        HPFloat startTimestamp{data.timestamp};
        data.timestamp = GetLocalTime(startTimestamp, record).Uint64();
        res.push_back(data);
    }
    FilterDataByStartTime(res, record.startTimeNs, processorName_);
    if (!SaveToDataInventory<CpuFreqData>(std::move(res), dataInventory, processorName_))
    {
        ERROR("Save data failed, %.", processorName_);
        return false;
    }
    return true;
}

HostMemUsageProcessor::HostMemUsageProcessor(const std::string &profPath_)
    : HostUsageProcessor(profPath_, PROCESSOR_NAME_MEM_USAGE, DBInfo("host_mem_usage.db", "MemUsage"))
{
}

bool HostMemUsageProcessor::ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record,
                                        const std::string &dbPath)
{
    OriMemUsage oriData;
    std::vector<MemUsageData> res;
    std::string sql{"SELECT start_time, usage FROM " + usageDb_.tableName};
    if (!usageDb_.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query mem usage data failed, db path is %.", dbPath);
    }
    if (oriData.empty())
    {
        ERROR("mem usage original data is empty. db path is %", dbPath);
        return false;
    }
    if (!Reserve(res, oriData.size()))
    {
        ERROR("Reserved for mem usage data failed.");
        return false;
    }
    MemUsageData data;
    for (const auto &row : oriData)
    {
        std::tie(data.timestamp, data.usage) = row;
        HPFloat startTimestamp{data.timestamp};
        data.timestamp = GetLocalTime(startTimestamp, record).Uint64();
        res.push_back(data);
    }
    FilterDataByStartTime(res, record.startTimeNs, processorName_);
    if (!SaveToDataInventory<MemUsageData>(std::move(res), dataInventory, processorName_))
    {
        ERROR("Save data failed, %.", processorName_);
        return false;
    }
    return true;
}

HostDiskUsageProcessor::HostDiskUsageProcessor(const std::string &profPath_)
    : HostUsageProcessor(profPath_, PROCESSOR_NAME_DISK_USAGE, DBInfo("host_disk_usage.db", "DiskUsage"))
{
}

bool HostDiskUsageProcessor::ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record,
                                         const std::string &dbPath)
{
    OriDiskUsage oriData;
    std::vector<DiskUsageData> res;
    std::string sql{"SELECT start_time, usage, disk_read, disk_write FROM " + usageDb_.tableName};
    if (!usageDb_.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query disk usage data failed, db path is %.", dbPath);
    }
    if (oriData.empty())
    {
        ERROR("disk usage original data is empty. db path is %", dbPath);
        return false;
    }
    if (!Reserve(res, oriData.size()))
    {
        ERROR("Reserved for disk usage data failed.");
        return false;
    }
    DiskUsageData data;
    for (const auto &row : oriData)
    {
        std::tie(data.timestamp, data.usage, data.readRate, data.writeRate) = row;
        HPFloat startTimestamp{data.timestamp};
        data.timestamp = GetLocalTime(startTimestamp, record).Uint64();
        res.push_back(data);
    }
    FilterDataByStartTime(res, record.startTimeNs, processorName_);
    if (!SaveToDataInventory<DiskUsageData>(std::move(res), dataInventory, processorName_))
    {
        ERROR("Save data failed, %.", processorName_);
        return false;
    }
    return true;
}

HostNetworkUsageProcessor::HostNetworkUsageProcessor(const std::string &profPath_)
    : HostUsageProcessor(profPath_, PROCESSOR_NAME_NETWORK_USAGE, DBInfo("host_network_usage.db", "NetworkUsage"))
{
}

bool HostNetworkUsageProcessor::ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record,
                                            const std::string &dbPath)
{
    OriNetworkUsage oriData;
    std::vector<NetWorkUsageData> res;
    std::string sql{"SELECT start_time, usage, speed FROM " + usageDb_.tableName};
    if (!usageDb_.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query network usage data failed, db path is %.", dbPath);
    }
    if (oriData.empty())
    {
        ERROR("network usage original data is empty. db path is %", dbPath);
        return false;
    }
    if (!Reserve(res, oriData.size()))
    {
        ERROR("Reserved for network usage data failed.");
        return false;
    }
    NetWorkUsageData data;
    for (const auto &row : oriData)
    {
        std::tie(data.timestamp, data.usage, data.speed) = row;
        HPFloat startTimestamp{data.timestamp};
        data.timestamp = GetLocalTime(startTimestamp, record).Uint64();
        res.push_back(data);
    }
    FilterDataByStartTime(res, record.startTimeNs, processorName_);
    if (!SaveToDataInventory<NetWorkUsageData>(std::move(res), dataInventory, processorName_))
    {
        ERROR("Save data failed, %.", processorName_);
        return false;
    }
    return true;
}

OSRuntimeApiProcessor::OSRuntimeApiProcessor(const std::string &profPath_)
    : HostUsageProcessor(profPath_, PROCESSOR_NAME_OSRT_API, DBInfo("host_runtime_api.db", "Syscall"))
{
}

bool OSRuntimeApiProcessor::ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record,
                                        const std::string &dbPath)
{
    OriSysCall oriData;
    std::vector<OSRuntimeApiData> res;
    std::string sql{
        "SELECT runtime_api_name, runtime_pid, runtime_tid, runtime_trans_start, runtime_trans_end "
        "FROM " +
        usageDb_.tableName};
    if (!usageDb_.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query os runtime api data failed, db path is %.", dbPath);
    }
    if (oriData.empty())
    {
        ERROR("os runtime api original data is empty. db path is %", dbPath);
        return false;
    }
    if (!Reserve(res, oriData.size()))
    {
        ERROR("Reserved for runtime api failed.");
        return false;
    }
    OSRuntimeApiData data;
    for (const auto &row : oriData)
    {
        std::tie(data.name, data.pid, data.tid, data.timestamp, data.endTime) = row;
        HPFloat startTimestamp{data.timestamp};
        data.timestamp = GetLocalTime(startTimestamp, record).Uint64();
        HPFloat endTimestamp{data.endTime};
        data.endTime = GetLocalTime(endTimestamp, record).Uint64();
        res.push_back(data);
    }
    FilterDataByStartTime(res, record.startTimeNs, processorName_);
    if (!SaveToDataInventory<OSRuntimeApiData>(std::move(res), dataInventory, processorName_))
    {
        ERROR("Save data failed, %.", processorName_);
        return false;
    }
    return true;
}
}  // namespace Domain
}  // namespace Analysis
