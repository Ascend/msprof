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

#include "analysis/csrc/domain/services/persistence/host/ccu_add_info_db_dumper.h"

#include <algorithm>
#include <limits>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/persistence/ccu_db_writer.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis
{
namespace Domain
{
using namespace Infra;
using namespace Host::Cann;

namespace
{
const std::string CCU_ADD_INFO_DB_NAME = "ccu_add_info.db";
const std::string TASK_TABLE = "CCUTaskInfo";
const std::string WAIT_SIGNAL_TABLE = "CCUWaitSignalInfo";
const std::string GROUP_TABLE = "CCUGroupInfo";

using TaskRows = std::vector<std::tuple<uint32_t, uint32_t, std::string, std::string, uint32_t, uint32_t, uint32_t,
                                        uint32_t, uint32_t, uint32_t, uint32_t>>;
using WaitSignalRows =
    std::vector<std::tuple<uint32_t, std::string, std::string, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>>;
using GroupRows = std::vector<
    std::tuple<uint32_t, std::string, std::string, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
               uint32_t, std::string, std::string, std::string, uint64_t, uint32_t, uint32_t>>;

bool ValidateGroupData(const std::vector<CcuGroupInfoRecord>& records)
{
    return std::all_of(records.begin(), records.end(), [](const CcuGroupInfoRecord& record)
                       { return record.dataSize <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()); });
}

bool SaveTaskData(const DBRunner& runner, const std::vector<CcuTaskInfoRecord>& records)
{
    if (!runner.CreateTable(TASK_TABLE, CcuInfoDB().GetTableCols(TASK_TABLE)))
    {
        return false;
    }
    if (records.empty())
    {
        WARN("CCU task info has no records; created empty table: %", TASK_TABLE);
    }
    TaskRows rows;
    rows.reserve(records.size());
    for (const auto& record : records)
    {
        rows.emplace_back(record.version, record.workFlowMode, record.itemId, record.groupName, record.rankId,
                          record.rankSize, record.streamId, record.taskId, record.dieId, record.missionId,
                          record.instrId);
    }
    return InsertCcuRowsOnce(runner, TASK_TABLE, rows);
}

bool SaveWaitSignalData(const DBRunner& runner, const std::vector<CcuWaitSignalInfoRecord>& records)
{
    if (!runner.CreateTable(WAIT_SIGNAL_TABLE, CcuInfoDB().GetTableCols(WAIT_SIGNAL_TABLE)))
    {
        return false;
    }
    if (records.empty())
    {
        WARN("CCU wait-signal info has no records; created empty table: %", WAIT_SIGNAL_TABLE);
    }
    WaitSignalRows rows;
    rows.reserve(records.size());
    for (const auto& record : records)
    {
        rows.emplace_back(record.version, record.itemId, record.groupName, record.rankId, record.rankSize,
                          record.workFlowMode, record.streamId, record.taskId, record.dieId, record.instrId,
                          record.missionId, record.ckeId, record.mask, record.channelId, record.remoteRankId);
    }
    return InsertCcuRowsOnce(runner, WAIT_SIGNAL_TABLE, rows);
}

bool SaveGroupData(const DBRunner& runner, const std::vector<CcuGroupInfoRecord>& records)
{
    if (!ValidateGroupData(records))
    {
        ERROR("CCU data_size exceeds the SQLite integer range");
        return false;
    }
    if (!runner.CreateTable(GROUP_TABLE, CcuInfoDB().GetTableCols(GROUP_TABLE)))
    {
        return false;
    }
    if (records.empty())
    {
        WARN("CCU group info has no records; created empty table: %", GROUP_TABLE);
    }
    GroupRows rows;
    rows.reserve(records.size());
    for (const auto& record : records)
    {
        rows.emplace_back(record.version, record.itemId, record.groupName, record.rankId, record.rankSize,
                          record.workFlowMode, record.streamId, record.taskId, record.dieId, record.instrId,
                          record.missionId, record.reduceOpType, record.inputDataType, record.outputDataType,
                          record.dataSize, record.channelId, record.remoteRankId);
    }
    return InsertCcuRowsOnce(runner, GROUP_TABLE, rows);
}
}  // namespace

bool CcuAddInfoDBDumper::DumpData(const CcuInfoData& data) const
{
    if (!data.HasInput())
    {
        return true;
    }
    if (!ValidateGroupData(data.groupRecords))
    {
        ERROR("CCU data_size exceeds the SQLite integer range");
        return false;
    }
    const std::string sqlitePath = Utils::File::PathJoin({hostPath_, Common::SQLITE});
    if (!Utils::File::CreateDir(sqlitePath))
    {
        ERROR("Create CCU add-info sqlite directory failed: %", sqlitePath);
        return false;
    }

    DBRunner runner(Utils::File::PathJoin({sqlitePath, CCU_ADD_INFO_DB_NAME}));
    if (data.taskSourceCompleted && !runner.CheckTableExists(TASK_TABLE))
    {
        ERROR("Completed CCU task source has no persisted table; clear markers and reimport");
        return false;
    }
    if (data.waitSignalSourceCompleted && !runner.CheckTableExists(WAIT_SIGNAL_TABLE))
    {
        ERROR("Completed CCU wait-signal source has no persisted table; clear markers and reimport");
        return false;
    }
    if (data.groupSourceCompleted && !runner.CheckTableExists(GROUP_TABLE))
    {
        ERROR("Completed CCU group source has no persisted table; clear markers and reimport");
        return false;
    }
    const bool hasTaskSource = data.taskSourceParsed || !data.taskRecords.empty();
    const bool hasWaitSignalSource = data.waitSignalSourceParsed || !data.waitSignalRecords.empty();
    const bool hasGroupSource = data.groupSourceParsed || !data.groupRecords.empty();
    if (hasTaskSource && !SaveTaskData(runner, data.taskRecords))
    {
        ERROR("Persist CCU task info failed");
        return false;
    }
    if (hasWaitSignalSource && !SaveWaitSignalData(runner, data.waitSignalRecords))
    {
        ERROR("Persist CCU wait-signal info failed");
        return false;
    }
    if (hasGroupSource && !SaveGroupData(runner, data.groupRecords))
    {
        ERROR("Persist CCU group info failed");
        return false;
    }
    for (const auto& file : data.completedFiles)
    {
        Utils::FileWriter marker(file + ".complete", std::ios::out | std::ios::trunc);
        if (!marker.IsOpen())
        {
            ERROR("Create CCU add-info complete marker failed: %", file);
            return false;
        }
    }
    return true;
}

}  // namespace Domain
}  // namespace Analysis
