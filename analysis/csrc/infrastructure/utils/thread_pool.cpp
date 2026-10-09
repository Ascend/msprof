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

#include "thread_pool.h"

#include <thread>

#include "analysis/csrc/infrastructure/dfx/log.h"
#include "utils.h"

namespace Analysis
{
namespace Utils
{

ThreadPool::ThreadPool(uint32_t threadsNum) : threadsNum_(threadsNum) {}

ThreadPool::~ThreadPool()
{
    if (running_)
    {
        Stop();
    }
}

bool ThreadPool::Start()
{
    // 避免多次调用
    if (running_)
    {
        ERROR("Do not call Start multiple times");
        return false;
    }

    if (threadsNum_ == 0)
    {
        ERROR("ThreadPool thread number is less than 0");
        return false;
    }

    running_ = true;
    auto ret = Utils::Reserve(threads_, threadsNum_);
    if (!ret)
    {
        ERROR("Reserve threads vector failed");
        return false;
    }
    for (uint32_t i = 0; i < threadsNum_; ++i)
    {
        std::thread t(&ThreadPool::Loop, this);
        threads_.emplace_back(std::move(t));
    }
    return true;
}

bool ThreadPool::Stop()
{
    // 防止多次调用
    if (!running_)
    {
        ERROR("Do not call Stop multiple times");
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 停止Loop
        running_ = false;
        // 释放阻塞在FetchTask的线程
        hasTaskToDo_.notify_all();
    }

    // 等待所有线程执行结束
    for (auto &t : threads_)
    {
        t.join();
    }
    return true;
}

void ThreadPool::AddTask(const Task &task)
{
    std::unique_lock<std::mutex> lock(mutex_);
    taskQueue_.emplace_back(
        [task]
        {
            try
            {
                task();
            }
            catch (const std::exception &ex)
            {
                ERROR("Thread[%] in Pool caught exception: %", std::this_thread::get_id(), ex.what());
            }
            catch (...)
            {
                ERROR("Thread[%] in Pool caught unknown exception", std::this_thread::get_id());
            }
        });
    // 释放一个阻塞在FetchTask的线程
    hasTaskToDo_.notify_one();
}

void ThreadPool::WaitAllTasks()
{
    std::unique_lock<std::mutex> lock(mutex_);
    waitTaskDone_.wait(lock, [this] { return taskQueue_.empty(); });
}

uint32_t ThreadPool::GetUnassignedTasksNum()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return taskQueue_.size();
}

uint32_t ThreadPool::GetThreadsNum() { return threads_.size(); }

bool ThreadPool::FetchTask(Task &task)
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (taskQueue_.empty() && running_)
    {
        // 若任务队列没有任务将会阻塞等待任务的添加
        hasTaskToDo_.wait(lock);
    }

    if (!taskQueue_.empty() && running_)
    {
        task = taskQueue_.front();
        taskQueue_.pop_front();
        waitTaskDone_.notify_one();
        return true;
    }
    return false;
}

void ThreadPool::Loop()
{
    Task task;
    while (running_)
    {
        if (FetchTask(task))
        {
            task();
        }
    }
}
}  // namespace Utils
}  // namespace Analysis
