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

#include "analysis/csrc/domain/data_process/ai_task/msproftx_host_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

MsprofTxHostProcessor::MsprofTxHostProcessor(const std::string &profPath) : DataProcessor(profPath) {}

OriMsprofTxHostData LoadTxData(const DBInfo &msprofTxDB, const std::string &dbPath)
{
    OriMsprofTxHostData data;
    if (msprofTxDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return data;
    }
    std::string sql{
        "SELECT pid, tid, category, payload_type, message_type, payload_value, start_time, end_time, "
        "event_type, message FROM " +
        msprofTxDB.tableName};
    if (!msprofTxDB.dbRunner->QueryData(sql, data))
    {
        ERROR("Query msproftx data failed, db path is %.", dbPath);
        return data;
    }
    return data;
}

OriMsprofTxExHostData LoadTxExData(const DBInfo &msprofTxExDB, const std::string &dbPath)
{
    OriMsprofTxExHostData data;
    if (msprofTxExDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return data;
    }
    std::string sql{"SELECT pid, tid, mark_id, start_time, end_time, event_type, domain, message FROM " +
                    msprofTxExDB.tableName};
    if (!msprofTxExDB.dbRunner->QueryData(sql, data))
    {
        ERROR("Query msproftx data failed, db path is %.", dbPath);
        return data;
    }
    return data;
}

bool MsprofTxHostProcessor::Process(DataInventory &dataInventory)
{
    ProfTimeRecord record;
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_))
    {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", profPath_);
        return false;
    }
    SyscntConversionParams params;
    if (!Context::GetInstance().GetSyscntConversionParams(params, HOST_ID, profPath_))
    {
        ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
        return false;
    }
    OriMsprofTxHostData oriTxData;
    DBInfo msprofTxDb("msproftx.db", "MsprofTx");
    OriMsprofTxExHostData oriTxExData;
    DBInfo msprofTxExDb("msproftx.db", "MsprofTxEx");
    bool notExistFlagTx = false;
    bool notExistFlagTxEx = false;
    if (!ProcessData<OriMsprofTxExHostData>(oriTxExData, msprofTxExDb, notExistFlagTxEx, LoadTxExData) &
        !ProcessData<OriMsprofTxHostData>(oriTxData, msprofTxDb, notExistFlagTx, LoadTxData))
    {
        ERROR("Get original data failed");
        return false;
    }
    if (notExistFlagTx && notExistFlagTxEx)
    {
        WARN("msprofTx in % is not exists, return", profPath_);
        return true;
    }
    std::vector<MsprofTxHostData> formatData;
    if (!Utils::Reserve(formatData, oriTxData.size() + oriTxExData.size()))
    {
        ERROR("Reserve for % data failed.", TABLE_NAME_MSTX);
        return false;
    }
    FormatTxData(oriTxData, formatData, params, record);
    FormatTxExData(oriTxExData, formatData, params, record);
    FilterDataByStartTime(formatData, record.startTimeNs, PROCESSOR_NAME_MSTX);
    if (!SaveToDataInventory<MsprofTxHostData>(std::move(formatData), dataInventory, PROCESSOR_NAME_MSTX))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_MSTX);
        return false;
    }
    return true;
}

void MsprofTxHostProcessor::FormatTxData(const OriMsprofTxHostData &oriTxData,
                                         std::vector<MsprofTxHostData> &processedData, SyscntConversionParams &params,
                                         ProfTimeRecord &record)
{
    MsprofTxHostData tmpData;
    std::string eventType;
    for (const auto &row : oriTxData)
    {
        std::tie(tmpData.pid, tmpData.tid, tmpData.category, tmpData.payloadType, tmpData.messageType,
                 tmpData.payloadValue, tmpData.timestamp, tmpData.end, eventType, tmpData.message) = row;
        HPFloat start = Utils::GetTimeFromHostCnt(tmpData.timestamp, params);
        HPFloat end = Utils::GetTimeFromHostCnt(tmpData.end, params);
        tmpData.eventType = GetEnumTypeValue(eventType, NAME_STR(MSTX_EVENT_TYPE_TABLE), MSTX_EVENT_TYPE_TABLE);
        tmpData.timestamp = GetLocalTime(start, record).Uint64();
        tmpData.end = GetLocalTime(end, record).Uint64();
        processedData.push_back(tmpData);
    }
}

void MsprofTxHostProcessor::FormatTxExData(const OriMsprofTxExHostData &oriTxExData,
                                           std::vector<MsprofTxHostData> &processedData, SyscntConversionParams &params,
                                           ProfTimeRecord &record)
{
    MsprofTxHostData tmpData;
    std::string eventType;
    uint64_t markId;
    for (const auto &row : oriTxExData)
    {
        std::tie(tmpData.pid, tmpData.tid, markId, tmpData.timestamp, tmpData.end, eventType, tmpData.domain,
                 tmpData.message) = row;
        Utils::HPFloat start = Utils::GetTimeFromHostCnt(tmpData.timestamp, params);
        Utils::HPFloat end = Utils::GetTimeFromHostCnt(tmpData.end, params);
        tmpData.eventType = GetEnumTypeValue(eventType, NAME_STR(MSTX_EVENT_TYPE_TABLE), MSTX_EVENT_TYPE_TABLE);
        tmpData.connectionId = markId + START_CONNECTION_ID_MSTX;
        tmpData.timestamp = GetLocalTime(start, record).Uint64();
        tmpData.end = GetLocalTime(end, record).Uint64();
        processedData.push_back(tmpData);
    }
}
}  // namespace Domain
}  // namespace Analysis
