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

#include "analysis/csrc/domain/services/modeling/step_trace/include/step_trace_process.h"

#include <algorithm>
#include <deque>

#include "analysis/csrc/domain/services/modeling/step_trace/model_step_trace.h"
#include "analysis/csrc/domain/services/parser/track/include/ts_track_parser.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Infra;
namespace
{
const int MAX_START_NUM = 2;

class StepTracePreprocess
{
   public:
    StepTracePreprocess()
    {
        labelHandlers = {
            {StepLabel::ModelStartLabel,
             std::bind(&StepTracePreprocess::HandleModelStartLabel, this, std::placeholders::_1)},
            {StepLabel::ModelEndLabel,
             std::bind(&StepTracePreprocess::HandleModelEndLabel, this, std::placeholders::_1)},
            {StepLabel::GetNextLabel, std::bind(&StepTracePreprocess::HandleGetNextLabel, this, std::placeholders::_1)},
            {StepLabel::AllReduceLabel,
             std::bind(&StepTracePreprocess::HandleAllReduceLabel, this, std::placeholders::_1)},
            {StepLabel::TrainingTraceLabel,
             std::bind(&StepTracePreprocess::HandleIterationLabel, this, std::placeholders::_1)},
            {StepLabel::MstxLabel, std::bind(&StepTracePreprocess::HandleMstxLabel, this, std::placeholders::_1)}};
    };

    std::vector<HalTrackData> Run(const std::vector<HalTrackData>& datas)
    {
        for (const HalTrackData& record : datas)
        {
            if (record.stepTrace.modelId != currentModeId_)
            {
                for (auto& data : currentStepTraceQueue_)
                {
                    reorderedStepTrace_.insert(reorderedStepTrace_.end(), data.allRecord.begin(), data.allRecord.end());
                }
                currentStepTraceQueue_.clear();
                currentModeId_ = record.stepTrace.modelId;
            }
            labelHandlers[TransTagIdToLabel(record.stepTrace.tagId)](record);
        }
        if (!currentStepTraceQueue_.empty())
        {
            for (auto& data : currentStepTraceQueue_)
            {
                reorderedStepTrace_.insert(reorderedStepTrace_.end(), data.allRecord.begin(), data.allRecord.end());
            }
            currentStepTraceQueue_.clear();
        }
        return reorderedStepTrace_;
    }

   private:
    StepLabel TransTagIdToLabel(uint16_t tagId)
    {
        if (tagId == MODEL_START_TAG)
        {
            return StepLabel::ModelStartLabel;
        }
        else if (tagId == MODEL_END_TAG)
        {
            return StepLabel::ModelEndLabel;
        }
        else if (tagId >= GET_NEXT_START_TAG && tagId < STEP_START_TAG)
        {
            return StepLabel::GetNextLabel;
        }
        else if (tagId >= ALL_REDUCE_START)
        {
            return StepLabel::AllReduceLabel;
        }
        else if (tagId == MSTX_TAG)
        {
            return StepLabel::MstxLabel;
        }
        else
        {
            return StepLabel::TrainingTraceLabel;
        }
    }

    // 处理模型开始标签的函数
    void HandleModelStartLabel(const HalTrackData& record) { currentStepTraceQueue_.push_back({{record}, {record}}); }

    // 处理模型结束标签的函数
    void HandleModelEndLabel(const HalTrackData& record)
    {
        int startTagNum = 0;
        while (!currentStepTraceQueue_.empty())
        {
            if (currentStepTraceQueue_.front().tag.front().stepTrace.tagId == MODEL_START_TAG)
            {
                startTagNum += 1;
                if (startTagNum == MAX_START_NUM)
                {
                    break;
                }
            }
            reorderedStepTrace_.insert(reorderedStepTrace_.end(), currentStepTraceQueue_.front().allRecord.begin(),
                                       currentStepTraceQueue_.front().allRecord.end());
            currentStepTraceQueue_.pop_front();
        }
        reorderedStepTrace_.emplace_back(record);
    }

    // 处理GETNEXT标签的函数
    void HandleGetNextLabel(const HalTrackData& record)
    {
        if (!currentStepTraceQueue_.empty())
        {
            currentStepTraceQueue_.back().allRecord.emplace_back(record);
        }
    }

