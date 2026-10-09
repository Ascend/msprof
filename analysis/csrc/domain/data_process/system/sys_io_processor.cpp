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
#include "analysis/csrc/domain/data_process/system/sys_io_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

namespace
{
const std::unordered_map<std::string, std::tuple<std::string, std::string, std::string>> ORI_DB_INFO_TABLE = {
    {PROCESSOR_NAME_NIC, std::make_tuple("nic.db", "NicOriginalData", "NicReportData")},
    {PROCESSOR_NAME_ROCE, std::make_tuple("roce.db", "RoceOriginalData", "RoceReportData")},
};

const std::unordered_map<std::string, std::tuple<std::string, std::string>> RECEIVE_SEND_DB_INFO_TABLE = {
    {PROCESSOR_NAME_NIC_TIMELINE, std::make_tuple("nicreceivesend_table.db", "NicReceiveSend")},
    {PROCESSOR_NAME_ROCE_TIMELINE, std::make_tuple("rocereceivesend_table.db", "RoceReceiveSend")},
};
}  // namespace

SysIOProcessor::SysIOProcessor(const std::string &profPath, const std::string &processorName)
    : DataProcessor(profPath), processorName_(processorName)
{
}

bool SysIOProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    std::vector<SysIOOriginalData> allProcessedData;
    std::vector<SysIOReportData> allSummaryData;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        flag = ProcessSingleDevice(devicePath, allProcessedData, allSummaryData) && flag;
    }
    return SaveData(allProcessedData, allSummaryData, dataInventory) && flag;
}

bool SysIOProcessor::SaveData(std::vector<SysIOOriginalData> &timelineData, std::vector<SysIOReportData> &summaryData,
                              DataInventory &dataInventory)
{
    bool flag = true;
    if (!timelineData.empty())
    {
        if (processorName_ == PROCESSOR_NAME_NIC)
        {
            std::vector<NicOriginalData> allNicOriginalData;
            NicOriginalData nicOriginalData;
            nicOriginalData.sysIOOriginalData = std::move(timelineData);
            allNicOriginalData.push_back(nicOriginalData);
            if (!SaveToDataInventory<NicOriginalData>(std::move(allNicOriginalData), dataInventory, processorName_))
            {
                flag = false;
                ERROR("Save NicOriginalData Data To DataInventory failed, profPath is %", profPath_);
            }
        }
        else if (processorName_ == PROCESSOR_NAME_ROCE)
        {
            std::vector<RoceOriginalData> allRoceOriginalData;
            RoceOriginalData roceOriginalData;
            roceOriginalData.sysIOOriginalData = std::move(timelineData);
            allRoceOriginalData.push_back(roceOriginalData);
            if (!SaveToDataInventory<RoceOriginalData>(std::move(allRoceOriginalData), dataInventory, processorName_))
            {
                flag = false;
                ERROR("Save RoceOriginalData Data To DataInventory failed, profPath is %", profPath_);
            }
        }
    }

    if (!summaryData.empty())
    {
        if (processorName_ == PROCESSOR_NAME_NIC)
        {
            std::vector<NicReportData> allNicReportData;
            NicReportData nicReportData;
            nicReportData.sysIOReportData = std::move(summaryData);
            allNicReportData.push_back(nicReportData);
            if (!SaveToDataInventory<NicReportData>(std::move(allNicReportData), dataInventory, processorName_))
            {
                flag = false;
                ERROR("Save NicReportData Data To DataInventory failed, profPath is %", profPath_);
            }
        }
        else if (processorName_ == PROCESSOR_NAME_ROCE)
        {
            std::vector<RoceReportData> allRoceReportData;
            RoceReportData roceReportData;
            roceReportData.sysIOReportData = std::move(summaryData);
            allRoceReportData.push_back(roceReportData);
            if (!SaveToDataInventory<RoceReportData>(std::move(allRoceReportData), dataInventory, processorName_))
            {
                flag = false;
                ERROR("Save RoceReportData Data To DataInventory failed, profPath is %", profPath_);
            }
        }
    }
    return flag;
}

