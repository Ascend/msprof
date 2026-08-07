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

#include "analysis/csrc/domain/entities/json_trace/include/trace_event.h"

namespace Analysis
{
namespace Domain
{
TraceEvent::TraceEvent(uint32_t pid, uint32_t tid, const std::string &name) : pid_(pid), tid_(tid), name_(name) {}

void TraceEvent::ToJson(JsonWriter &ostream)
{
    ostream["name"] << name_;
    ostream["pid"] << pid_;
    ostream["tid"] << tid_;
}

DurationEvent::DurationEvent(uint32_t pid, uint32_t tid, double dur, const std::string &ts, const std::string &name,
                             const std::string &cat)
    : TraceEvent(pid, tid, name), dur_(dur), ts_(ts), cat_(cat)
{
}
void DurationEvent::ToJson(JsonWriter &ostream)
{
    TraceEvent::ToJson(ostream);
    ostream["ts"] << ts_;
    ostream["dur"] << dur_;
    ostream["ph"] << ph_;
    if (cat_ != " ")
    {
        ostream["cat"] << cat_;
    }
    ostream["args"];
    // 嵌套json，需要再包一层{}，其中ProcessArgs函数为子类实现，子类只需要关注args参数部分实现，无需感知其他参数
    ostream.StartObject();
    ProcessArgs(ostream);
    ostream.EndObject();
}

CounterEvent::CounterEvent(uint32_t pid, uint32_t tid, const std::string &ts, const std::string &name)
    : TraceEvent(pid, tid, name), ts_(ts)
{
}
void CounterEvent::ToJson(JsonWriter &ostream)
{
    TraceEvent::ToJson(ostream);
    ostream["ts"] << ts_;
    ostream["ph"] << ph_;
    ostream["args"];
    ostream.StartObject();
    ProcessArgs(ostream);
    ostream.EndObject();
}

void CounterEvent::ProcessArgs(JsonWriter &ostream)
{
    for (const auto &kv : seriesDValue_)
    {
        ostream[kv.first.c_str()] << kv.second;
    }
    for (const auto &kv : seriesIValue_)
    {
        ostream[kv.first.c_str()] << kv.second;
    }
}

void CounterEvent::SetSeriesDValue(const std::string &key, const double &value) { seriesDValue_[key] = value; }

void CounterEvent::SetSeriesIValue(const std::string &key, const uint64_t &value) { seriesIValue_[key] = value; }

FlowEvent::FlowEvent(uint32_t pid, uint32_t tid, const std::string &ts, const std::string &cat, const std::string &id,
                     const std::string &name, const std::string &ph, const std::string &bp)
    : TraceEvent(pid, tid, name), ts_(ts), cat_(cat), id_(id), ph_(ph), bp_(bp)
{
}
void FlowEvent::ToJson(JsonWriter &ostream)
{
    TraceEvent::ToJson(ostream);
    ostream["ph"] << ph_;
    ostream["cat"] << cat_;
    ostream["id"] << id_;
    ostream["ts"] << ts_;
    if (bp_ != " ")
    {
        ostream["bp"] << bp_;
    }
}
}  // namespace Domain
}  // namespace Analysis
