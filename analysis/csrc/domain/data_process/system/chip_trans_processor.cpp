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

#include "chip_trans_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;
using namespace Environment;

ChipTransProcessor::ChipTransProcessor(const std::string& profPaths) : DataProcessor(profPaths) {}

bool ChipTransProcessor::Process(DataInventory& dataInventory)
{
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    bool flag = true;
    ChipTransData chipTransData;
    auto version = Context::GetInstance().GetPlatformVersion(DEFAULT_DEVICE_ID, profPath_);
    if (Context::GetInstance().IsChipV6(version))
    {
        ChipTransV6Processor chipTransV6Processor(profPath_);
        return chipTransV6Processor.Process(dataInventory);
    }
    for (const auto& devicePath : deviceList)
    {
        if (!ProcessOneDevice(devicePath, chipTransData))
        {
            flag = false;
        }
    }
    if (!SaveToDataInventory<PaLinkInfoData>(std::move(chipTransData.resPaData), dataInventory,
                                             TABLE_NAME_PA_LINK_INFO))
    {
        ERROR("Save data failed, %.", TABLE_NAME_PA_LINK_INFO);
        return false;
    }
    if (!SaveToDataInventory<PcieInfoData>(std::move(chipTransData.resPcieData), dataInventory, TABLE_NAME_PCIE_INFO))
    {
        ERROR("Save data failed, %.", TABLE_NAME_PCIE_INFO);
        return false;
    }
    return flag;
}

bool ChipTransProcessor::ProcessOneDevice(const std::string& devicePath, ChipTransData& chipTransData)
{
    auto deviceId = GetDeviceIdByDevicePath(devicePath);
    if (!Context::GetInstance().GetProfTimeRecordInfo(chipTransData.timeRecord, profPath_, deviceId))
    {
        ERROR("Failed to obtain the time in start_info and end_info. Path is %, device id is %.", profPath_, deviceId);
        return false;
    }
    DBInfo paLinkInfo("chip_trans.db", "PaLinkInfo");
    DBInfo pcieInfo("chip_trans.db", "PcieInfo");
    std::string paLinkDBPath = Utils::File::PathJoin({devicePath, SQLITE, paLinkInfo.dbName});
    std::string pcieDBPath = Utils::File::PathJoin({devicePath, SQLITE, pcieInfo.dbName});
    if (!paLinkInfo.ConstructDBRunner(paLinkDBPath) || !pcieInfo.ConstructDBRunner(pcieDBPath))
    {
        return false;
    }
    bool flag = true;
    auto status = CheckPathAndTable(paLinkDBPath, paLinkInfo);
    if (status == CHECK_SUCCESS)
    {
        chipTransData.oriPaData = LoadPaData(paLinkInfo);
    }
    else if (status == CHECK_FAILED)
    {
        flag = false;
    }

    status = CheckPathAndTable(pcieDBPath, pcieInfo);
    if (status == CHECK_SUCCESS)
    {
        chipTransData.oriPcieData = LoadPcieData(pcieInfo);
    }
    else if (status == CHECK_FAILED)
    {
        flag = false;
    }

    std::vector<PaLinkInfoData> paData;
    std::vector<PcieInfoData> pcieData;
    if (!FormatData(paData, pcieData, chipTransData, deviceId))
    {
        ERROR("Format data failed, %.", PROCESSOR_NAME_CHIP_TRAINS);
        return false;
    }
    FilterDataByStartTime(paData, chipTransData.timeRecord.startTimeNs, TABLE_NAME_PA_LINK_INFO);
    chipTransData.resPaData.insert(chipTransData.resPaData.end(), paData.begin(), paData.end());

    FilterDataByStartTime(pcieData, chipTransData.timeRecord.startTimeNs, TABLE_NAME_PCIE_INFO);
    chipTransData.resPcieData.insert(chipTransData.resPcieData.end(), pcieData.begin(), pcieData.end());
    return flag;
}

OriPaFormat ChipTransProcessor::LoadPaData(const DBInfo& paLinkInfo)
{
    OriPaFormat oriPaData;
    std::string sql{
        "SELECT pa_link_id, pa_link_traffic_monit_rx, pa_link_traffic_monit_tx, sys_time "
        "FROM " +
        paLinkInfo.tableName};
    if (!paLinkInfo.dbRunner->QueryData(sql, oriPaData))
    {
        ERROR("Failed to obtain data from the % table.", paLinkInfo.tableName);
    }
    return oriPaData;
}

OriPcieFormat ChipTransProcessor::LoadPcieData(const DBInfo& pcieInfo)
{
    OriPcieFormat oriPcieData;
    std::string sql{
        "SELECT pcie_id, pcie_write_bandwidth, pcie_read_bandwidth, sys_time "
        "FROM " +
        pcieInfo.tableName};
    if (!pcieInfo.dbRunner->QueryData(sql, oriPcieData))
    {
        ERROR("Failed to obtain data from the % table.", pcieInfo.tableName);
    }
    return oriPcieData;
}

bool ChipTransProcessor::FormatData(std::vector<PaLinkInfoData>& paFormatData,
                                    std::vector<PcieInfoData>& pcieFormatData, ChipTransData& chipTransData,
                                    const uint16_t& deviceId)
{
    PaLinkInfoData paLinkInfoData;
    PcieInfoData pcieInfoData;
    paLinkInfoData.deviceId = deviceId;
    paLinkInfoData.deviceId = deviceId;
    pcieInfoData.deviceId = deviceId;
    if (!Reserve(paFormatData, chipTransData.resPaData.size()) ||
        !Reserve(pcieFormatData, chipTransData.resPcieData.size()))
    {
        ERROR("Reserve for Chip trains data failed.");
        return false;
    }
    for (auto& row : chipTransData.oriPaData)
    {
        uint64_t sysTime;
        std::tie(paLinkInfoData.paLinkId, paLinkInfoData.paLinkTrafficMonitRx, paLinkInfoData.paLinkTrafficMonitTx,
                 sysTime) = row;
        HPFloat timestamp(sysTime);
        paLinkInfoData.timestamp = GetLocalTime(timestamp, chipTransData.timeRecord).Uint64();
        paFormatData.push_back(paLinkInfoData);
    }
    for (auto& row : chipTransData.oriPcieData)
    {
        uint64_t sysTime;
        std::tie(pcieInfoData.pcieId, pcieInfoData.pcieWriteBandwidth, pcieInfoData.pcieReadBandwidth, sysTime) = row;
        HPFloat timestamp(sysTime);
        pcieInfoData.timestamp = GetLocalTime(timestamp, chipTransData.timeRecord).Uint64();
        pcieFormatData.push_back(pcieInfoData);
    }
    return true;
}

}  // namespace Domain
}  // namespace Analysis
