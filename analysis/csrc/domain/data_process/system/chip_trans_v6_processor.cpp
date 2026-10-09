/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include "chip_trans_v6_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;
using namespace Environment;

ChipTransV6Processor::ChipTransV6Processor(const std::string& profPaths) : DataProcessor(profPaths) {}

bool ChipTransV6Processor::Process(DataInventory& dataInventory)
{
    ChipTransV6Data chipTransV6Data;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    bool flag = true;
    for (const auto& devicePath : deviceList)
    {
        if (!ProcessOneDevice(devicePath, chipTransV6Data))
        {
            flag = false;
        }
    }
    if (!SaveToDataInventory<PcieInfoV6Data>(std::move(chipTransV6Data.resPcieV6Data), dataInventory,
                                             TABLE_NAME_PCIE_INFO_V6))
    {
        ERROR("Save data failed, %.", TABLE_NAME_PCIE_INFO_V6);
        return false;
    }
    return flag;
}

bool ChipTransV6Processor::ProcessOneDevice(const std::string& devicePath, ChipTransV6Data& chipTransV6Data)
{
    auto deviceId = GetDeviceIdByDevicePath(devicePath);
    if (!Context::GetInstance().GetProfTimeRecordInfo(chipTransV6Data.timeRecord, profPath_, deviceId))
    {
        ERROR("Failed to obtain the time in start_info and end_info. Path is %, device id is %.", profPath_, deviceId);
        return false;
    }
    DBInfo pcieInfo("chip_trans.db", "PcieInfoV6");
    std::string pcieDBPath = Utils::File::PathJoin({devicePath, SQLITE, pcieInfo.dbName});
    if (!pcieInfo.ConstructDBRunner(pcieDBPath))
    {
        ERROR("Create % connection failed.", pcieDBPath);
        return false;
    }
    bool flag = true;

    auto status = CheckPathAndTable(pcieDBPath, pcieInfo, false);
    if (status == CHECK_SUCCESS)
    {
        chipTransV6Data.oriPcieV6Data = LoadPcieV6Data(pcieInfo);
    }
    else if (status == CHECK_FAILED)
    {
        flag = false;
    }
    else if (status == NOT_EXIST)
    {
        return true;
    }

    std::vector<PcieInfoV6Data> pcieV6Data;
    if (!FormatData(pcieV6Data, chipTransV6Data, deviceId))
    {
        ERROR("Format data failed, %.", PROCESSOR_NAME_CHIP_TRAINS_V6);
        return false;
    }

    FilterDataByStartTime(pcieV6Data, chipTransV6Data.timeRecord.startTimeNs, TABLE_NAME_PCIE_INFO);
    chipTransV6Data.resPcieV6Data.insert(chipTransV6Data.resPcieV6Data.end(), pcieV6Data.begin(), pcieV6Data.end());
    return flag;
}

OriPcieV6Format ChipTransV6Processor::LoadPcieV6Data(const DBInfo& pcieInfo)
{
    OriPcieV6Format oriPcieV6Data;
    std::string sql{
        "SELECT die_id, pcie_write_bandwidth, pcie_read_bandwidth, sys_time "
        "FROM " +
        pcieInfo.tableName};
    if (!pcieInfo.dbRunner->QueryData(sql, oriPcieV6Data))
    {
        ERROR("Failed to obtain data from the % table.", pcieInfo.tableName);
    }
    return oriPcieV6Data;
}

bool ChipTransV6Processor::FormatData(std::vector<PcieInfoV6Data>& pcieV6FormatData, ChipTransV6Data& chipTransV6Data,
                                      const uint16_t& deviceId)
{
    PcieInfoV6Data pcieInfoV6Data;
    pcieInfoV6Data.deviceId = deviceId;
    if (!Reserve(pcieV6FormatData, chipTransV6Data.resPcieV6Data.size()))
    {
        ERROR("Reserve for chip trains V6 data failed.");
        return false;
    }
    for (auto& row : chipTransV6Data.oriPcieV6Data)
    {
        uint64_t sysTime;
        std::tie(pcieInfoV6Data.dieId, pcieInfoV6Data.pcieWriteBandwidth, pcieInfoV6Data.pcieReadBandwidth, sysTime) =
            row;
        HPFloat timestamp(sysTime);
        pcieInfoV6Data.timestamp = GetLocalTime(timestamp, chipTransV6Data.timeRecord).Uint64();
        pcieV6FormatData.emplace_back(pcieInfoV6Data);
    }
    return true;
}
}  // namespace Domain
}  // namespace Analysis
