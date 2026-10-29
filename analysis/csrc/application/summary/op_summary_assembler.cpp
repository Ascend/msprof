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

#include "analysis/csrc/application/summary/op_summary_assembler.h"

#include <algorithm>
#include <cstddef>
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

const std::vector<std::string> BASE_HEADER{DEVICE_ID, MODEL_ID,  TASK_ID,         STREAM_ID,     OP_NAME,       OP_TYPE,
                                           OP_STATE,  TASK_TYPE, TASK_START_TIME, TASK_DURATION, TASK_WAIT_TIME};
const std::vector<std::string> ADDITIONAL_TENSOR_HEADER{
    BLOCK_NUM,     MIX_BLOCK_NUM, HF32_ELIGIBLE,     INPUT_SHAPES,   INPUT_DATA_TYPES,
    INPUT_FORMATS, OUTPUT_SHAPES, OUTPUT_DATA_TYPES, OUTPUT_FORMATS, CONTEXT_ID};

// Cube utilization is the only pmu value derived from another column, so the formula lives here and
// both the assembler and the emitter call it.
void ComputeCubeUsage(std::string &usageCell, const std::string &durationCell)
{
    auto duration = 0.0;
    double usage = 0.0;
    if (StrToDouble(duration, durationCell) == ANALYSIS_OK && !Utils::IsDoubleEqual(duration, 0.0) &&
        StrToDouble(usage, usageCell) == ANALYSIS_OK)
    {
        usage = usage / duration;
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(ACCURACY_THREE) << usage;
        usageCell = ss.str();
        return;
    }
    usageCell = "0";
}

int GetIndexForVec(const std::vector<std::string> &vec, const std::string &key)
{
    auto it = std::find(vec.begin(), vec.end(), key);
    if (it != vec.end())
    {
        return std::distance(vec.begin(), it);
    }
    return Analysis::Common::INVALID_VALUE;
}
}  // namespace

OpSummaryAssembler::OpSummaryAssembler(const std::string &name, const std::string &profPath)
    : SummaryAssembler(name, profPath)
{
}

