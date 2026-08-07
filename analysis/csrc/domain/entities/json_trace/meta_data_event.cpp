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

#include "analysis/csrc/domain/entities/json_trace/include/meta_data_event.h"

#include <unordered_map>

namespace Analysis
{
namespace Domain
{

MetaDataEvent::MetaDataEvent(uint32_t pid, int tid, const std::string &name) : TraceEvent(pid, tid, name) {}

void MetaDataEvent::ToJson(JsonWriter &ostream)
{
    TraceEvent::ToJson(ostream);
    ostream["ph"] << ph_;
    ostream["args"];
    // 嵌套json，需要再包一层{}，其中ProcessArgs函数为子类实现，子类只需要关注args参数部分实现，无需感知其他参数
    ostream.StartObject();
    ProcessArgs(ostream);
    ostream.EndObject();
}

MetaDataNameEvent::MetaDataNameEvent(uint32_t pid, uint32_t tid, const std::string &name, const std::string &argName)
    : MetaDataEvent(pid, tid, name), argsName_(argName)
{
}

void MetaDataNameEvent::ProcessArgs(JsonWriter &ostream) { ostream["name"] << argsName_; }

MetaDataLabelEvent::MetaDataLabelEvent(uint32_t pid, uint32_t tid, const std::string &name, const std::string &label)
    : MetaDataEvent(pid, tid, name), argsLabel_(label)
{
}

void MetaDataLabelEvent::ProcessArgs(JsonWriter &ostream) { ostream["labels"] << argsLabel_; }

MetaDataIndexEvent::MetaDataIndexEvent(uint32_t pid, uint32_t tid, const std::string &name, uint32_t index)
    : MetaDataEvent(pid, tid, name), argsSortIndex_(index)
{
}

void MetaDataIndexEvent::ProcessArgs(JsonWriter &ostream) { ostream["sort_index"] << argsSortIndex_; }

LayerInfo GetLayerInfo(std::string processName) { return LAYER_INFO.at(processName); }
}  // namespace Domain
}  // namespace Analysis
