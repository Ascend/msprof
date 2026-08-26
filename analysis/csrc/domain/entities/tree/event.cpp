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

#include "analysis/csrc/domain/entities/tree/include/event.h"

#include <utility>

namespace Analysis
{
namespace Domain
{

std::atomic<uint32_t> g_eventIDCnt{0};

Event::Event(std::shared_ptr<ParserApi> eventPtr, const EventInfo &eventInfo)
    : apiPtr(std::move(eventPtr)), info(eventInfo), id(++g_eventIDCnt)
{
}

Event::Event(std::shared_ptr<ParserAdditionalInfo> eventPtr, const EventInfo &eventInfo)
    : additionPtr(std::move(eventPtr)), info(eventInfo), id(++g_eventIDCnt)
{
}

Event::Event(std::shared_ptr<ParserCompactInfo> eventPtr, const EventInfo &eventInfo)
    : compactPtr(std::move(eventPtr)), info(eventInfo), id(++g_eventIDCnt)
{
}

Event::Event(std::shared_ptr<ParserConcatTensorInfo> eventPtr, const EventInfo &eventInfo)
    : tensorPtr(std::move(eventPtr)), info(eventInfo), id(++g_eventIDCnt)
{
}
}  // namespace Domain
}  // namespace Analysis
