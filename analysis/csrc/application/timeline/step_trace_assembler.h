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

#ifndef ANALYSIS_APPLICATION_TIMELINE_STEP_TRACE_ASSEMBLER_H
#define ANALYSIS_APPLICATION_TIMELINE_STEP_TRACE_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/step_trace_data.h"

namespace Analysis
{
namespace Application
{
class StepTraceAssembler : public JsonAssembler
{
   public:
    StepTraceAssembler();

   private:
    void GenerateTrainTrace(const std::vector<TrainTraceData> &trainData, const std::string &profPath,
                            std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layer);
    void GenerateFpBpTrace(const TrainTraceData &data, uint32_t pid, int tid);
    void GenerateRefreshTrace(const TrainTraceData &data, uint32_t pid, int tid);
    void GenerateDataAugTrace(const TrainTraceData &data, uint32_t pid, int tid);
    void GenerateMetaData(std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layer);
    void GenerateReduceTrace(const std::vector<AllReduceData> &reduceData, const std::string &profPath,
                             std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layer);
    void GenerateNextTrace(const std::vector<GetNextData> &nextData, const std::string &profPath,
                           std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layer);
    uint8_t AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath) override;

   private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
    std::set<std::pair<uint32_t, uint32_t>> pidTidSet_;
};

class StepTraceEvent : public DurationEvent
{
   public:
    StepTraceEvent(uint32_t pid, uint32_t tid, double dur, const std::string &ts, const std::string &name,
                   const std::string &cat, uint32_t indexId, double iterTime, const std::string fpStart,
                   const std::string bpEnd, const std::string iterEnd)
        : DurationEvent(pid, tid, dur, ts, name, cat),
          indexId_(indexId),
          iterTime_(iterTime),
          fpStart_(fpStart),
          bpEnd_(bpEnd),
          iterEnd_(iterEnd)
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    uint32_t indexId_;
    double iterTime_;
    std::string fpStart_;
    std::string bpEnd_;
    std::string iterEnd_;
};

class FpBpTraceEvent : public DurationEvent
{
   public:
    FpBpTraceEvent(uint32_t pid, uint32_t tid, double dur, const std::string &ts, const std::string &name,
                   const std::string &cat, uint32_t indexId, double fpBpTime, const std::string fpStart,
                   const std::string bpEnd)
        : DurationEvent(pid, tid, dur, ts, name, cat),
          indexId_(indexId),
          fpBpTime_(fpBpTime),
          fpStart_(fpStart),
          bpEnd_(bpEnd)
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    uint32_t indexId_;
    double fpBpTime_;
    std::string fpStart_;
    std::string bpEnd_;
};

class RefreshTraceEvent : public DurationEvent
{
   public:
    RefreshTraceEvent(uint32_t pid, uint32_t tid, double dur, const std::string &ts, const std::string &name,
                      const std::string &cat, uint32_t indexId, double refresh, const std::string bpEnd,
                      const std::string iterEnd)
        : DurationEvent(pid, tid, dur, ts, name, cat),
          indexId_(indexId),
          refresh_(refresh),
          bpEnd_(bpEnd),
          iterEnd_(iterEnd)
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    uint32_t indexId_;
    double refresh_;
    std::string bpEnd_;
    std::string iterEnd_;
};

class DataAug1TraceEvent : public TraceEvent
{
   public:
    DataAug1TraceEvent(uint32_t pid, uint32_t tid, const std::string &ts, const std::string &name,
                       const std::string &cat, const std::string &id, double arg)
        : TraceEvent(pid, tid, name), ts_(ts), cat_(cat), id_(id), arg_(arg)
    {
    }

   private:
    void ToJson(JsonWriter &ostream) override;

   private:
    std::string ts_;
    std::string cat_;
    std::string id_;
    std::string ph_ = "s";
    double arg_;
};

class DataAug0TraceEvent : public TraceEvent
{
   public:
    DataAug0TraceEvent(uint32_t pid, uint32_t tid, const std::string &ts, const std::string &name,
                       const std::string &cat, const std::string &id, uint32_t arg)
        : TraceEvent(pid, tid, name), ts_(ts), cat_(cat), id_(id), arg_(arg)
    {
    }

   private:
    void ToJson(JsonWriter &ostream) override;

   private:
    std::string ts_;
    std::string cat_;
    std::string id_;
    std::string ph_ = "t";
    uint32_t arg_;
};

class ReduceTraceEvent : public DurationEvent
{
   public:
    ReduceTraceEvent(uint32_t pid, uint32_t tid, double dur, const std::string &ts, const std::string &name,
                     const std::string &cat, uint32_t indexId, std::unordered_map<std::string, std::string> &args)
        : DurationEvent(pid, tid, dur, ts, name, cat), indexId_(indexId), args_(std::move(args))
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    uint32_t indexId_;
    std::unordered_map<std::string, std::string> args_;
};

class NextTraceEvent : public DurationEvent
{
   public:
    NextTraceEvent(uint32_t pid, uint32_t tid, double dur, const std::string &ts, const std::string &name,
                   const std::string &cat, const std::string &start, const std::string &end, double time)
        : DurationEvent(pid, tid, dur, ts, name, cat), start_(start), end_(end), time_(time)
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    std::string start_;
    std::string end_;
    double time_;
};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_TIMELINE_STEP_TRACE_ASSEMBLER_H
