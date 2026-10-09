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

#include "analysis/csrc/domain/data_process/ai_task/step_trace_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{

StepTraceProcessor::StepTraceProcessor(const std::string &profPath) : DataProcessor(profPath) {}

std::vector<TrainTraceData> GenerateTraceData(const DBInfo &dbInfo, LocaltimeContext &allParams, bool &errFlag,
                                              SyscntConversionParams &params)
{
    OriTrace oriData;
    std::vector<TrainTraceData> processedData;
    std::string sql{
        "SELECT model_id, iteration_id, FP_start, BP_end, iteration_end, iteration_time, fp_bp_time,"
        " grad_refresh_bound, data_aug_bound FROM " +
        dbInfo.tableName};
    if (!dbInfo.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", dbInfo.tableName);
        errFlag = true;
        return processedData;
    }
    if (!Utils::Reserve(processedData, oriData.size()))
    {
        ERROR("Reserve for Training trace data failed.");
        errFlag = true;
        return processedData;
    }
    TrainTraceData tmp;
    tmp.deviceId = allParams.deviceId;
    for (const auto &row : oriData)
    {
        std::tie(tmp.modelId, tmp.indexId, tmp.fpStart, tmp.bpEnd, tmp.iterEnd, tmp.iterTime, tmp.fpBpTime,
                 tmp.gradRefreshBound, tmp.dataAugBound) = row;
        if (tmp.fpStart != 0)
        {
            HPFloat fp = GetTimeFromSyscnt(tmp.fpStart, params);
            tmp.fpStart = GetLocalTime(fp, allParams.timeRecord).Uint64();
        }
        if (tmp.bpEnd != 0)
        {
            HPFloat bp = GetTimeFromSyscnt(tmp.bpEnd, params);
            tmp.bpEnd = GetLocalTime(bp, allParams.timeRecord).Uint64();
        }
        HPFloat iterEnd = GetTimeFromSyscnt(tmp.iterEnd, params);
        tmp.iterEnd = GetLocalTime(iterEnd, allParams.timeRecord).Uint64();
        tmp.iterTime = GetDurTimeFromSyscnt(tmp.iterTime, params).Uint64();
        tmp.fpBpTime = GetDurTimeFromSyscnt(tmp.fpBpTime, params).Uint64();
        tmp.gradRefreshBound = GetDurTimeFromSyscnt(tmp.gradRefreshBound, params).Uint64();
        tmp.dataAugBound = GetDurTimeFromSyscnt(tmp.dataAugBound, params).Uint64();
        tmp.timestamp = (tmp.iterEnd >= tmp.iterTime) ? (tmp.iterEnd - tmp.iterTime) : 0;
        processedData.push_back(tmp);
    }
    return processedData;
}

std::vector<GetNextData> GenerateNextData(const DBInfo &dbInfo, LocaltimeContext &allParams, bool &errFlag,
                                          SyscntConversionParams &params)
{
    OriGetNext oriData;
    std::vector<GetNextData> processedData;
    std::string sql{"SELECT model_id, index_id, start_time, end_time FROM " + dbInfo.tableName};
    if (!dbInfo.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", dbInfo.tableName);
        errFlag = true;
        return processedData;
    }
    if (!Utils::Reserve(processedData, oriData.size()))
    {
        ERROR("Reserve for GetNext data failed.");
        errFlag = true;
        return processedData;
    }
    GetNextData tmp;
    tmp.deviceId = allParams.deviceId;
    for (const auto &row : oriData)
    {
        std::tie(tmp.modelId, tmp.indexId, tmp.timestamp, tmp.end) = row;
        HPFloat start = GetTimeFromSyscnt(tmp.timestamp, params);
        tmp.timestamp = GetLocalTime(start, allParams.timeRecord).Uint64();
        HPFloat end = GetTimeFromSyscnt(tmp.end, params);
        tmp.end = GetLocalTime(end, allParams.timeRecord).Uint64();
        processedData.push_back(tmp);
    }
    return processedData;
}

