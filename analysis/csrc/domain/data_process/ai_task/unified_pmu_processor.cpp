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
#include "analysis/csrc/domain/data_process/ai_task/unified_pmu_processor.h"

#include <unordered_set>

#include "analysis/csrc/application/credential/id_pool.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis
{
namespace Domain
{
using namespace Environment;
using namespace Application::Credential;

namespace
{
const std::string AI_VECTOR_CORE_DB = "ai_vector_core.db";
const std::string TYPE_AIV = "AIV";
const std::string AI_CORE_DB = "aicore.db";
const std::string TYPE_AIC = "AIC";
const std::set<std::string> SAMPLE_BASED_DB_NAMES = {AI_VECTOR_CORE_DB, AI_CORE_DB};
const std::set<std::string> INVALID_COLUMN_NAMES = {"task_id",    "stream_id", "subtask_id", "batch_id", "task_type",
                                                    "start_time", "end_time",  "ffts_type",  "core_type"};
const std::string TASK_BASED = "task-based";
const double DOUBLE_ZERO = 0.0;

struct TaskPmuData
{
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t subtaskId = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    double value = 0.0;
};

struct SampleTimelineData
{
    double timestamp = 0.0;
    uint32_t coreId = UINT32_MAX;
    std::string taskCyc;
};

struct SampleSummaryData
{
    std::string metric;
    double value = 0.0;
    uint32_t coreid = UINT32_MAX;
};
}  // namespace

UnifiedPmuProcessor::UnifiedPmuProcessor(const std::string &profPaths) : DataProcessor(profPaths) {}

bool UnifiedPmuProcessor::Process(DataInventory &dataInventory)
{
    std::string metricMode;
    if (!Context::GetInstance().GetMetricMode(metricMode, profPath_))
    {
        ERROR("GetMetricMode failed.");
        return false;
    }
    if (metricMode == TASK_BASED)
    {
        // 对于task-based pmu数据,目前只支持stars芯片(仅stars支持获取globalTaskId)
        uint16_t version = Context::GetInstance().GetPlatformVersion(DEFAULT_DEVICE_ID, profPath_);
        if (!Context::GetInstance().IsStarsChip(version))
        {
            WARN("This platformVersion does not support the processing of pmu data.");
            return true;
        }
        return TaskBasedProcess(dataInventory);
    }
    return SampleBasedProcess(dataInventory);
}

bool UnifiedPmuProcessor::TaskBasedProcess(DataInventory &dataInventory)
{
    INFO("TaskBasedProcess, dir is %", profPath_);
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::unordered_map<std::string, uint16_t> dbPathAndDeviceIds;
    DBInfo metricDB("metric_summary.db", "MetricSummary");
    bool flag = true;
    for (const auto &devicePath : deviceList)
    {
        auto deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        if (deviceId == INVALID_DEVICE_ID)
        {
            ERROR("the invalid deviceId cannot to be identified.");
            return false;
        }
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, metricDB.dbName});
        if (!metricDB.ConstructDBRunner(dbPath))
        {
            ERROR("construct DB Runner failed.");
            flag = false;
            continue;
        }
        auto status = CheckPathAndTable(dbPath, metricDB);
        if (status == CHECK_SUCCESS)
        {
            dbPathAndDeviceIds.insert({dbPath, deviceId});
        }
        else if (status == CHECK_FAILED)
        {
            return false;
        }
    }
    if (dbPathAndDeviceIds.empty())
    {
        INFO("There is no db to export, prof path is %.", profPath_);
        return true;
    }
    auto headers = GetAndCheckTableColumns(dbPathAndDeviceIds, metricDB);
    if (headers.empty())
    {
        ERROR("GetAndCheckTableColumns failed, please check tableColumns are consistent.");
        return false;
    }

    if (!ProcessTaskBasedData(dbPathAndDeviceIds, metricDB, headers, dataInventory))
    {
        ERROR("TaskBasedProcess: process data failed.");
        flag = false;
    }

    return flag;
}

