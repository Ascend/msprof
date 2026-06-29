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

#include "analysis/csrc/domain/services/persistence/device/metric_summary_persistence.h"

#include "analysis/csrc/domain/services/association/include/pmu_association.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Common;
namespace
{
const int CHIP4_DEFAULT_COLUMN_NUM = 4;
const int OTHER_CHIP_DEFAULT_COLUMN_NUM = 5;
const int CHIP4_PMU_TIME_NUM = 2;
const int OTHER_CHIP_PMU_TIME_NUM = 1;
const std::string AIC_PREFIX = "aic_";
const std::string AIV_PREFIX = "aiv_";
}  // namespace

MetricSummaryPersistence::~MetricSummaryPersistence()
{
    if (stmt_)
    {
        sqlite3_finalize(stmt_);
    }
    if (db_)
    {
        int rc = sqlite3_close(db_);
        if (rc != SQLITE_OK)
        {
            ERROR("First close failed, err: % !", sqlite3_errmsg(db_));
            sqlite3_close_v2(db_);
        }
        db_ = nullptr;
    }
}

bool MetricSummaryPersistence::BindAndExecuteInsert(std::unordered_map<PmuHeaderType, std::vector<uint64_t>>& ids,
                                                    std::unordered_map<PmuHeaderType, std::vector<double>>& pmu)
{
    // 此处和Python表头保持强规定顺序，按照id(后续不展示，可以不用关注顺序)，AIC:time cyc, pmu, AIV:time cyc
    // pmu的顺序绑定值
    int index = 0;
    for (auto& value : ids[TASK_ID])
    {
        sqlite3_bind_int64(stmt_, ++index, value);
    }
    for (auto& value : pmu[AIC_TOTAL_TIME])
    {
        sqlite3_bind_double(stmt_, ++index, value);
    }
    for (auto& value : ids[AIC_TOTAL_CYCLE])
    {
        sqlite3_bind_int64(stmt_, ++index, value);
    }
    for (auto& value : pmu[AIC_PMU_RESULT])
    {
        sqlite3_bind_double(stmt_, ++index, value);
    }
    for (auto& value : pmu[AIV_TOTAL_TIME])
    {
        sqlite3_bind_double(stmt_, ++index, value);
    }
    for (auto& value : ids[AIV_TOTAL_CYCLE])
    {
        sqlite3_bind_int64(stmt_, ++index, value);
    }
    for (auto& value : pmu[AIV_PMU_RESULT])
    {
        sqlite3_bind_double(stmt_, ++index, value);
    }
    auto rc = sqlite3_step(stmt_);
    if (rc != SQLITE_DONE)
    {
        // 插入失败，回滚事务
        if (sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr))
        {
            ERROR("sqlite3_exec return %: rollback failed!", rc);
        }
        std::string errorMsg = "Failed to insert data: " + std::string(sqlite3_errmsg(db_));
        ERROR("sqlite3_exec return %, %", rc, errorMsg);
        return false;
    }
    return true;
}

