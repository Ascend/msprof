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

#include "analysis/csrc/application/credential/id_pool.h"

#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Application
{
namespace Credential
{

namespace
{
const uint32_t DEVICE_ID = 0;
const uint32_t STREAM_ID = 1;
const uint32_t TASK_ID = 2;
const uint32_t CONTEXT_ID = 3;
const uint32_t BATCH_ID = 4;
const uint32_t DEV_TYPE_ID = 5;
}  // namespace

// 清除map数据
void IdPool::Clear()
{
    uint64Ids_.clear();
    correlationIds_.clear();
    uint32Ids_.clear();
    uint64Index_ = 0;
    correlationIndex_ = 0;
    uint32Index_ = 0;
}

// 获取string格式map的key对应value(64位)，如果不存在则添加(key, value)并返回value
uint64_t IdPool::GetUint64Id(const std::string& key)
{
    std::lock_guard<std::mutex> lock(uint64Mutex_);
    if (uint64Ids_.find(key) == uint64Ids_.end())
    {
        uint64Ids_.insert({key, uint64Index_});
        uint64Index_++;
    }
    return uint64Ids_[key];
}

// 获取string格式map的key对应value(32位)，如果不存在则添加(key, value)并返回value
uint32_t IdPool::GetUint32Id(const std::string& key)
{
    std::lock_guard<std::mutex> lock(uint32Mutex_);
    if (uint32Ids_.find(key) == uint32Ids_.end())
    {
        uint32Ids_.insert({key, uint32Index_});
        uint32Index_++;
    }
    return uint32Ids_[key];
}

// 获取CorrelationTuple格式map的key对应value，如果不存在则添加(key, value)并返回value
uint64_t IdPool::GetId(const CorrelationTuple& key)
{
    std::lock_guard<std::mutex> lock(correlationMutex_);
    std::string stringKey =
        Utils::Join("_", Utils::Join("_", std::get<DEVICE_ID>(key), std::get<STREAM_ID>(key)), std::get<TASK_ID>(key),
                    std::get<CONTEXT_ID>(key), std::get<BATCH_ID>(key), std::get<DEV_TYPE_ID>(key));
    if (correlationIds_.find(stringKey) == correlationIds_.end())
    {
        correlationIds_.insert({stringKey, correlationIndex_});
        correlationIndex_++;
    }
    return correlationIds_[stringKey];
}

// 输出stringIds_ map全部数据
std::unordered_map<std::string, uint64_t>& IdPool::GetAllUint64Ids()
{
    std::lock_guard<std::mutex> lock(uint64Mutex_);
    return uint64Ids_;
}

}  // namespace Credential
}  // namespace Application
}  // namespace Analysis
