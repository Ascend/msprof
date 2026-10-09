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

#include "analysis/csrc/domain/data_process/system/netdev_stats_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

namespace
{
constexpr size_t MIN_RECORD_NUM = 2;
inline void UnpackNetDevStatsData(const OriNetDevStatsData &oriData, NetDevStatsData &data)
{
    std::tie(data.timestamp, data.macTxPfcPkt, data.macRxPfcPkt, data.macTxTotalOct, data.macRxTotalOct,
             data.macTxBadOct, data.macRxBadOct, data.roceTxAllPkt, data.roceRxAllPkt, data.roceTxErrPkt,
             data.roceRxErrPkt, data.roceTxCnpPkt, data.roceRxCnpPkt, data.roceNewPktRty, data.nicTxAllOct,
             data.nicRxAllOct) = oriData;
}

inline uint64_t CheckAndComputeDiff(uint64_t curVal, uint64_t preVal)
{
    return (curVal >= preVal) ? (curVal - preVal) : 0;
}
}  // namespace

NetDevStatsProcessor::NetDevStatsProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool NetDevStatsProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    ProfTimeRecord record;
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_))
    {
        ERROR("Failed to obtain the time in start_info and end_info.");
        return false;
    }
    std::vector<NetDevStatsEventData> allEventData;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        flag = ProcessSingleDevice(devicePath, allEventData, record) && flag;
    }
    if (!SaveToDataInventory<NetDevStatsEventData>(std::move(allEventData), dataInventory, PROCESSOR_NAME_NETDEV_STATS))
    {
        flag = false;
        ERROR("Save NetDevStats Data To DataInventory failed, profPath is %.", profPath_);
    }
    return flag;
}

bool NetDevStatsProcessor::ProcessSingleDevice(const std::string &devicePath,
                                               std::vector<NetDevStatsEventData> &allEventData,
                                               const ProfTimeRecord &record)
{
    auto deviceId = GetDeviceIdByDevicePath(devicePath);
    if (deviceId == INVALID_DEVICE_ID)
    {
        ERROR("the invalid deviceId cannot to be identified, profPath is %.", profPath_);
        return false;
    }
    DBInfo netDevStatsDB("netdev_stats.db", "NetDevStatsOriginalData");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, netDevStatsDB.dbName});
    if (!netDevStatsDB.ConstructDBRunner(dbPath) || netDevStatsDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    auto status = CheckPathAndTable(dbPath, netDevStatsDB);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    auto processedData = ProcessData(netDevStatsDB, deviceId, record);
    if (processedData.empty())
    {
        ERROR("Format NetDevStats data error, dbPath is %.", dbPath);
        return false;
    }
    FilterDataByStartTime(processedData, record.startTimeNs, PROCESSOR_NAME_NETDEV_STATS);
    allEventData.insert(allEventData.end(), processedData.begin(), processedData.end());
    return true;
}

std::vector<NetDevStatsEventData> NetDevStatsProcessor::ProcessData(const DBInfo &netDevStatsDB, uint16_t deviceId,
                                                                    const ProfTimeRecord &record)
{
    std::vector<OriNetDevStatsData> oriData;
    std::string sql{
        "SELECT timestamp, mac_tx_pfc_pkt, mac_rx_pfc_pkt, mac_tx_total_oct, mac_rx_total_oct,"
        "mac_tx_bad_oct, mac_rx_bad_oct, roce_tx_all_pkt, roce_rx_all_pkt, roce_tx_err_pkt, roce_rx_err_pkt,"
        "roce_tx_cnp_pkt, roce_rx_cnp_pkt, roce_new_pkt_rty, nic_tx_all_oct, nic_rx_all_oct FROM " +
        netDevStatsDB.tableName + " ORDER BY timestamp"};
    if (!netDevStatsDB.dbRunner->QueryData(sql, oriData) || oriData.size() < MIN_RECORD_NUM)
    {
        ERROR("Get % data failed, profPath is %, device is %", netDevStatsDB.tableName, profPath_, deviceId);
        return {};
    }
    return ComputeEventData(oriData, deviceId, record);
}

