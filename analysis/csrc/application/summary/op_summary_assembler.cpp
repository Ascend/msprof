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

#include "analysis/csrc/application/summary/op_summary_assembler.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Utils;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Application;
namespace
{
// headers
const std::string DEVICE_ID = "Device_id";
const std::string MODEL_ID = "Model ID";
const std::string TASK_ID = "Task ID";
const std::string STREAM_ID = "Stream ID";
const std::string OP_NAME = "Op Name";
const std::string OP_TYPE = "OP Type";
const std::string OP_STATE = "OP State";
const std::string TASK_TYPE = "Task Type";
const std::string TASK_START_TIME = "Task Start Time(us)";
const std::string TASK_DURATION = "Task Duration(us)";
const std::string TASK_WAIT_TIME = "Task Wait Time(us)";
const std::string BLOCK_NUM = "Block Num";
const std::string MIX_BLOCK_NUM = "Mix Block Num";
const std::string HF32_ELIGIBLE = "HF32 Eligible";
const std::string INPUT_SHAPES = "Input Shapes";
const std::string INPUT_DATA_TYPES = "Input Data Types";
const std::string INPUT_FORMATS = "Input Formats";
const std::string OUTPUT_SHAPES = "Output Shapes";
const std::string OUTPUT_DATA_TYPES = "Output Data Types";
const std::string OUTPUT_FORMATS = "Output Formats";
const std::string CONTEXT_ID = "Context ID";
const std::string CUBE_UTILIZATION = "cube_utilization(%)";
const std::string AIV_TIME = "aiv_time(us)";
const std::string AIV_TOTAL_TIME = "aiv_total_time(us)";

// headers index
const std::string DUR_IDX = "durIdx";
const std::string USAGE_IDX = "usageIdx";

// WRITE_BACK与INVALID类型不需要处理，针对helper场景, 去除运行在AI_CPU的HCCL小算子不生成op_summary,
// 运行在AI_CORE上的HCCL小算子也不呈现在op_summary,因此这四个类型都不生成数据即可，直接排除掉
const std::vector<std::string> INVALID_TASK_TYPE{"WRITE_BACK", "INVALID", "HCCL_AI_CPU", "COMMUNICATION"};
const std::vector<std::string> BASE_HEADER{DEVICE_ID, MODEL_ID,  TASK_ID,         STREAM_ID,     OP_NAME,       OP_TYPE,
                                           OP_STATE,  TASK_TYPE, TASK_START_TIME, TASK_DURATION, TASK_WAIT_TIME};
const std::vector<std::string> ADDITIONAL_TENSOR_HEADER{
    BLOCK_NUM,     MIX_BLOCK_NUM, HF32_ELIGIBLE,     INPUT_SHAPES,   INPUT_DATA_TYPES,
    INPUT_FORMATS, OUTPUT_SHAPES, OUTPUT_DATA_TYPES, OUTPUT_FORMATS, CONTEXT_ID};

int GetIndexForVec(const std::vector<std::string> &vec, const std::string &key)
{
    auto it = std::find(vec.begin(), vec.end(), key);
    if (it != vec.end())
    {
        return std::distance(vec.begin(), it);
    }
    return Analysis::Common::INVALID_VALUE;
}

int GetColumnIndex(const std::vector<std::string> &headers, std::vector<std::string> &&possibleNames)
{
    for (auto &name : possibleNames)
    {
        auto idx = GetIndexForVec(headers, name);
        if (idx != Analysis::Common::INVALID_VALUE)
        {
            return idx;
        }
    }
    return Analysis::Common::INVALID_VALUE;
}
}  // namespace

OpSummaryAssembler::OpSummaryAssembler(const std::string &name, const std::string &profPath)
    : SummaryAssembler(name, profPath)
{
}

void OpSummaryAssembler::GenerateHcclBody(std::vector<CommunicationOpData> &opData)
{
    auto len = headers_.size();
    for (const auto &data : opData)
    {
        std::vector<std::string> row = {std::to_string(data.deviceId),
                                        std::to_string(data.modelId),
                                        NA,
                                        NA,
                                        data.opName,
                                        data.opType,
                                        NA,
                                        "COMMUNICATION",
                                        DivideByPowersOfTenWithPrecision(data.timestamp),
                                        DivideByPowersOfTenWithPrecision(data.end - data.timestamp),
                                        "0",
                                        "0"};
        // 业务可以保证headers的长度会超过hcclOp数据的长度
        row.insert(row.end(), len - row.size(), NA);
        res_.emplace_back(row);
    }
}

