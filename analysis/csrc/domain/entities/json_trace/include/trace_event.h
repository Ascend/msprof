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

#ifndef ANALYSIS_DOMAIN_JSON_TRACE_TRACE_EVENT_H
#define ANALYSIS_DOMAIN_JSON_TRACE_TRACE_EVENT_H

#include <string>
#include <unordered_map>

#include "analysis/csrc/infrastructure/dump_tools/json_tool/include/json_writer.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;
/**
 * TraceEvent为对标chrome:tracing解析的trace格式数据的抽象父类，有三个共同成员：pid, tid, name
 */
class TraceEvent
{
   public:
    void DumpJson(JsonWriter &ostream)
    {
        ostream.StartObject();
        ToJson(ostream);
        ostream.EndObject();
    }
    TraceEvent(uint32_t pid, uint32_t tid, const std::string &name);
    virtual ~TraceEvent() = default;

   protected:
    virtual void ToJson(JsonWriter &ostream);

   private:
    uint32_t pid_;
    uint32_t tid_;
    std::string name_;
};

/**
 * DurationEvent对应chrome:tracing的持续性事件, Duration Events是将所有pid、tid、时间段匹配的事件整合在一起，
 * 以栈的方式显示，其中栈顶在上，栈底在下。这种事件类型主要用于表示一段时间内发生的事件，通过整合这些事件，
 * 可以清晰地展示在特定时间段内哪些函数或操作被调用，以及它们之间的调用关系。
 */
class DurationEvent : public TraceEvent
{
   public:
    DurationEvent(uint32_t pid, uint32_t tid, double dur, const std::string &ts, const std::string &name,
                  const std::string &cat = " ");

   private:
    void ToJson(JsonWriter &ostream) override;
    virtual void ProcessArgs(JsonWriter &ostream) {};

   private:
    double dur_;
    std::string ts_;
    std::string ph_ = "X";
    std::string cat_;
};

/**
 *  CounterEvent对应chrome:tracing的计数器事件，Counter
 * Events用于记录和跟踪应用程序中的计数器变化。通过收集和分析这些事件，
 *  以柱状图形式展示出来，使得开发者能够直观地看到计数器的变化趋势，进而分析性能瓶颈、资源使用情况等问题
 */
class CounterEvent : public TraceEvent
{
   public:
    CounterEvent(uint32_t pid, uint32_t tid, const std::string &ts, const std::string &name);
    void SetSeriesDValue(const std::string &key, const double &value);
    void SetSeriesIValue(const std::string &key, const uint64_t &value);

   private:
    void ToJson(JsonWriter &ostream) override;
    void ProcessArgs(JsonWriter &ostream);

   private:
    std::string ts_;
    std::string ph_ = "C";
    std::unordered_map<std::string, double> seriesDValue_;
    std::unordered_map<std::string, uint64_t> seriesIValue_;
};

/**
 *  FlowEvent对应chrome:tracing的序列事件，Flow Events通常指的是在应用程序运行过程中，各个组件或模块之间传递和处理数据的
 *  事件。这些事件记录了数据从源头到目标经过的路径，以及在这个过程中可能发生的转换或处理。
 *  通过这些Flow Events，可以了解应用程序内部的数据流动情况。
 */
class FlowEvent : public TraceEvent
{
   public:
    FlowEvent(uint32_t pid, uint32_t tid, const std::string &ts, const std::string &cat, const std::string &id,
              const std::string &name, const std::string &ph, const std::string &bp = " ");

   private:
    void ToJson(JsonWriter &ostream) override;

   private:
    std::string ts_;
    std::string cat_;
    std::string id_;
    std::string ph_;
    std::string bp_;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_JSON_TRACE_TRACE_EVENT_H