std::vector<std::string> UnifiedPmuProcessor::GetAndCheckTableColumns(
    const std::unordered_map<std::string, uint16_t> &dbPathAndDeviceID, DBInfo &metricDB)
{
    INFO("GetAndCheckTableColumns.");
    TableColumns tableColumns;
    std::vector<std::string> headers;
    for (const auto &pair : dbPathAndDeviceID)
    {
        MAKE_SHARED_RETURN_VALUE(metricDB.dbRunner, DBRunner, headers, pair.first);
        if (tableColumns.empty())
        {
            tableColumns = metricDB.dbRunner->GetTableColumns(metricDB.tableName);
        }
        else if (tableColumns != metricDB.dbRunner->GetTableColumns(metricDB.tableName))
        {
            ERROR("%'s tableColumns are different from others.", pair.first);
            // 若发现在各db中表字段不同,则返回空表头
            headers.clear();
            return headers;
        }
    }
    for (const auto &tableColumn : tableColumns)
    {
        headers.emplace_back(tableColumn.name);
    }
    return headers;
}

bool UnifiedPmuProcessor::ProcessTaskBasedData(const std::unordered_map<std::string, uint16_t> &dbPathAndDeviceIds,
                                               Analysis::Infra::DBInfo &metricDB, std::vector<std::string> &headers,
                                               DataInventory &dataInventory)
{
    bool flag = true;
    std::vector<UnifiedTaskPmu> processedData;
    for (const auto &dbPathAndDeviceId : dbPathAndDeviceIds)
    {
        for (const auto &header : headers)
        {
            if (!ProcessTaskBasedDataByHeader(dbPathAndDeviceId, metricDB, header, processedData))
            {
                flag = false;
                ERROR("Process task_based data failed, failed device is %", dbPathAndDeviceId.second);
            }
        }
    }
    if (!SaveToDataInventory<UnifiedTaskPmu>(std::move(processedData), dataInventory, PROCESSOR_NAME_TASK_PMU_INFO))
    {
        ERROR("Inject unified task PMU data to data inventory failed");
        return false;
    }
    return flag;
}

bool UnifiedPmuProcessor::ProcessTaskBasedDataByHeader(const std::pair<std::string, uint16_t> &dbPathAndDeviceId,
                                                       Analysis::Infra::DBInfo &metricDB, const std::string &header,
                                                       std::vector<UnifiedTaskPmu> &processedData)
{
    if (INVALID_COLUMN_NAMES.find(header) != INVALID_COLUMN_NAMES.end())
    {
        INFO("This column[%] does not need to be processed.", header);
        return true;
    }
    OTFormat pmuData = GetTaskBasedData(dbPathAndDeviceId.first, header, metricDB);
    if (!FormatTaskBasedData(pmuData, processedData, header, dbPathAndDeviceId.second))
    {
        ERROR("FormatData failed, dbPath is %.", dbPathAndDeviceId.first);
        return false;
    }
    return true;
}