void OpSummaryAssembler::SplitDataByTaskId(std::vector<TaskInfoData> &taskInfo)
{
    for (auto &data : taskInfo)
    {
        TaskId id{static_cast<uint16_t>(data.streamId), static_cast<uint16_t>(data.batchId), data.taskId,
                  data.contextId, data.deviceId};
        computeTask_[id] = &data;
    }
}

std::vector<std::string> OpSummaryAssembler::GenerateOneTaskRow(const TaskInfoData &computeTask,
                                                                const AscendTaskData &task)
{
    std::string ctxId = std::to_string(computeTask.contextId);
    if (computeTask.contextId == UINT32_MAX)
    {
        ctxId = NA;
    }
    return {std::to_string(task.deviceId),
            std::to_string(computeTask.modelId),
            std::to_string(task.taskId),
            std::to_string(task.streamId),
            computeTask.opName,
            computeTask.opType,
            computeTask.opState,
            computeTask.taskType,
            DivideByPowersOfTenWithPrecision(task.timestamp),
            DivideByPowersOfTenWithPrecision(task.end - task.timestamp),
            "0",
            std::to_string(computeTask.blockNum),
            std::to_string(computeTask.mixBlockNum),
            computeTask.opFlag,
            computeTask.inputShapes,
            computeTask.inputDataTypes,
            computeTask.inputFormats,
            computeTask.outputShapes,
            computeTask.outputDataTypes,
            computeTask.outputFormats,
            ctxId};
}

void OpSummaryAssembler::MergeTaskAndPmu(std::shared_ptr<MetricSummary> &pmu, std::vector<std::string> &row,
                                         const TaskId &id, std::unordered_map<std::string, int> &indexTable)
{
    if (pmu == nullptr)
    {
        return;
    }
    // pmu的结构为map<TaskId, vector<vector<string>>>
    auto it = (*pmu).data.find(id);
    if (it != (*pmu).data.end() && !it->second.empty())
    {
        // 队列匹配，取出第一个元素
        auto data = it->second.front();
        for (const auto &pair : data)
        {
            row.emplace_back(pair);
        }
        AddCubeUsage(row, indexTable);
        // 删除第一个元素
        it->second.erase(it->second.begin());
        // 判断vector中是否还有元素，如果没有就直接删除整个键值对<TaskId, vector<vector<string>>>
        if (it->second.empty())
        {
            pmu->data.erase(it);
        }
    }
    else
    {
        row.insert(row.end(), headers_.size() - row.size(), NA);
    }
}

void OpSummaryAssembler::GenerateOpBody(std::vector<AscendTaskData> &taskData, std::shared_ptr<MetricSummary> &pmu)
{
    if (pmu != nullptr)
    {
        headers_.insert(headers_.end(), pmu->labels.begin(), pmu->labels.end());
    }
    std::unordered_map<std::string, int> indexTable{{DUR_IDX, GetIndexForVec(headers_, TASK_DURATION)},
                                                    {USAGE_IDX, GetIndexForVec(headers_, CUBE_UTILIZATION)}};
    for (const auto &task : taskData)
    {
        TaskId id{static_cast<uint16_t>(task.streamId), static_cast<uint16_t>(task.batchId), task.taskId,
                  task.contextId, task.deviceId};
        auto it = computeTask_.find(id);
        if (it != computeTask_.end())
        {
            const std::string &taskType = it->second->taskType;
            const std::string &opName = it->second->opName;
            bool isInvalidType =
                std::find(INVALID_TASK_TYPE.begin(), INVALID_TASK_TYPE.end(), taskType) != INVALID_TASK_TYPE.end();
            bool isCommWithValidSuffix = (taskType == "COMMUNICATION" && EndsWith(opName, AIV_KERNEL));
            if (!isInvalidType || isCommWithValidSuffix)
            {
                auto row = GenerateOneTaskRow(*it->second, task);
                MergeTaskAndPmu(pmu, row, id, indexTable);
                res_.emplace_back(row);
            }
        }
    }
}

std::set<int> OpSummaryAssembler::GetMaskCols()
{
    std::set<int> maskCols;
    const auto platVersion = Context::GetInstance().GetPlatformVersion(DEFAULT_DEVICE_ID, profPath_);
    std::vector<std::string> STARS_HEADER{CONTEXT_ID, MIX_BLOCK_NUM, AIV_TIME, AIV_TOTAL_TIME};
    if (!Context::GetInstance().IsStarsChip(platVersion))
    {
        for (auto &name : STARS_HEADER)
        {
            auto idx = GetIndexForVec(headers_, name);
            if (idx != Analysis::Common::INVALID_VALUE)
            {
                maskCols.insert(idx);
            }
        }
    }
    return maskCols;
}