bool SysIOProcessor::ProcessSingleDevice(const std::string &devicePath, std::vector<SysIOOriginalData> &timelineData,
                                         std::vector<SysIOReportData> &summaryData)
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
    std::string dbName;
    std::string oriTableName;
    std::string reportTableName;
    std::tie(dbName, oriTableName, reportTableName) = ORI_DB_INFO_TABLE.find(processorName_)->second;
    std::string dbPath = File::PathJoin({devicePath, SQLITE, dbName});
    DBInfo timelineDb(dbName, oriTableName);
    if (!timelineDb.ConstructDBRunner(dbPath) || timelineDb.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    bool flag = true;
    auto status = CheckPathAndTable(dbPath, timelineDb);
    if (status == CHECK_SUCCESS)
    {
        flag = ProcessTimelineData(timelineDb, localtimeContext, timelineData) && flag;
    }
    else if (status == CHECK_FAILED)
    {
        flag = false;
    }

    DBInfo summaryDb(dbName, reportTableName);
    if (!summaryDb.ConstructDBRunner(dbPath) || summaryDb.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    status = CheckPathAndTable(dbPath, summaryDb);
    if (status == CHECK_SUCCESS)
    {
        flag = ProcessSummaryData(summaryDb, localtimeContext, summaryData) && flag;
    }
    else if (status == CHECK_FAILED)
    {
        flag = false;
    }
    return flag;
}

bool SysIOProcessor::ProcessTimelineData(const DBInfo &sysIODB, LocaltimeContext &localtimeContext,
                                         std::vector<SysIOOriginalData> &timelineData)
{
    if (!Context::GetInstance().GetClockMonotonicRaw(localtimeContext.hostMonotonic, true, localtimeContext.deviceId,
                                                     profPath_) ||
        !Context::GetInstance().GetClockMonotonicRaw(localtimeContext.deviceMonotonic, false, localtimeContext.deviceId,
                                                     profPath_))
    {
        ERROR("Device MonotonicRaw is invalid in path: %., device id is %", profPath_, localtimeContext.deviceId);
        return false;
    }
    OriSysIOData oriData;
    std::string sql =
        "SELECT timestamp, bandwidth, rxpacket, rxbyte, rxpackets, rxbytes, rxerrors, "
        "rxdropped, txpacket, txbyte, txpackets, txbytes, txerrors, txdropped, funcid "
        "FROM " +
        sysIODB.tableName;
    if (!sysIODB.dbRunner->QueryData(sql, oriData) || oriData.empty())
    {
        ERROR("Failed to obtain data from the % table.", sysIODB.tableName);
        return false;
    }
    std::vector<SysIOOriginalData> processedData;
    if (!Reserve(processedData, oriData.size()))
    {
        ERROR("Reserve for % data failed, profPath is %, deviceId is %.", processorName_, profPath_,
              localtimeContext.deviceId);
        return false;
    }
    SysIOOriginalData tempData;
    tempData.deviceId = localtimeContext.deviceId;
    double oriTimestamp;
    for (const auto &row : oriData)
    {
        std::tie(oriTimestamp, tempData.bandwidth, tempData.rxPacketRate, tempData.rxByteRate, tempData.rxPackets,
                 tempData.rxBytes, tempData.rxErrors, tempData.rxDropped, tempData.txPacketRate, tempData.txByteRate,
                 tempData.txPackets, tempData.txBytes, tempData.txErrors, tempData.txDropped, tempData.funcId) = row;
        HPFloat timestamp =
            GetTimeBySamplingTimestamp(oriTimestamp, localtimeContext.hostMonotonic, localtimeContext.deviceMonotonic);
        tempData.timestamp = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        tempData.bandwidth = tempData.bandwidth * BYTE_SIZE * BYTE_SIZE,  // MB/s -> B/s
            processedData.push_back(tempData);
    }
    FilterDataByStartTime(processedData, localtimeContext.timeRecord.startTimeNs, processorName_);
    timelineData.insert(timelineData.end(), processedData.begin(), processedData.end());
    return true;
}

bool SysIOProcessor::ProcessSummaryData(const DBInfo &sysIODB, const LocaltimeContext &localtimeContext,
                                        std::vector<SysIOReportData> &summaryData)
{
    SysIOReportData tempData;
    OriSysIOReportData reportData;
    std::vector<SysIOReportData> processedData;
    std::string sql{
        "SELECT duration, bandwidth, rxbandwidth, rxpacket, rxerrorrate, rxdroppedrate, "
        "txbandwidth, txpacket, txerrorrate, txdroppedrate, funcid FROM " +
        sysIODB.tableName};
    if (!sysIODB.dbRunner->QueryData(sql, reportData) || reportData.empty())
    {
        ERROR("Failed to obtain throughput data from the % table, profPath is %, deviceId is %.", sysIODB.tableName,
              profPath_, localtimeContext.deviceId);
        return false;
    }
    tempData.deviceId = localtimeContext.deviceId;
    double duration;
    for (const auto &row : reportData)
    {
        std::tie(duration, tempData.bandwidth, tempData.rxBandwidthEfficiency, tempData.rxPacketRate,
                 tempData.rxErrorRate, tempData.rxDroppedRate, tempData.txBandwidthEfficiency, tempData.txPacketRate,
                 tempData.txErrorRate, tempData.txDroppedRate, tempData.funcId) = row;
        HPFloat timestamp = HPFloat(duration);
        tempData.timestamp = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        processedData.push_back(tempData);
    }
    FilterDataByStartTime(processedData, localtimeContext.timeRecord.startTimeNs, processorName_);
    summaryData.insert(summaryData.end(), processedData.begin(), processedData.end());
    return true;
}

NicProcessor::NicProcessor(const std::string &profPath) : SysIOProcessor(profPath, PROCESSOR_NAME_NIC) {}

RoCEProcessor::RoCEProcessor(const std::string &profPath) : SysIOProcessor(profPath, PROCESSOR_NAME_ROCE) {}

SysIOTimelineProcessor::SysIOTimelineProcessor(const std::string &profPath, const std::string &processorName)
    : DataProcessor(profPath), processorName_(processorName)
{
}

bool SysIOTimelineProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    std::vector<SysIOReceiveSendData> allProcessedData;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        flag = ProcessSingleDevice(devicePath, allProcessedData) && flag;
    }
    if (allProcessedData.empty())
    {
        WARN("No % data to save into dataInventory", processorName_);
        return flag;
    }
    if (processorName_ == PROCESSOR_NAME_NIC_TIMELINE)
    {
        std::vector<NicReceiveSendData> allNicReceiveSendData;
        NicReceiveSendData nicReceiveSendData;
        nicReceiveSendData.sysIOReceiveSendData = std::move(allProcessedData);
        allNicReceiveSendData.push_back(nicReceiveSendData);
        if (!SaveToDataInventory<NicReceiveSendData>(std::move(allNicReceiveSendData), dataInventory, processorName_))
        {
            flag = false;
            ERROR("Save % Data To DataInventory failed, profPath is %", processorName_, profPath_);
        }
    }
    else if (processorName_ == PROCESSOR_NAME_ROCE_TIMELINE)
    {
        std::vector<RoceReceiveSendData> allRoceReceiveSendData;
        RoceReceiveSendData roceReceiveSendData;
        roceReceiveSendData.sysIOReceiveSendData = std::move(allProcessedData);
        allRoceReceiveSendData.push_back(roceReceiveSendData);
        if (!SaveToDataInventory<RoceReceiveSendData>(std::move(allRoceReceiveSendData), dataInventory, processorName_))
        {
            flag = false;
            ERROR("Save % Data To DataInventory failed, profPath is %", processorName_, profPath_);
        }
    }
    return flag;
}