std::vector<NetDevStatsEventData> NetDevStatsProcessor::ComputeEventData(const std::vector<OriNetDevStatsData> &oriData,
                                                                         uint16_t deviceId,
                                                                         const ProfTimeRecord &record)
{
    std::vector<NetDevStatsEventData> rstEventData;
    if (!Reserve(rstEventData, oriData.size() - 1))
    {
        ERROR("Reserve for NetDevStats data failed, profPath is %, deviceId is %.", profPath_, deviceId);
        return rstEventData;
    }
    NetDevStatsData preRecord;
    NetDevStatsData curRecord;
    UnpackNetDevStatsData(oriData[0], preRecord);
    NetDevStatsEventData eventData;
    eventData.deviceId = deviceId;
    for (size_t i = 1; i < oriData.size(); ++i)
    {
        UnpackNetDevStatsData(oriData[i], curRecord);
        if (curRecord.timestamp <= preRecord.timestamp)
        {
            ERROR("The timestamp of NetDevStatsData is not increasing, profPath is %", profPath_);
            continue;
        }
        double duration = (curRecord.timestamp - preRecord.timestamp) / NS_TO_S;
        eventData.macTxPfcPkt = CheckAndComputeDiff(curRecord.macTxPfcPkt, preRecord.macTxPfcPkt);
        eventData.macRxPfcPkt = CheckAndComputeDiff(curRecord.macRxPfcPkt, preRecord.macRxPfcPkt);
        eventData.macTxByte = CheckAndComputeDiff(curRecord.macTxTotalOct, preRecord.macTxTotalOct);
        eventData.macTxBandwidth = eventData.macTxByte / duration;
        eventData.macRxByte = CheckAndComputeDiff(curRecord.macRxTotalOct, preRecord.macRxTotalOct);
        eventData.macRxBandwidth = eventData.macRxByte / duration;
        eventData.macTxBadByte = CheckAndComputeDiff(curRecord.macTxBadOct, preRecord.macTxBadOct);
        eventData.macRxBadByte = CheckAndComputeDiff(curRecord.macRxBadOct, preRecord.macRxBadOct);
        eventData.roceTxPkt = CheckAndComputeDiff(curRecord.roceTxAllPkt, preRecord.roceTxAllPkt);
        eventData.roceRxPkt = CheckAndComputeDiff(curRecord.roceRxAllPkt, preRecord.roceRxAllPkt);
        eventData.roceTxErrPkt = CheckAndComputeDiff(curRecord.roceTxErrPkt, preRecord.roceTxErrPkt);
        eventData.roceRxErrPkt = CheckAndComputeDiff(curRecord.roceRxErrPkt, preRecord.roceRxErrPkt);
        eventData.roceTxCnpPkt = CheckAndComputeDiff(curRecord.roceTxCnpPkt, preRecord.roceTxCnpPkt);
        eventData.roceRxCnpPkt = CheckAndComputeDiff(curRecord.roceRxCnpPkt, preRecord.roceRxCnpPkt);
        eventData.roceNewPktRty = CheckAndComputeDiff(curRecord.roceNewPktRty, preRecord.roceNewPktRty);
        eventData.nicTxByte = CheckAndComputeDiff(curRecord.nicTxAllOct, preRecord.nicTxAllOct);
        eventData.nicTxBandwidth = eventData.nicTxByte / duration;
        eventData.nicRxByte = CheckAndComputeDiff(curRecord.nicRxAllOct, preRecord.nicRxAllOct);
        eventData.nicRxBandwidth = eventData.nicRxByte / duration;
        HPFloat timestamp{curRecord.timestamp};
        eventData.timestamp = GetLocalTime(timestamp, record).Uint64();
        rstEventData.push_back(eventData);
        std::swap(preRecord, curRecord);
    }
    return rstEventData;
}
}  // namespace Domain
}  // namespace Analysis