bool MetricSummaryPersistence::ConstructData(std::unordered_map<PmuHeaderType, std::vector<uint64_t>>& ids,
                                             std::unordered_map<PmuHeaderType, std::vector<double>>& result,
                                             DeviceTask& task, int aicLength, int aivLength)
{
    std::vector<double> aicPmuResult(aicLength, 0.0);
    std::vector<double> aivPmuResult(aivLength, 0.0);
    if (task.acceleratorType == MIX_AIV || task.acceleratorType == MIX_AIC)
    {
        auto pmu = dynamic_cast<PmuInfoMixAccelerator*>(task.pmuInfo.get());
        ids[AIC_TOTAL_CYCLE].push_back(pmu->aicTotalCycles);
        ids[AIV_TOTAL_CYCLE].push_back(pmu->aivTotalCycles);
        result[AIC_TOTAL_TIME].push_back(pmu->aiCoreTime);
        result[AIV_TOTAL_TIME].push_back(pmu->aivTime);
        if (pmu->aicPmuResult.empty())
        {
            result[AIC_PMU_RESULT] = aicPmuResult;
        }
        else
        {
            result[AIC_PMU_RESULT] = pmu->aicPmuResult;
        }
        if (pmu->aivPmuResult.empty())
        {
            result[AIV_PMU_RESULT] = aivPmuResult;
        }
        else
        {
            result[AIV_PMU_RESULT] = pmu->aivPmuResult;
        }
    }
    else if (task.acceleratorType == AIV)
    {
        auto pmu = dynamic_cast<PmuInfoSingleAccelerator*>(task.pmuInfo.get());
        ids[AIC_TOTAL_CYCLE].push_back(0);  // AIC置0
        ids[AIV_TOTAL_CYCLE].push_back(pmu->totalCycles);
        result[AIC_TOTAL_TIME].push_back(0.0);
        result[AIV_TOTAL_TIME].push_back(pmu->totalTime);
        result[AIC_PMU_RESULT] = aicPmuResult;
        result[AIV_PMU_RESULT] = pmu->pmuResult;
    }
    else if (task.acceleratorType == AIC)
    {
        auto pmu = dynamic_cast<PmuInfoSingleAccelerator*>(task.pmuInfo.get());
        ids[AIC_TOTAL_CYCLE].push_back(pmu->totalCycles);
        ids[AIV_TOTAL_CYCLE].push_back(0);  // AIV置0
        result[AIC_TOTAL_TIME].push_back(pmu->totalTime);
        result[AIV_TOTAL_TIME].push_back(0.0);
        result[AIC_PMU_RESULT] = pmu->pmuResult;
        result[AIV_PMU_RESULT] = aivPmuResult;
    }
    else
    {
        return false;
    }
    return true;
}

MetricSummaryDB::MetricSummaryDB(TableColumns columns)
{
    dbName_ = "metric_summary.db";
    tableColNames_["MetricSummary"] = columns;
}

TableColumns MetricSummaryPersistence::GetTableColumn(const DeviceContext& context)
{
    TableColumns res;
    if (context.GetChipID() == CHIP_V4_1_0)
    {
        auto aicHeader = aicCalculator_->GetPmuHeader();
        aicLength_ = static_cast<int>(aicHeader.size());
        auto aivHeader = aivCalculator_->GetPmuHeader();
        aivLength_ = static_cast<int>(aivHeader.size());
        res.emplace_back("task_id", SQL_INTEGER_TYPE);
        res.emplace_back("stream_id", SQL_INTEGER_TYPE);
        res.emplace_back("subtask_id", SQL_INTEGER_TYPE);
        res.emplace_back("batch_id", SQL_INTEGER_TYPE);
        res.emplace_back("aic_total_time", SQL_NUMERIC_TYPE);
        res.emplace_back("aic_total_cycles", SQL_NUMERIC_TYPE);
        for (auto& str : aicHeader)
        {
            res.emplace_back((AIC_PREFIX + str), SQL_NUMERIC_TYPE);
        }
        res.emplace_back("aiv_total_time", SQL_NUMERIC_TYPE);
        res.emplace_back("aiv_total_cycles", SQL_NUMERIC_TYPE);
        for (auto& str : aivHeader)
        {
            res.emplace_back((AIV_PREFIX + str), SQL_NUMERIC_TYPE);
        }
    }
    return res;
}

