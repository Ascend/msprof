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

#include "analysis/csrc/application/summary/summary_step_trace_assembler.h"

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
const std::string HEADER_REDUCE_START = "Reduce Start(us)";
const std::string HEADER_REDUCE_DURATION = "Reduce Duration(us)";

// 与 Python StepTraceViewer.get_step_trace_data 的 SQL 语义对齐：训练数据缺失时落盘哨兵为 0，导出时应渲染为 N/A。
std::string FormatMissingFieldAsNa(uint64_t value, bool isHighPrecision = false)
{
    return value == 0 ? NA : DivideByPowersOfTenWithPrecision(value, isHighPrecision);
}
}  // namespace

void SummaryStepTraceAssembler::AddAllReduceHeaders()
{
    if (!formatedAllReduceData_.empty())
    {
        for (auto &allReducePair : formatedAllReduceData_)
        {
            uint64_t allReduceGroupSize = allReducePair.second.size();
            allReduceGroupMaxSize_ =
                allReduceGroupSize > allReduceGroupMaxSize_ ? allReduceGroupSize : allReduceGroupMaxSize_;
        }
    }

    for (uint64_t i = 0; i < allReduceGroupMaxSize_; ++i)
    {
        headers_.emplace_back(HEADER_REDUCE_START);
        headers_.emplace_back(HEADER_REDUCE_DURATION);
    }
}

SummaryStepTraceAssembler::SummaryStepTraceAssembler(const std::string &name, const std::string &profPath)
    : SummaryAssembler(name, profPath)
{
    headers_ = {
        "Device_id",          "Iteration ID",      "FP Start(us)",          "BP End(us)",         "Iteration End(us)",
        "Iteration Time(us)", "FP to BP Time(us)", "Iteration Refresh(us)", "Data Aug Bound(us)", "Model ID"};
}

uint8_t SummaryStepTraceAssembler::AssembleData(DataInventory &dataInventory)
{
    auto trainTraceData = dataInventory.GetPtr<std::vector<TrainTraceData>>();
    auto allReduceData = dataInventory.GetPtr<std::vector<AllReduceData>>();

    if (trainTraceData == nullptr)
    {
        WARN("trainTraceData not exists, can't export step trace data.");
        return DATA_NOT_EXIST;
    }

    if (allReduceData == nullptr)
    {
        WARN(
            "No all reduce data collected, maybe the all_reduce table is not created, "
            "now try to export data with no all reduce");
    }
    else
    {
        FormatAllReduceData(*allReduceData);
    }

    // get final headers
    AddAllReduceHeaders();
    // assemble trace and all reduce data
    AssembleStepTraceData(*trainTraceData);

    if (res_.empty())
    {
        ERROR("Can't match any step trace data, failed to generate step_trace_*.csv");
        return ASSEMBLE_FAILED;
    }

    WriteToFile(File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, STEP_TRACE_SUMMARY_NAME}), {});

    return ASSEMBLE_SUCCESS;
}

void SummaryStepTraceAssembler::FormatAllReduceData(const std::vector<AllReduceData> &allReduceData)
{
    if (allReduceData.empty())
    {
        WARN("all reduce data is empty, no all reduce data for step trace, check table all_reduce please.");
        return;
    }
    for (auto &allReduceDatum : allReduceData)
    {
        TraceId traceId = {allReduceDatum.modelId, allReduceDatum.iterEnd};
        auto it = formatedAllReduceData_.find(traceId);
        if (it != formatedAllReduceData_.end())
        {
            it->second.emplace_back(DivideByPowersOfTenWithPrecision(allReduceDatum.timestamp),
                                    DivideByPowersOfTenWithPrecision(allReduceDatum.end - allReduceDatum.timestamp));
        }
        else
        {
            formatedAllReduceData_[traceId] = {
                {DivideByPowersOfTenWithPrecision(allReduceDatum.timestamp),
                 DivideByPowersOfTenWithPrecision(allReduceDatum.timestamp - allReduceDatum.end)}};
        }
    }
}

void SummaryStepTraceAssembler::AssembleStepTraceData(const std::vector<TrainTraceData> &trainTraceData)
{
    if (trainTraceData.empty())
    {
        WARN("train trace data is empty, no train trace data for step trace, check table training_trace please.");
        return;
    }

    for (auto &trainTraceDatum : trainTraceData)
    {
        TraceId traceId = {trainTraceDatum.modelId, trainTraceDatum.iterEnd};
        // FP Start / BP End / Iteration Time / FP to BP Time / Iteration Refresh / Data Aug Bound 缺失时为 0，
        // 与 Python 导出一致渲染为 N/A；Iteration End 与 Model ID 无哨兵语义，保持原样格式化。
        std::vector<std::string> row = {std::to_string(trainTraceDatum.deviceId),
                                        std::to_string(trainTraceDatum.indexId),
                                        FormatMissingFieldAsNa(trainTraceDatum.fpStart, true),
                                        FormatMissingFieldAsNa(trainTraceDatum.bpEnd, true),
                                        DivideByPowersOfTenWithPrecision(trainTraceDatum.iterEnd, true),
                                        FormatMissingFieldAsNa(trainTraceDatum.iterTime),
                                        FormatMissingFieldAsNa(trainTraceDatum.fpBpTime),
                                        FormatMissingFieldAsNa(trainTraceDatum.gradRefreshBound),
                                        FormatMissingFieldAsNa(trainTraceDatum.dataAugBound),
                                        std::to_string(trainTraceDatum.modelId)};
        auto it = formatedAllReduceData_.find(traceId);
        if (it != formatedAllReduceData_.end())
        {
            int count = 0;
            for (auto &allReduceData : it->second)
            {
                row.emplace_back(FormatHighPrecisionForCsv(allReduceData.first));
                row.emplace_back(allReduceData.second);
                ++count;
            }
            row.insert(row.end(), allReduceGroupMaxSize_ - count, NA);
        }
        else
        {
            row.insert(row.end(), allReduceGroupMaxSize_, NA);
        }
        res_.emplace_back(row);
    }
}

}  // namespace Application
}  // namespace Analysis