void OpSummaryAssembler::GenerateHcclBody(const std::vector<CommunicationOpData> &opData,
                                          std::vector<SortedOpRow> &rows)
{
    rows.reserve(rows.size() + opData.size());
    for (size_t i = 0; i < opData.size(); ++i)
    {
        const auto &data = opData[i];
        SortedOpRow row;
        row.deviceId = data.deviceId;
        row.timestamp = data.timestamp;
        row.duration = data.end - data.timestamp;
        row.index = i;
        row.hccl = true;
        row.pad = true;
        rows.emplace_back(std::move(row));
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

void OpSummaryAssembler::GenerateOpBody(const AssociatedTaskCollection &associatedTasks,
                                        std::shared_ptr<MetricSummary> &pmu, std::vector<SortedOpRow> &rows)
{
    // Length of the row GenerateOneTaskRow builds, taken before the pmu labels are appended so the
    // two can never drift apart.
    const size_t baseCols = headers_.size();
    if (pmu != nullptr)
    {
        headers_.insert(headers_.end(), pmu->labels.begin(), pmu->labels.end());
    }
    // Cube utilization is the only pmu value derived from another column. Resolve its position once
    // instead of rebuilding the whole base row for every task.
    const int usageIdx = GetIndexForVec(headers_, CUBE_UTILIZATION);
    const bool usageInPmu = usageIdx >= 0 && static_cast<size_t>(usageIdx) >= baseCols;
    rows.reserve(rows.size() + associatedTasks.records.size());
    for (size_t recordIndex = 0; recordIndex < associatedTasks.records.size(); ++recordIndex)
    {
        const auto &item = associatedTasks.records[recordIndex];
        if (!item.opSummaryRequired)
        {
            continue;
        }
        TaskId taskId{item.ascendTask->streamId, item.ascendTask->batchId, item.ascendTask->taskId,
                      item.ascendTask->contextId, item.ascendTask->deviceId};
        SortedOpRow key;
        key.deviceId = item.ascendTask->deviceId;
        key.timestamp = item.ascendTask->timestamp;
        key.duration = item.ascendTask->end - item.ascendTask->timestamp;
        key.index = recordIndex;
        if (pmu != nullptr)
        {
            auto it = pmu->data.find(taskId);
            if (it != pmu->data.end() && !it->second.empty())
            {
                // Consume the first pmu row for this task, exactly like the emitter used to.
                key.extra = it->second.front();
                it->second.erase(it->second.begin());
                if (it->second.empty())
                {
                    pmu->data.erase(it);
                }
                const size_t offset = static_cast<size_t>(usageIdx) - baseCols;
                if (usageInPmu && offset < key.extra.size() && key.extra[offset] != NA)
                {
                    ComputeCubeUsage(key.extra[offset], DivideByPowersOfTenWithPrecision(key.duration));
                }
            }
            else
            {
                key.pad = true;
            }
        }
        rows.emplace_back(std::move(key));
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

void OpSummaryAssembler::EmitOpSummary(std::vector<SortedOpRow> &rows, const AssociatedTaskCollection *associatedTasks,
                                       const std::vector<CommunicationOpData> *hcclData, const std::set<int> &maskCols)
{
    std::sort(rows.begin(), rows.end(),
              [](const SortedOpRow &lv, const SortedOpRow &rv)
              {
                  if (lv.deviceId == rv.deviceId)
                  {
                      return lv.timestamp < rv.timestamp;
                  }
                  return lv.deviceId < rv.deviceId;
              });
    const int timeIndex = GetIndexForVec(headers_, TASK_START_TIME);
    const int waitIndex = GetIndexForVec(headers_, TASK_WAIT_TIME);
    const std::string fileName = File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, OP_SUMMARY_NAME});
    auto &consumer = DeliverableConsumer::Instance();
    consumer.StartCsv(fileName, headers_, maskCols, "", rows.size());
    std::vector<std::vector<std::string>> batch;
    batch.reserve(std::min(CSV_ROW_BATCH, rows.size()));
    auto flush = [&consumer, &fileName, &batch]()
    {
        if (batch.empty())
        {
            return;
        }
        consumer.AppendCsv(fileName, batch);
        batch.clear();
    };
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const SortedOpRow &key = rows[i];
        std::vector<std::string> record;
        if (key.hccl)
        {
            const auto &data = hcclData->at(key.index);
            record = {std::to_string(data.deviceId),
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
            if (headers_.size() > record.size())
            {
                record.insert(record.end(), headers_.size() - record.size(), NA);
            }
        }
        else
        {
            const auto &item = associatedTasks->records[key.index];
            record = GenerateOneTaskRow(*item.taskInfo, *item.ascendTask);
            if (!key.extra.empty())
            {
                record.insert(record.end(), key.extra.begin(), key.extra.end());
            }
            else if (key.pad && headers_.size() > record.size())
            {
                record.insert(record.end(), headers_.size() - record.size(), NA);
            }
        }
        if (i > 0 && waitIndex >= 0 && static_cast<size_t>(waitIndex) < record.size())
        {
            double currStart = 0.0;
            double preStart = 0.0;
            double preDur = 0.0;
            if (StrToDouble(currStart, DivideByPowersOfTenWithPrecision(rows[i].timestamp)) == ANALYSIS_OK &&
                StrToDouble(preStart, DivideByPowersOfTenWithPrecision(rows[i - 1].timestamp)) == ANALYSIS_OK &&
                StrToDouble(preDur, DivideByPowersOfTenWithPrecision(rows[i - 1].duration)) == ANALYSIS_OK)
            {
                const auto waitTime = std::max(0.0, currStart - preStart - preDur);
                record[static_cast<size_t>(waitIndex)] = std::to_string(waitTime);
            }
        }
        if (timeIndex >= 0 && timeIndex < static_cast<int>(record.size()))
        {
            record[static_cast<size_t>(timeIndex)] = FormatHighPrecisionForCsv(record[static_cast<size_t>(timeIndex)]);
        }
        batch.emplace_back(std::move(record));
        if (batch.size() >= CSV_ROW_BATCH)
        {
            flush();
        }
    }
    flush();
    consumer.FinishCsv(fileName);
}

uint8_t OpSummaryAssembler::AssembleData(DataInventory &dataInventory)
{
    auto associatedTasks = dataInventory.GetPtr<AssociatedTaskCollection>();
    auto hcclOpData = dataInventory.GetPtr<std::vector<CommunicationOpData>>();
    auto metricData = dataInventory.GetPtr<MetricSummary>();
    if (associatedTasks == nullptr && hcclOpData == nullptr)
    {
        WARN("No data to export op summary");
        return DATA_NOT_EXIST;
    }
    headers_ = BASE_HEADER;
    std::vector<SortedOpRow> rows;
    if (associatedTasks != nullptr)
    {
        headers_.insert(headers_.end(), ADDITIONAL_TENSOR_HEADER.begin(), ADDITIONAL_TENSOR_HEADER.end());
        GenerateOpBody(*associatedTasks, metricData, rows);
    }
    else
    {
        headers_.emplace_back(CONTEXT_ID);
    }
    if (hcclOpData != nullptr)
    {
        GenerateHcclBody(*hcclOpData, rows);
    }
    if (rows.empty())
    {
        ERROR("Can't match any task data, failed to generate op_summary.csv");
        return ASSEMBLE_FAILED;
    }
    EmitOpSummary(rows, associatedTasks.get(), hcclOpData.get(), GetMaskCols());
    return ASSEMBLE_SUCCESS;
}

}  // namespace Application
}  // namespace Analysis