void OpSummaryAssembler::AddCubeUsage(std::vector<std::string> &data, std::unordered_map<std::string, int> &indexTable)
{
    const auto usageIdx = indexTable.at(USAGE_IDX);
    const auto durIdx = indexTable.at(DUR_IDX);
    if (usageIdx == Analysis::Common::INVALID_VALUE || durIdx == Analysis::Common::INVALID_VALUE ||
        data[usageIdx] == NA)
    {
        return;
    }

    auto dur = 0.0;
    uint64_t cycle;
    double usage = 0.0;
    if (StrToDouble(dur, data[durIdx]) == ANALYSIS_OK && !Utils::IsDoubleEqual(dur, 0.0) &&
        StrToDouble(usage, data[usageIdx]) == ANALYSIS_OK)
    {
        usage = usage / dur;
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(ACCURACY_THREE) << usage;
        data[usageIdx] = ss.str();
    }
    else
    {
        data[usageIdx] = "0";
    }
}

void OpSummaryAssembler::CalculateWaitTime()
{
    // 业务可以保证headers_有值的情况下，device_id与start_time的下标是有效的
    const auto deviceIndex = GetIndexForVec(headers_, DEVICE_ID);
    const auto timeIndex = GetIndexForVec(headers_, TASK_START_TIME);
    using rowType = std::vector<std::string>;
    std::sort(res_.begin(), res_.end(),
              [deviceIndex, timeIndex](const rowType &lv, const rowType &rv)
              {
                  if (lv.at(deviceIndex) == rv.at(deviceIndex))
                  {
                      return lv.at(timeIndex) < rv.at(timeIndex);
                  }
                  return lv.at(deviceIndex) < rv.at(deviceIndex);
              });
    const auto durIndex = GetIndexForVec(headers_, TASK_DURATION);
    const auto waitIndex = GetIndexForVec(headers_, TASK_WAIT_TIME);
    double currStart = 0.0;
    double preStart = 0.0;
    double preDur = 0.0;
    for (std::size_t i = 1; i < res_.size(); ++i)
    {
        if (StrToDouble(currStart, res_.at(i).at(timeIndex)) == ANALYSIS_OK &&
            StrToDouble(preStart, res_.at(i - 1).at(timeIndex)) == ANALYSIS_OK &&
            StrToDouble(preDur, res_.at(i - 1).at(durIndex)) == ANALYSIS_OK)
        {
            const auto waitTime = std::max(0.0, currStart - preStart - preDur);
            res_.at(i).at(waitIndex) = std::to_string(waitTime);
        }
    }
}

void OpSummaryAssembler::WriteToFile(const std::string &fileName, const std::set<int> &maskCols)
{
    auto timeIndex = GetIndexForVec(headers_, TASK_START_TIME);
    for (auto &row : res_)
    {
        if (timeIndex < static_cast<int>(row.size()))
        {
            row[timeIndex].append("\t");
        }
    }

    CsvWriter csvWriter = CsvWriter();
    csvWriter.WriteCsv(fileName, headers_, res_, maskCols);
}

uint8_t OpSummaryAssembler::AssembleData(DataInventory &dataInventory)
{
    auto taskInfoData = dataInventory.GetPtr<std::vector<TaskInfoData>>();
    auto ascendTaskData = dataInventory.GetPtr<std::vector<AscendTaskData>>();
    auto hcclOpData = dataInventory.GetPtr<std::vector<CommunicationOpData>>();
    auto metricData = dataInventory.GetPtr<MetricSummary>();
    if ((taskInfoData == nullptr || ascendTaskData == nullptr) && hcclOpData == nullptr)
    {
        WARN("No data to export op summary");
        return DATA_NOT_EXIST;
    }
    headers_ = BASE_HEADER;
    // 当没有ascendTask或者没有taskInfo数据时，只生成hccl数据
    if (taskInfoData != nullptr && ascendTaskData != nullptr)
    {
        headers_.insert(headers_.end(), ADDITIONAL_TENSOR_HEADER.begin(), ADDITIONAL_TENSOR_HEADER.end());
        SplitDataByTaskId(*taskInfoData);
        GenerateOpBody(*ascendTaskData, metricData);
    }
    else
    {
        headers_.emplace_back(CONTEXT_ID);
    }
    if (hcclOpData != nullptr)
    {
        GenerateHcclBody(*hcclOpData);
    }
    CalculateWaitTime();
    if (res_.empty())
    {
        ERROR("Can't match any task data, failed to generate op_summary.csv");
        return ASSEMBLE_FAILED;
    }
    WriteToFile(File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, OP_SUMMARY_NAME}), GetMaskCols());
    return ASSEMBLE_SUCCESS;
}

}  // namespace Application
}  // namespace Analysis