bool SysIOTimelineProcessor::ProcessSingleDevice(const std::string &devicePath,
                                                 std::vector<SysIOReceiveSendData> &allProcessedData)
{
    LocaltimeContext localtimeContext;
    localtimeContext.deviceId = GetDeviceIdByDevicePath(devicePath);
    if (!Context::GetInstance().GetProfTimeRecordInfo(localtimeContext.timeRecord, profPath_,
                                                      localtimeContext.deviceId))
    {
        ERROR(
            "Failed to obtain the time in start_info and end_info, "
            "profPath is %, device id is %.",
            profPath_, localtimeContext.deviceId);
        return false;
    }
    if (!Context::GetInstance().GetClockMonotonicRaw(localtimeContext.hostMonotonic, true, localtimeContext.deviceId,
                                                     profPath_) ||
        !Context::GetInstance().GetClockMonotonicRaw(localtimeContext.deviceMonotonic, false, localtimeContext.deviceId,
                                                     profPath_))
    {
        ERROR("Device MonotonicRaw is invalid in path: %., device id is %", profPath_, localtimeContext.deviceId);
        return false;
    }
    std::string oriDbName;
    std::string oriTableName;
    std::tie(oriDbName, oriTableName) = RECEIVE_SEND_DB_INFO_TABLE.find(processorName_)->second;
    DBInfo sysIOReceiveSendDB(oriDbName, oriTableName);
    std::string dbPath = File::PathJoin({devicePath, SQLITE, sysIOReceiveSendDB.dbName});
    if (!sysIOReceiveSendDB.ConstructDBRunner(dbPath) || sysIOReceiveSendDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    // 并不是所有场景都有sys io数据
    auto status = CheckPathAndTable(dbPath, sysIOReceiveSendDB);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    OriSysIOReceiveSendData oriData = LoadData(sysIOReceiveSendDB);
    if (oriData.empty())
    {
        ERROR("Load % ReceiveSendData error, dbPath is %.", processorName_, dbPath);
        return false;
    }
    auto processedData = FormatData(oriData, localtimeContext);
    if (processedData.empty())
    {
        ERROR("Format % ReceiveSendData error, dbPath is %.", processorName_, dbPath);
        return false;
    }
    FilterDataByStartTime(processedData, localtimeContext.timeRecord.startTimeNs, processorName_);
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());
    return true;
}

