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

#include "analysis/csrc/domain/entities/tree/include/event_queue.h"

#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/time_logger.h"

namespace Analysis
{
namespace Domain
{

EventQueue::EventQueue(uint32_t threadId, uint64_t initSize) : threadId_(threadId), initSize_(initSize)
{
    data_.reserve(initSize);  // initSize = Api size + Tasktrack size
}

void EventQueue::Sort()
{
    std::string loggerName = "Sort event num " + std::to_string(tail_) + " in thread " + std::to_string(threadId_);
    Utils::TimeLogger logger(loggerName);
    auto comp = [](const std::shared_ptr<Event> &event1, const std::shared_ptr<Event> &event2)
    {
        return event1->info.start < event2->info.start ||
               (event1->info.start == event2->info.start && event1->info.level > event2->info.level);
    };
    std::sort(data_.begin(), data_.begin() + tail_, comp);
}

void EventQueue::Push(const std::shared_ptr<Event> &event)
{
    if (!event)
    {
        ERROR("Event pointer is nullptr");
        return;
    }

    if (tail_ == initSize_)
    {
        ERROR("The number of added events exceeds the maximum EventQueue capacity");
        return;
    }
    data_.emplace_back(event);
    tail_ += 1;
    bound_ = std::max(event->info.end, bound_);
}

std::shared_ptr<Event> EventQueue::Pop()
{
    if (Empty())
    {
        ERROR("Can not Pop when EventQueue is empty");
        return nullptr;
    }
    auto event = data_[head_];
    head_ += 1;
    return event;
}

std::shared_ptr<Event> EventQueue::Top()
{
    if (Empty())
    {
        INFO("EventQueue for thread % is empty", threadId_);
        return nullptr;
    }
    return data_[head_];
}

bool EventQueue::Empty() const { return head_ >= tail_; }

uint64_t EventQueue::GetBound() const { return bound_; }

uint64_t EventQueue::GetSize() const
{
    if (Empty())
    {
        return 0;
    }
    return tail_ - head_;
}

}  // namespace Domain
}  // namespace Analysis
