/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#include "analysis/csrc/domain/services/persistence/device/ccu_persistence.h"

#include <algorithm>
#include <ios>
#include <tuple>
#include <utility>

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/ccu_parser.h"
#include "analysis/csrc/domain/services/persistence/ccu_db_writer.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis
{
namespace Domain
{
using namespace Infra;
using namespace Utils;

namespace
{
const std::string CCU_DB_NAME = "ccu.db";
const std::string MISSION_TABLE = "OriginMission";
const std::string CHANNEL_TABLE = "OriginChannel";

using MissionRows =
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint32_t, uint64_t, uint32_t, uint64_t>>;
using ChannelRows = std::vector<std::tuple<uint32_t, uint64_t, uint32_t, uint32_t, uint32_t>>;

bool FitsSqliteInteger(uint64_t value) { return value <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()); }

bool ValidateMissionData(const CcuMissionDataSet& data)
{
    return std::all_of(data.records.begin(), data.records.end(),
                       [](const CcuMissionRecord& record)
                       {
                           return FitsSqliteInteger(record.lpStartTime) && FitsSqliteInteger(record.lpEndTime) &&
                                  FitsSqliteInteger(record.setckeBitStartTime) && FitsSqliteInteger(record.relEndTime);
                       });
}

bool ValidateChannelData(const CcuChannelDataSet& data)
{
    return std::all_of(data.records.begin(), data.records.end(),
                       [](const CcuChannelRecord& record) { return FitsSqliteInteger(record.timestamp); });
}

bool MarkFilesComplete(const std::string& dataPath, const std::vector<std::string>& files)
{
    for (const auto& file : files)
    {
        FileWriter marker(File::PathJoin({dataPath, file + ".complete"}), std::ios::out | std::ios::trunc);
        if (!marker.IsOpen())
        {
            ERROR("Create CCU complete marker failed: %", file);
            return false;
        }
    }
    return true;
}

bool SaveMissionData(DBRunner& runner, const std::string& dataPath, const CcuMissionDataSet& data)
{
    if (data.completedFiles.empty() && data.records.empty())
    {
        return true;
    }
    if (!runner.CreateTable(MISSION_TABLE, CcuDB().GetTableCols(MISSION_TABLE)))
    {
        ERROR("Create CCU mission table failed");
        return false;
    }
    MissionRows rows;
    rows.reserve(data.records.size());
    for (const auto& record : data.records)
    {
        rows.emplace_back(record.streamId, record.taskId, record.lpInstrId, record.lpStartTime, record.lpEndTime,
                          record.setckeBitInstrId, record.setckeBitStartTime, record.relId, record.relEndTime);
    }
    if (!InsertCcuRowsOnce(runner, MISSION_TABLE, rows))
    {
        ERROR("Persist CCU mission data failed");
        return false;
    }
    return MarkFilesComplete(dataPath, data.completedFiles);
}

bool SaveChannelData(DBRunner& runner, const std::string& dataPath, const CcuChannelDataSet& data)
{
    if (data.completedFiles.empty() && data.records.empty())
    {
        return true;
    }
    if (!runner.CreateTable(CHANNEL_TABLE, CcuDB().GetTableCols(CHANNEL_TABLE)))
    {
        ERROR("Create CCU channel table failed");
        return false;
    }
    ChannelRows rows;
    rows.reserve(data.records.size());
    for (const auto& record : data.records)
    {
        rows.emplace_back(record.channelId, record.timestamp, record.maxBw, record.minBw, record.avgBw);
    }
    if (!InsertCcuRowsOnce(runner, CHANNEL_TABLE, rows))
    {
        ERROR("Persist CCU channel data failed");
        return false;
    }
    return MarkFilesComplete(dataPath, data.completedFiles);
}

}  // namespace

uint32_t CcuPersistence::ProcessEntry(DataInventory& dataInventory, const Infra::Context& context)
{
    const auto missionData = dataInventory.GetPtr<CcuMissionDataSet>();
    const auto channelData = dataInventory.GetPtr<CcuChannelDataSet>();
    if (missionData == nullptr || channelData == nullptr)
    {
        ERROR("CCU parser output is missing");
        return ANALYSIS_ERROR;
    }
    if (!ValidateMissionData(*missionData))
    {
        ERROR("CCU mission timestamp exceeds the SQLite integer range");
        return ANALYSIS_ERROR;
    }
    if (!ValidateChannelData(*channelData))
    {
        ERROR("CCU channel timestamp exceeds the SQLite integer range");
        return ANALYSIS_ERROR;
    }

    const auto& deviceContext = static_cast<const DeviceContext&>(context);
    const std::string sqlitePath = File::PathJoin({deviceContext.GetDeviceFilePath(), Common::SQLITE});
    if (!File::CreateDir(sqlitePath))
    {
        ERROR("Create CCU sqlite directory failed: %", sqlitePath);
        return ANALYSIS_ERROR;
    }
    const std::string dataPath = File::PathJoin({deviceContext.GetDeviceFilePath(), "data"});
    DBRunner runner(File::PathJoin({sqlitePath, CCU_DB_NAME}));
    if (missionData->hasCompletedFiles && !runner.CheckTableExists(MISSION_TABLE))
    {
        ERROR("Completed CCU mission source has no persisted table; clear markers and reimport");
        return ANALYSIS_ERROR;
    }
    if (channelData->hasCompletedFiles && !runner.CheckTableExists(CHANNEL_TABLE))
    {
        ERROR("Completed CCU channel source has no persisted table; clear markers and reimport");
        return ANALYSIS_ERROR;
    }

    return SaveMissionData(runner, dataPath, *missionData) && SaveChannelData(runner, dataPath, *channelData)
               ? ANALYSIS_OK
               : ANALYSIS_ERROR;
}

namespace CCU_PERSISTENCE_REGISTER
{
REGISTER_PROCESS_SEQUENCE(CcuPersistence, true, CcuMissionParser, CcuChannelParser);
REGISTER_PROCESS_DEPENDENT_DATA(CcuPersistence, CcuMissionDataSet, CcuChannelDataSet);
REGISTER_PROCESS_SUPPORT_CHIP(CcuPersistence, CHIP_V6_1_0);
}  // namespace CCU_PERSISTENCE_REGISTER

}  // namespace Domain
}  // namespace Analysis