OriSysIOReceiveSendData SysIOTimelineProcessor::LoadData(const DBInfo &sysIOReceiveSendDB)
{
    OriSysIOReceiveSendData oriData;
    std::string sql{
        "SELECT timestamp, rx_bandwidth_efficiency, rx_packets, rx_error_rate, rx_dropped_rate, "
        "tx_bandwidth_efficiency, tx_packets, tx_error_rate, tx_dropped_rate, func_id FROM " +
        sysIOReceiveSendDB.tableName};
    if (!sysIOReceiveSendDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", sysIOReceiveSendDB.tableName);
    }
    return oriData;
}

std::vector<SysIOReceiveSendData> SysIOTimelineProcessor::FormatData(const OriSysIOReceiveSendData &oriData,
                                                                     const LocaltimeContext &localtimeContext)
{
    std::vector<SysIOReceiveSendData> formatData;
    if (!Reserve(formatData, oriData.size()))
    {
        ERROR("Reserve for Hccs data failed, profPath is %, deviceId is %.", profPath_, localtimeContext.deviceId);
        return formatData;
    }
    SysIOReceiveSendData tempData;
    tempData.deviceId = localtimeContext.deviceId;
    double oriTimestamp;
    for (const auto &row : oriData)
    {
        std::tie(oriTimestamp, tempData.rxBandwidthEfficiency, tempData.rxPacketRate, tempData.rxErrorRate,
                 tempData.rxDroppedRate, tempData.txBandwidthEfficiency, tempData.txPacketRate, tempData.txErrorRate,
                 tempData.txDroppedRate, tempData.funcId) = row;
        HPFloat timestamp =
            GetTimeBySamplingTimestamp(oriTimestamp, localtimeContext.hostMonotonic, localtimeContext.deviceMonotonic);
        tempData.timestamp = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        formatData.push_back(tempData);
    }
    return formatData;
}

NicTimelineProcessor::NicTimelineProcessor(const std::string &profPath)
    : SysIOTimelineProcessor(profPath, PROCESSOR_NAME_NIC_TIMELINE)
{
}

RoCETimelineProcessor::RoCETimelineProcessor(const std::string &profPath)
    : SysIOTimelineProcessor(profPath, PROCESSOR_NAME_ROCE_TIMELINE)
{
}

}  // namespace Domain
}  // namespace Analysis