uint32_t MetricSummaryPersistence::GenerateAndSavePmuData(std::map<TaskId, std::vector<DeviceTask>>& deviceTask,
                                                          std::string& dbPath)
{
    bool isNone = true;
    std::vector<uint64_t> ids;
    ids.reserve(CHIP4_DEFAULT_COLUMN_NUM);
    std::unordered_map<PmuHeaderType, std::vector<uint64_t>> idMap;
    std::unordered_map<PmuHeaderType, std::vector<double>> resultMap;
    uint64_t count;
    for (auto& it : deviceTask)
    {
        count = 0;
        ids.clear();
        ids.push_back(it.first.taskId);
        ids.push_back(it.first.streamId);
        ids.push_back(it.first.contextId);
        ids.push_back(it.first.batchId);
        for (auto& task : it.second)
        {
            if (dynamicFlag && count == 0 && it.second.size() > 1)
            {  // 动态profiling第一个算子的PMU不落盘
                count++;
                continue;
            }
            count++;
            idMap[TASK_ID] = ids;
            sqlite3_reset(stmt_);
            if (!ConstructData(idMap, resultMap, task, aicLength_, aivLength_))
            {
                continue;
            }
            if (BindAndExecuteInsert(idMap, resultMap))
            {
                isNone = false;
                idMap.clear();
                resultMap.clear();
            }
            else
            {
                ERROR("Transaction is error, rollback");
                return ANALYSIS_ERROR;
            }
        }
    }
    sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
    if (isNone && !File::DeleteFile(dbPath))
    {
        // 说明没有PMU数据，没有PMU数据就不应该生成DB，直接删除DB文件
        ERROR("delete % failed", dbPath);
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

void MetricSummaryPersistence::CreateConnection(const std::string& dbPath)
{
    int rc = sqlite3_open(dbPath.c_str(), &db_);  // 连接数据库
    if (rc != SQLITE_OK)
    {
        std::string errorMsg = "Failed to open database: " + std::string(sqlite3_errmsg(db_));
        ERROR("sqlite3_exec return %, %", rc, errorMsg);
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
    else
    {
        sqlite3_exec(db_, "PRAGMA synchronous = OFF;", nullptr, nullptr, nullptr);
    }
}

uint32_t MetricSummaryPersistence::SaveDataToDb(std::map<TaskId, std::vector<DeviceTask>>& deviceTask,
                                                std::string& dbPath, DBInfo& dbInfo)
{
    CreateConnection(dbPath);
    if (db_ == nullptr)
    {
        ERROR("Get DB fail!");
        return ANALYSIS_ERROR;
    }
    int colNum = static_cast<int>(dbInfo.database->GetTableCols(dbInfo.tableName).size());
    // 开启事务
    sqlite3_exec(db_, "BEGIN", nullptr, nullptr, nullptr);
    std::string valueStr;
    for (int i = 0; i < colNum - 1; ++i)
    {
        valueStr += "?,";
    }
    valueStr = "(" + valueStr + "?)";
    std::string sql = "INSERT INTO " + dbInfo.tableName + " VALUES" + valueStr;
    sqlite3_busy_timeout(db_, TIMEOUT);
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr);
    if (rc != SQLITE_OK)
    {
        std::string errorMsg = std::string(sqlite3_errmsg(db_));
        ERROR("sqlite3_prepare_v2 run failed: %", errorMsg);
        return ANALYSIS_ERROR;
    }
    if (GenerateAndSavePmuData(deviceTask, dbPath) == ANALYSIS_OK)
    {
        return ANALYSIS_OK;
    }
    else
    {
        ERROR("Insert data to % failed", dbInfo.tableName);
        return ANALYSIS_ERROR;
    }
}

uint32_t MetricSummaryPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);
    auto deviceTask = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    if (!deviceTask)
    {
        ERROR("device task data is null.");
        return ANALYSIS_ERROR;
    }
    SampleInfo sampleInfo;
    deviceContext.Getter(sampleInfo);
    dynamicFlag = sampleInfo.dynamic;
    aicCalculator_ = MetricCalculatorFactory::GetAicCalculator(sampleInfo.aiCoreMetrics);
    aivCalculator_ = MetricCalculatorFactory::GetAivCalculator(sampleInfo.aivMetrics);
    if (aicCalculator_ == nullptr || aivCalculator_ == nullptr)
    {
        WARN("There is no PMU metric config don't need to persistence");
        return ANALYSIS_OK;
    }
    DBInfo metricSummary("metric_summary.db", "MetricSummary");
    auto columns = GetTableColumn(deviceContext);
    MAKE_SHARED_NO_OPERATION(metricSummary.database, MetricSummaryDB, columns);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, metricSummary.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(metricSummary.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    if (!metricSummary.dbRunner->CreateTable(metricSummary.tableName,
                                             metricSummary.database->GetTableCols(metricSummary.tableName)))
    {
        ERROR("Create table: % failed", metricSummary.tableName);
        return ANALYSIS_ERROR;
    }
    if (SaveDataToDb(*deviceTask, dbPath, metricSummary) == ANALYSIS_OK)
    {
        INFO("Process % done!", metricSummary.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed: %", dbPath);
    return ANALYSIS_ERROR;
}

REGISTER_PROCESS_SEQUENCE(MetricSummaryPersistence, false, PmuAssociation);
REGISTER_PROCESS_DEPENDENT_DATA(MetricSummaryPersistence, std::map<TaskId, std::vector<Domain::DeviceTask>>);
REGISTER_PROCESS_SUPPORT_CHIP(MetricSummaryPersistence, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