UnifiedPmuProcessor::OTFormat UnifiedPmuProcessor::GetTaskBasedData(const std::string &dbPath,
                                                                    const std::string &columnName, DBInfo &metricDB)
{
    INFO("GetTaskBasedData, dbPath is %, column name is %.", dbPath, columnName);
    OTFormat oriData;
    MAKE_SHARED_RETURN_VALUE(metricDB.dbRunner, DBRunner, oriData, dbPath);
    if (metricDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string subtaskIdSql;
    uint16_t version = Context::GetInstance().GetPlatformVersion(DEFAULT_DEVICE_ID, profPath_);
    if (Context::GetInstance().IsChipV1(version))
    {
        subtaskIdSql = std::to_string(UINT32_MAX) + " AS subtask_id, ";
    }
    else
    {
        subtaskIdSql = "subtask_id, ";
    }
    std::string sql = "SELECT stream_id, task_id, ";
    sql.append(subtaskIdSql).append(" batch_id, ").append(columnName).append(" FROM ").append(metricDB.tableName);
    if (!metricDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query task-based % data failed, db path is %.", columnName, dbPath);
        return oriData;
    }
    return oriData;
}

uint64_t UnifiedPmuProcessor::UpdateColumnName(std::string &columnName)
{
    std::string suffix = "_time";
    if (columnName.size() >= suffix.size() && columnName.substr(columnName.size() - suffix.size()) == suffix)
    {
        return Analysis::Common::MILLI_SECOND;  // us -> ns
    }
    suffix = "_extra";
    if (columnName.size() >= suffix.size() && columnName.substr(columnName.size() - suffix.size()) == suffix)
    {
        columnName = columnName.substr(0, columnName.size() - suffix.size());  // 只改名，删去 _extra
    }
    suffix = "(ms)";
    if (columnName.size() >= suffix.size() && columnName.substr(columnName.size() - suffix.size()) == suffix)
    {
        columnName = columnName.substr(0, columnName.size() - suffix.size());  // 删去 (ms)
        return MICRO_SECOND;                                                   // ms -> ns
    }
    suffix = "(GB/s)";
    if (columnName.size() >= suffix.size() && columnName.substr(columnName.size() - suffix.size()) == suffix)
    {
        columnName = columnName.substr(0, columnName.size() - suffix.size());  // 删去 (GB/s)
        return BYTE_SIZE * BYTE_SIZE * BYTE_SIZE;                              // GB/s -> Byte/s
    }
    return 1;
}

bool UnifiedPmuProcessor::FormatTaskBasedData(const OTFormat &oriData, std::vector<UnifiedTaskPmu> &processedData,
                                              std::string columnName, const uint16_t &deviceId)
{
    INFO("FormatTaskBasedData.");
    if (oriData.empty())
    {
        ERROR("Task-based original data is empty, columnName is %.", columnName);
        return false;
    }
    if (!Utils::Reserve(processedData, oriData.size()))
    {
        ERROR("Reserve for % task-based data failed, columnName is %.", columnName);
        return false;
    }
    // 当前task-based pmu存在一个问题:
    // 1、没有考虑memory_bound或是cube utilization的计算
    auto valueScale = UpdateColumnName(columnName);
    TaskPmuData tempData;
    for (const auto &row : oriData)
    {
        std::tie(tempData.streamId, tempData.taskId, tempData.subtaskId, tempData.batchId, tempData.value) = row;
        processedData.emplace_back(deviceId, tempData.streamId, tempData.taskId, tempData.subtaskId, tempData.batchId,
                                   columnName, tempData.value * valueScale);
    }
    if (processedData.empty())
    {
        ERROR("FormatTaskBasedData data processing error.");
        return false;
    }
    return true;
}

bool UnifiedPmuProcessor::SampleBasedProcess(DataInventory &dataInventory)
{
    INFO("SampleBasedProcess, dir is %", profPath_);
    bool flag = true;
    uint64_t stringAiCoreId = IdPool::GetInstance().GetUint64Id(TYPE_AIC);
    uint64_t stringAiVectorCoreId = IdPool::GetInstance().GetUint64Id(TYPE_AIV);
    auto deviceList =
        Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);  // string db_path  pair<deviceId, CoreType>
    std::unordered_map<std::string, std::tuple<uint16_t, uint64_t>> dbPathTable;

    for (const auto &name : SAMPLE_BASED_DB_NAMES)
    {
        uint64_t coreType = (name == AI_CORE_DB) ? stringAiCoreId : stringAiVectorCoreId;
        DBInfo aicDB(name, "AICoreOriginalData");
        for (const auto &devicePath : deviceList)
        {
            auto deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
            if (deviceId == INVALID_DEVICE_ID)
            {
                ERROR("the invalid deviceId cannot to be identified.");
                return false;
            }
            std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, aicDB.dbName});
            if (!aicDB.ConstructDBRunner(dbPath))
            {
                ERROR("construct DB Runner failed.");
                flag = false;
                continue;
            }
            auto status = CheckPathAndTable(dbPath, aicDB);
            if (status == CHECK_SUCCESS)
            {
                dbPathTable.insert({dbPath, std::make_tuple(deviceId, coreType)});
            }
            else if (status == CHECK_FAILED)
            {
                flag = false;
                ERROR("Check % failed!", dbPath);
                continue;
            }
        }
    }
    if (dbPathTable.empty())
    {
        INFO("There is no db to export, prof path is %.", profPath_);
        return true;
    }

    if (!SampleBasedTimelineProcess(dbPathTable, dataInventory))
    {
        ERROR("Failed to process sample-based timeline data, fileDir is %.", profPath_);
        flag = false;
    }
    if (!SampleBasedSummaryProcess(dbPathTable, dataInventory))
    {
        ERROR("Failed to process sample-based summary data, fileDir is %.", profPath_);
        flag = false;
    }
    return flag;
}

