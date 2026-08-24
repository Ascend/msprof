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
/* 非常重要，使用DataInventory缓存的数据，
   如果没有用REGISTER_PROCESS_DEPENDENT_DATA注册，会立即被释放 */

#ifndef ANALYSIS_DEVICE_TASK_DATAINVENTORY_DATA_INVENTORY_H
#define ANALYSIS_DEVICE_TASK_DATAINVENTORY_DATA_INVENTORY_H

#include <memory>
#include <mutex>
#include <set>
#include <typeindex>
#include <unordered_map>

#include "analysis/csrc/infrastructure/utils/utils.h"

using Analysis::Format;
using Analysis::Log;

namespace Analysis
{
namespace Infra
{

struct BaseType
{
    virtual ~BaseType() = default;
};
using BaseTypePtr = std::shared_ptr<BaseType>;

template <typename T>
struct CustomType : public BaseType
{
    std::shared_ptr<T> data;
};

template <typename T>
struct SharedPtrType
{
    using Type = std::shared_ptr<T>;
};

class DataInventory
{
   public:
    /**
     * @brief 将数据存放入DataInventory
     *
     * @tparam T 数据类型
     * @param ptr 数据实例
     * @return 成功或失败
     * @note 非常重要：使用本接口缓存的数据，如果没有用REGISTER_PROCESS_DEPENDENT_DATA注册，会立即被释放
     */
    template <typename T>
    bool Inject(std::shared_ptr<T> ptr)
    {
        if (ptr == nullptr)
        {
            return false;
        }

        if (GetPtr<T>() != nullptr)
        {  // 不支持注入相同类型数据
            return false;
        }

        std::shared_ptr<CustomType<T>> container;
        MAKE_SHARED0_RETURN_VALUE(container, CustomType<T>, false);
        container->data = ptr;
        return InputToData(typeid(T), container);
    }
    template <typename T>
    std::shared_ptr<T> GetPtr() const
    {
        return Cast<T>(GetPtr(typeid(T)));
    }

    /**
     * @brief 输入需要保留的数据类型，删除其它数据
     *
     * @param keepingDataType 需要保留的数据类型列表
     * @return 释放的类型集合
     */
    std::set<std::type_index> RemoveRestData(const std::set<std::type_index>& keepingDataType);

    std::size_t Size() { return data_.size(); }

    DataInventory() = default;
    ~DataInventory() = default;
    DataInventory(const DataInventory&) = delete;
    DataInventory& operator=(const DataInventory&) = delete;
    DataInventory(DataInventory&& dataInventory) : data_(std::move(dataInventory.data_)) {}
    DataInventory& operator=(DataInventory&& dataInventory)
    {
        if (&dataInventory == this)
        {
            return *this;
        }
        data_.swap(dataInventory.data_);
        dataInventory.data_.clear();
        return *this;
    }

   private:
    template <typename T>
    std::shared_ptr<T> Cast(const BaseTypePtr& ptr) const
    {
        if (ptr == nullptr)
        {
            return {};
        }
        auto customPtr = std::static_pointer_cast<CustomType<T>>(ptr);
        return customPtr->data;
    }
    bool InputToData(std::type_index idx, BaseTypePtr ptr);
    BaseTypePtr GetPtr(std::type_index idx) const;

   private:
    std::unordered_map<std::type_index, BaseTypePtr> data_;
    mutable std::mutex mutex_;
};

}  // namespace Infra

}  // namespace Analysis

#endif