std::vector<AllReduceData> GenerateAllReduceData(const DBInfo &dbInfo, LocaltimeContext &allParams, bool &errFlag,
                                                 SyscntConversionParams &params)
{
    OriAllReduce oriData;
    std::vector<AllReduceData> processedData;
    std::string sql{"SELECT model_id, index_id, iteration_end, start, end FROM " + dbInfo.tableName};
    if (!dbInfo.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", dbInfo.tableName);
        errFlag = true;
        return processedData;
    }
    if (!Utils::Reserve(processedData, oriData.size()))
    {
        ERROR("Reserve for AllReduce data failed.");
        errFlag = true;
        return processedData;
    }
    AllReduceData tmp;
    tmp.deviceId = allParams.deviceId;
    for (const auto &row : oriData)
    {
        std::tie(tmp.modelId, tmp.indexId, tmp.iterEnd, tmp.timestamp, tmp.end) = row;
        HPFloat iterEnd = GetTimeFromSyscnt(tmp.iterEnd, params);
        tmp.iterEnd = GetLocalTime(iterEnd, allParams.timeRecord).Uint64();
        HPFloat start = GetTimeFromSyscnt(tmp.timestamp, params);
        tmp.timestamp = GetLocalTime(start, allParams.timeRecord).Uint64();
        HPFloat end = GetTimeFromSyscnt(tmp.end, params);
        tmp.end = GetLocalTime(end, allParams.timeRecord).Uint64();
        processedData.push_back(tmp);
    }
    return processedData;
}

bool StepTraceProcessor::Process(DataInventory &dataInventory)
{
    LocaltimeContext allParams;
    bool flag = true;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<TrainTraceData> resTrace;
    std::vector<GetNextData> resNext;
    std::vector<AllReduceData> resAllReduce;
    for (const auto &devPath : deviceList)
    {
        allParams.deviceId = GetDeviceIdByDevicePath(devPath);
        if (allParams.deviceId == INVALID_DEVICE_ID)
        {
            ERROR("the invalid deviceId cannot to be identified.");
            flag = false;
            continue;
        }
        if (!Context::GetInstance().GetProfTimeRecordInfo(allParams.timeRecord, profPath_, allParams.deviceId))
        {
            ERROR("GetProfTimeRecordInfo failed, profPath is %, device id is %.", profPath_, allParams.deviceId);
            flag = false;
            continue;
        }
        OriTrace traceData;
        DBInfo traceDb("trace.db", "training_trace");
        flag = ProcessData<TrainTraceData>(traceDb, allParams, resTrace, GenerateTraceData, devPath) && flag;
        FilterDataByStartTime(resTrace, allParams.timeRecord.startTimeNs, PROCESSOR_NAME_STEP_TRACE);

        OriGetNext nextData;
        DBInfo nextDb("trace.db", "get_next");
        flag = ProcessData<GetNextData>(nextDb, allParams, resNext, GenerateNextData, devPath) && flag;
        FilterDataByStartTime(resNext, allParams.timeRecord.startTimeNs, PROCESSOR_NAME_STEP_TRACE);

        OriAllReduce reduceData;
        DBInfo reduceDb("trace.db", "all_reduce");
        flag = ProcessData<AllReduceData>(reduceDb, allParams, resAllReduce, GenerateAllReduceData, devPath) && flag;
        FilterDataByStartTime(resAllReduce, allParams.timeRecord.startTimeNs, PROCESSOR_NAME_STEP_TRACE);
    }
    if (!SaveStepTraceData(resTrace, resNext, resAllReduce, dataInventory))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_STEP_TRACE);
        flag = false;
    }
    return flag;
}

bool StepTraceProcessor::SaveStepTraceData(std::vector<TrainTraceData> &trace, std::vector<GetNextData> &next,
                                           std::vector<AllReduceData> &reduce, DataInventory &dataInventory)
{
    auto traceFlag = SaveToDataInventory<TrainTraceData>(std::move(trace), dataInventory, PROCESSOR_NAME_STEP_TRACE);
    auto nextFlag = SaveToDataInventory<GetNextData>(std::move(next), dataInventory, PROCESSOR_NAME_STEP_TRACE);
    auto reduceFlag = SaveToDataInventory<AllReduceData>(std::move(reduce), dataInventory, PROCESSOR_NAME_STEP_TRACE);
    return traceFlag && nextFlag && reduceFlag;
}
}  // namespace Domain
}  // namespace Analysis