bool UnifiedPmuProcessor::SampleBasedTimelineProcess(
    const std::unordered_map<std::string, std::tuple<uint16_t, uint64_t>> &dbPathTable, DataInventory &dataInventory)
{
    INFO("SampleBasedTimeline");
    bool flag = true;
    std::vector<UnifiedSampleTimelinePmu> processedData;
    for (const auto &pair : dbPathTable)
    {
        // deviceId,  profId, hostMonotonic, deviceMonotonic, timeRecord
        Utils::LocaltimeContext localtimeContext;
        // 0 deviceId, 1 coreType
        localtimeContext.deviceId = std::get<0>(pair.second);
        double freq;

        if (!Context::GetInstance().GetPmuFreq(freq, localtimeContext.deviceId, profPath_) ||
            Utils::IsDoubleEqual(freq, DOUBLE_ZERO))
        {
            ERROR("GetPmuFreq failed or aic freq is invalid, profPath is %.", profPath_);
            return false;
        }
        if (!Context::GetInstance().GetProfTimeRecordInfo(localtimeContext.timeRecord, profPath_,
                                                          localtimeContext.deviceId))
        {
            ERROR("GetProfTimeRecordInfo failed, profPath is %, device id is %.", profPath_, localtimeContext.deviceId);
            return false;
        }
        auto oriData = GetSampleBasedTimelineData(pair.first);
        if (!FormatSampleBasedTimelineData(oriData, processedData, localtimeContext, freq, std::get<1>(pair.second)))
        {
            ERROR("FormatSampleBasedTimelineData failed, dbPath is %.", pair.first);
            flag = false;
            continue;
        }
        FilterDataByStartTime(processedData, localtimeContext.timeRecord.startTimeNs,
                              PROCESSOR_NAME_SAMPLE_PMU_TIMELINE);
    }

    if (!SaveToDataInventory<UnifiedSampleTimelinePmu>(std::move(processedData), dataInventory,
                                                       PROCESSOR_NAME_SAMPLE_PMU_TIMELINE))
    {
        ERROR("SampleBasedTimeline: saveData failed.");
        flag = false;
    }
    return flag;
}