    // 处理ALLREDUCE标签的函数
    void HandleAllReduceLabel(const HalTrackData& record)
    {
        for (auto& data : currentStepTraceQueue_)
        {
            if (data.tag.back().stepTrace.tagId != ITER_END_TAG)
            {
                data.allRecord.emplace_back(record);
                break;
            }
        }
    }

    // 处理迭代标签的函数
    void HandleIterationLabel(const HalTrackData& record)
    {
        bool isNewIteration = true;
        for (auto& data : currentStepTraceQueue_)
        {
            if (data.tag.back().stepTrace.tagId < record.stepTrace.tagId)
            {
                data.tag.emplace_back(record);
                data.allRecord.emplace_back(record);
                isNewIteration = false;
                break;
            }
        }
        if (isNewIteration)
        {
            currentStepTraceQueue_.push_back({{record}, {record}});
        }
    }

    // 处理mstx数据
    void HandleMstxLabel(const HalTrackData& record) { currentStepTraceQueue_.push_back({{record}, {record}}); }

   private:
    struct StepData
    {
        std::vector<HalTrackData> tag;
        std::vector<HalTrackData> allRecord;
    };
    uint64_t currentModeId_{UINT64_MAX};
    std::deque<StepData> currentStepTraceQueue_;
    std::vector<HalTrackData> reorderedStepTrace_;
    // 表驱动：映射标签ID到处理函数
    std::unordered_map<StepLabel, std::function<void(const HalTrackData&)>> labelHandlers;
};
}  // namespace

bool Compare(const HalTrackData& a, const HalTrackData& b)
{
    if (a.stepTrace.modelId == b.stepTrace.modelId)
    {
        return a.stepTrace.timestamp < b.stepTrace.timestamp;
    }
    return a.stepTrace.modelId < b.stepTrace.modelId;
}

std::vector<HalTrackData> StepTraceProcess::PreprocessData(std::vector<HalTrackData>& data)
{
    std::sort(data.begin(), data.end(), Compare);
    auto preprocessor = StepTracePreprocess();
    return preprocessor.Run(data);
}

void StepTraceProcess::SaveStepTraceTask()
{
    if (!currentStepTraceTask_.empty())
    {
        if (currentStepTraceTask_.back().stepTrace.start >= currentStepTraceTask_.back().stepTrace.end ||
            !currentStepTraceTask_.back().stepTrace.start)
        {
            currentStepTraceTask_.pop_back();
        }
        if (!currentStepTraceTask_.empty())
        {
            stepTraceTasks_[currentModeId_] = currentStepTraceTask_;
        }
        currentStepTraceTask_.clear();
    }
}

uint32_t StepTraceProcess::ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context&)
{
    INFO("Start to process step trace data");
    auto trackData = dataInventory.GetPtr<std::vector<HalTrackData>>();
    if (!trackData)
    {
        ERROR("Ts track data is null");
        return Analysis::ANALYSIS_ERROR;
    }
    currentModeId_ = UINT64_MAX;
    currentStepTraceTask_.clear();
    stepTraceTasks_.clear();
    auto oriData = GetTrackDataByType(*trackData, STEP_TRACE);
    if (oriData.empty())
    {
        WARN("stepData is empty");
        return Analysis::ANALYSIS_OK;
    }
    // 预处理step trace数据
    auto stepData = PreprocessData(oriData);
    // 状态机处理
    ModelStepTrace modelStepTrace{};
    for (auto& step : stepData)
    {
        if (step.stepTrace.modelId != currentModeId_)
        {
            SaveStepTraceTask();
            currentModeId_ = step.stepTrace.modelId;
            modelStepTrace.Init();
        }
        modelStepTrace.OnStep(step, currentStepTraceTask_);
    }
    SaveStepTraceTask();
    std::shared_ptr<StepTraceTaskMap> data;
    MAKE_SHARED_RETURN_VALUE(data, StepTraceTaskMap, ANALYSIS_ERROR, std::move(stepTraceTasks_));
    dataInventory.Inject(data);
    return Analysis::ANALYSIS_OK;
}

REGISTER_PROCESS_SEQUENCE(StepTraceProcess, false, TsTrackParser);
REGISTER_PROCESS_DEPENDENT_DATA(StepTraceProcess, std::vector<HalTrackData>);
REGISTER_PROCESS_SUPPORT_CHIP(StepTraceProcess, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
