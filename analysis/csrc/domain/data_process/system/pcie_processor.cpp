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
#include "analysis/csrc/domain/data_process/system/pcie_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
namespace
{
// 将 BandwidthData 的所有 uint32_t 属性乘以 1000
void ConvertMicrosecToSec(BandwidthData& data)
{
    data.min *= MICRO_SECOND;
    data.max *= MICRO_SECOND;
    data.avg *= MICRO_SECOND;
}

// 处理 PCIeData 中的所有 BandwidthData 类型的成员, B/us -> B/s
void ConvertBandwidthData(PCIeData& data)
{
    // 无txNonpostLatency
    ConvertMicrosecToSec(data.txPost);
    ConvertMicrosecToSec(data.txNonpost);
    ConvertMicrosecToSec(data.txCpl);
    ConvertMicrosecToSec(data.rxPost);
    ConvertMicrosecToSec(data.rxNonpost);
    ConvertMicrosecToSec(data.rxCpl);
}
}  // namespace

PCIeProcessor::PCIeProcessor(const std::string& profPaths) : DataProcessor(profPaths) {}

bool PCIeProcessor::Process(DataInventory& dataInventory)
{
    INFO("PCIeProcessor Process, dir is %", profPath_);
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<PCIeData> res;
    bool flag = true;
    for (const auto& devicePath : deviceList)
    {
        flag = ProcessOneDevice(devicePath, res) && flag;
    }
    if (!SaveToDataInventory<PCIeData>(std::move(res), dataInventory, PROCESSOR_NAME_PCIE))
    {
        flag = false;
        ERROR("Save data failed, %.", PROCESSOR_NAME_PCIE);
    }
    return flag;
}

bool PCIeProcessor::ProcessOneDevice(const std::string& devicePath, std::vector<PCIeData>& res)
{
    LocaltimeContext localtimeContext;
    uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
    if (deviceId == INVALID_DEVICE_ID)
    {
        ERROR("the invalid deviceId cannot to be identified.");
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(localtimeContext.timeRecord, profPath_, deviceId))
    {
        ERROR("Failed to GetProfTimeRecordInfo, fileDir is %, device id is %.", profPath_, deviceId);
        return false;
    }
    if (!Context::GetInstance().GetClockMonotonicRaw(localtimeContext.hostMonotonic, true, deviceId, profPath_) ||
        !Context::GetInstance().GetClockMonotonicRaw(localtimeContext.deviceMonotonic, false, deviceId, profPath_))
    {
        ERROR("Device MonotonicRaw is invalid in path: %., device id is %", profPath_, deviceId);
        return false;
    }
    DBInfo pcieDB("pcie.db", "PcieOriginalData");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, pcieDB.dbName});
    pcieDB.ConstructDBRunner(dbPath);
    auto status = CheckPathAndTable(dbPath, pcieDB);
    if (status != CHECK_SUCCESS)
    {
        return status != CHECK_FAILED;
    }
    PCIeDataFormat pcieData = LoadData(dbPath, pcieDB);
    std::vector<PCIeData> processedData;
    if (!FormatData(localtimeContext, pcieData, processedData))
    {
        ERROR("FormatData failed, fileDir is %.", profPath_);
        return false;
    }
    FilterDataByStartTime(processedData, localtimeContext.timeRecord.startTimeNs, PROCESSOR_NAME_PCIE);
    res.insert(res.end(), processedData.begin(), processedData.end());
    return true;
}

PCIeDataFormat PCIeProcessor::LoadData(const std::string& dbPath, const Analysis::Infra::DBInfo& pcieDB)
{
    INFO("PCIeProcessor GetData, dir is %.", dbPath);
    PCIeDataFormat pcieData;
    if (pcieDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return pcieData;
    }
    std::string sql =
        "SELECT device_id, timestamp, tx_p_bandwidth_min, tx_p_bandwidth_max, tx_p_bandwidth_avg, "
        "tx_np_bandwidth_min, tx_np_bandwidth_max, tx_np_bandwidth_avg, tx_cpl_bandwidth_min, "
        "tx_cpl_bandwidth_max, tx_cpl_bandwidth_avg, tx_np_latency_min, tx_np_latency_max, "
        "tx_np_latency_avg, rx_p_bandwidth_min, rx_p_bandwidth_max, rx_p_bandwidth_avg, "
        "rx_np_bandwidth_min, rx_np_bandwidth_max, rx_np_bandwidth_avg, rx_cpl_bandwidth_min, "
        "rx_cpl_bandwidth_max, rx_cpl_bandwidth_avg "
        "FROM " +
        pcieDB.tableName + " WHERE tx_p_bandwidth_max >= tx_p_bandwidth_min";
    if (!pcieDB.dbRunner->QueryData(sql, pcieData))
    {
        ERROR("Query PCIe data failed, db path is %.", dbPath);
        return pcieData;
    }
    return pcieData;
}

bool PCIeProcessor::FormatData(const LocaltimeContext& localtimeContext, const PCIeDataFormat& pcieData,
                               std::vector<PCIeData>& processedData)
{
    INFO("PCIeProcessor FormatData.");
    if (pcieData.empty())
    {
        ERROR("Pcie original data is empty.");
        return false;
    }
    if (!Reserve(processedData, pcieData.size()))
    {
        ERROR("Reserve for PCIe data failed.");
        return false;
    }
    PCIeData tempData;
    for (const auto& data : pcieData)
    {
        uint16_t deviceId;
        uint64_t oriTimestamp;
        std::tie(tempData.deviceId, oriTimestamp, tempData.txPost.min, tempData.txPost.max, tempData.txPost.avg,
                 tempData.txNonpost.min, tempData.txNonpost.max, tempData.txNonpost.avg, tempData.txCpl.min,
                 tempData.txCpl.max, tempData.txCpl.avg, tempData.txNonpostLatency.min, tempData.txNonpostLatency.max,
                 tempData.txNonpostLatency.avg, tempData.rxPost.min, tempData.rxPost.max, tempData.rxPost.avg,
                 tempData.rxNonpost.min, tempData.rxNonpost.max, tempData.rxNonpost.avg, tempData.rxCpl.min,
                 tempData.rxCpl.max, tempData.rxCpl.avg) = data;
        HPFloat timestamp = GetTimeBySamplingTimestamp(
            static_cast<double>(oriTimestamp), localtimeContext.hostMonotonic, localtimeContext.deviceMonotonic);
        tempData.timestamp = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        // 时延单位本身即为ns, 带宽单位 B/us -> B/s
        ConvertBandwidthData(tempData);
        processedData.push_back(tempData);
    }
    if (processedData.empty())
    {
        ERROR("PCIe data processing error.");
        return false;
    }
    return true;
}
}  // namespace Domain
}  // namespace Analysis