UnifiedPmuProcessor::OSTFormat UnifiedPmuProcessor::GetSampleBasedTimelineData(const std::string &dbPath)
{
    INFO("GetSampleBasedTimelineData, dbPath is %.", dbPath);
    OSTFormat oriData;
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, oriData, dbPath);
    if (dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql = "SELECT timestamp, task_cyc, coreid FROM AICoreOriginalData;";
    if (!dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query sample-based timeline data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

bool UnifiedPmuProcessor::FormatSampleBasedTimelineData(const OSTFormat &oriData,
                                                        std::vector<UnifiedSampleTimelinePmu> &processedData,
                                                        const Utils::LocaltimeContext &threadData, const double freq,
                                                        const uint64_t coreType)
{
    INFO("FormatSampleBasedTimelineData.");
    if (oriData.empty())
    {
        ERROR("Sample-based timeline original data is empty.");
        return false;
    }
    if (!Utils::Reserve(processedData, processedData.size() + oriData.size()))
    {
        ERROR("Reserve for % sample-based timeline data failed.");
        return false;
    }
    std::unordered_map<uint32_t, double> coreIdTimeMap;
    SampleTimelineData tempData;
    for (const auto &row : oriData)
    {
        std::tie(tempData.timestamp, tempData.taskCyc, tempData.coreId) = row;
        uint64_t cycle;
        if (Utils::StrToU64(cycle, tempData.taskCyc) != ANALYSIS_OK)
        {
            ERROR("task_cyc to uint64_t failed.");
            return false;
        }
        auto corePreRecord = coreIdTimeMap.find(tempData.coreId);
        double interval = (corePreRecord == coreIdTimeMap.end()) ? 1.0 : (tempData.timestamp - corePreRecord->second);
        coreIdTimeMap[tempData.coreId] = tempData.timestamp;
        if (Utils::IsDoubleEqual(interval, DOUBLE_ZERO))
        {
            WARN(
                "The interval is invalid, the record timestamp is %, "
                "core id is %.",
                tempData.timestamp, tempData.coreId);
            continue;
        }
        // task_cyc 单位为 次; freq单位为 MHz,即次/秒; interval单位为 ns
        // 由公式 (task_cyc / (freq * 10^6) / (interval / 10^9)) * 100% 化简而来
        double usage = (cycle * MILLI_SECOND) / (freq * interval) * PERCENTAGE;

        Utils::HPFloat timestamp{tempData.timestamp};
        processedData.emplace_back(threadData.deviceId, Utils::GetLocalTime(timestamp, threadData.timeRecord).Uint64(),
                                   cycle, usage, freq, static_cast<uint16_t>(tempData.coreId), coreType);
    }
    if (processedData.empty())
    {
        ERROR("FormatSampleBasedSummaryData data processing error.");
        return false;
    }
    return true;
}

bool UnifiedPmuProcessor::SampleBasedSummaryProcess(
    const std::unordered_map<std::string, std::tuple<uint16_t, uint64_t>> &dbPathTable, DataInventory &dataInventory)
{
    INFO("SampleBasedSummary");
    bool flag = true;
    std::vector<UnifiedSampleSummaryPmu> processedData;

    for (const auto &pair : dbPathTable)
    {
        auto oriData = GetSampleBasedSummaryData(pair.first);
        // 0 deviceId, 1 coreType
        if (!FormatSampleBasedSummaryData(oriData, processedData, std::get<0>(pair.second), std::get<1>(pair.second)))
        {
            ERROR("FormatSampleBasedSummaryData failed, dbPath is %.", pair.first);
            flag = false;
            continue;
        }
    }

    if (!SaveToDataInventory<UnifiedSampleSummaryPmu>(std::move(processedData), dataInventory,
                                                      PROCESSOR_NAME_SAMPLE_PMU_SUMMARY))
    {
        ERROR("SampleBasedSummary: saveData failed.");
        flag = false;
    }
    return flag;
}

UnifiedPmuProcessor::OSSFormat UnifiedPmuProcessor::GetSampleBasedSummaryData(const std::string &dbPath)
{
    INFO("GetSampleBasedSummaryData, dbPath is %.", dbPath);
    OSSFormat oriData;
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, oriData, dbPath);
    if (dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql = "SELECT metric, value, coreid FROM MetricSummary WHERE value IS NOT NULL;";
    if (!dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query sample-based summary data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

bool UnifiedPmuProcessor::FormatSampleBasedSummaryData(const OSSFormat &oriData,
                                                       std::vector<UnifiedSampleSummaryPmu> &processedData,
                                                       const uint16_t deviceId, const uint64_t coreType)
{
    INFO("FormatSampleBasedSummaryData.");
    if (oriData.empty())
    {
        ERROR("Sample-based summary original data is empty.");
        return false;
    }
    if (!Utils::Reserve(processedData, processedData.size() + oriData.size()))
    {
        ERROR("Reserve for % sample-based summary data failed.");
        return false;
    }
    SampleSummaryData tempData;
    for (const auto &data : oriData)
    {
        std::tie(tempData.metric, tempData.value, tempData.coreid) = data;
        auto valueScale = UpdateColumnName(tempData.metric);
        processedData.emplace_back(deviceId, tempData.metric, tempData.value * valueScale,
                                   static_cast<uint16_t>(tempData.coreid), coreType);
    }
    if (processedData.empty())
    {
        ERROR("FormatSampleBasedSummaryData data processing error.");
        return false;
    }
    return true;
}

}  // namespace Domain
}  // namespace Analysis
