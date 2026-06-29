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

#ifndef ANALYSIS_DOMAIN_DATA_PROCESSOR_H
#define ANALYSIS_DOMAIN_DATA_PROCESSOR_H

#include <functional>
#include <set>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/db/include/db_info.h"

namespace Analysis
{
namespace Domain
{
const uint8_t CHECK_SUCCESS = 0;
const uint8_t NOT_EXIST = 1;
const uint8_t CHECK_FAILED = 2;

using namespace Analysis::Infra;
using namespace Analysis::Application;
// 该类用于定义处理父类
// 主要包括以下特性：用于规范各db处理流程
// 1、Run拉起processor整体流程
// 2、以PROF文件为粒度，分别拉起Process
// (不强制要求 Run, Process完成上述工作,具体根据子类的实际情况进行实现,比如不感知PROF文件的情况)
class DataProcessor
{
   public:
    DataProcessor() = default;
    explicit DataProcessor(const std::string& profPath);
    bool Run(DataInventory&, const std::string& processorName);
    virtual ~DataProcessor() = default;

   protected:
    static uint8_t CheckPathAndTable(const std::string& path, const DBInfo& dbInfo, bool enableStrictCheck = true);
    static uint16_t GetEnumTypeValue(const std::string& key, const std::string& tableName,
                                     const std::unordered_map<std::string, uint16_t>& enumTable);
    template <typename Tp>
    void FilterDataByStartTime(std::vector<Tp>& data, uint64_t startTimeNs, const std::string& processorName,
                               std::function<bool(const Tp&, uint64_t)> condition = nullptr);

    template <typename Tp>
    bool SaveToDataInventory(std::vector<Tp>&& data, DataInventory& dataInventory, const std::string& processorName);

   protected:
    std::string profPath_;

   private:
    virtual bool Process(DataInventory& dataInventory) = 0;
};

template <typename Tp>
void DataProcessor::FilterDataByStartTime(std::vector<Tp>& datas, uint64_t startTimeNs,
                                          const std::string& processorName,
                                          std::function<bool(const Tp&, uint64_t)> condition)
{
    INFO("There are % records before % data filtering, filterTime is %.", datas.size(), processorName, startTimeNs);
    datas.erase(
        std::remove_if(datas.begin(), datas.end(), [startTimeNs, condition](const Tp& data)
                       { return condition == nullptr ? data.timestamp < startTimeNs : condition(data, startTimeNs); }),
        datas.end());
    INFO("There are % records after % data filtering.", datas.size(), processorName);
}

template <typename Tp>
bool DataProcessor::SaveToDataInventory(std::vector<Tp>&& data, DataInventory& dataInventory,
                                        const std::string& processorName)
{
    INFO("Save % data into dataInventory", processorName);
    if (data.empty())
    {
        WARN("% no data to inject to dataInventory", processorName);
        return true;
    }
    std::shared_ptr<std::vector<Tp>> oneSharedData;
    MAKE_SHARED_RETURN_VALUE(oneSharedData, std::vector<Tp>, false, std::move(data));
    dataInventory.Inject(oneSharedData);
    return true;
}
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_DATA_PROCESSOR_H
