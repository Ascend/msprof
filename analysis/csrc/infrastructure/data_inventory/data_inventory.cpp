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
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"

using namespace Analysis;

namespace Analysis
{

namespace Infra
{

bool DataInventory::InputToData(std::type_index idx, BaseTypePtr ptr)
{
    if (ptr == nullptr)
    {
        ERROR("invalid ptr, type name: %", idx.name());
        return false;
    }

    std::lock_guard<std::mutex> lg(mutex_);
    auto ret = data_.emplace(idx, ptr);
    return ret.second;
}

std::set<std::type_index> DataInventory::RemoveRestData(const std::set<std::type_index>& keepingDataType)
{
    std::set<std::type_index> removedTypes;
    std::lock_guard<std::mutex> lg(mutex_);
    for (auto it = data_.begin(); it != data_.end();)
    {
        if (std::find(keepingDataType.begin(), keepingDataType.end(), it->first) == std::end(keepingDataType))
        {
            removedTypes.insert(it->first);
            it = data_.erase(it);
            continue;
        }
        ++it;
    }
    return removedTypes;
}

BaseTypePtr DataInventory::GetPtr(std::type_index idx) const
{
    std::lock_guard<std::mutex> lg(mutex_);
    auto it = data_.find(idx);
    if (it == data_.end())
    {
        return {};
    }

    return it->second;
}

}  // namespace Infra

}  // namespace Analysis
