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

#ifndef MSPROF_METRIC_PROCESSOR_H
#define MSPROF_METRIC_PROCESSOR_H

#include <map>
#include <unordered_map>
#include <utility>

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis
{
namespace Domain
{

class MetricProcessor : public DataProcessor
{
   public:
    MetricProcessor() = default;
    explicit MetricProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    bool TaskBasedProcess(DataInventory &dataInventory);
    std::vector<std::string> GetAndCheckTableColumns(const std::unordered_map<std::string, uint16_t> &dbPathTable,
                                                     DBInfo &metricDB);

    bool TaskBasedProcessByDevice(const std::pair<const std::string, uint16_t> &dbRecord,
                                  const std::vector<std::string> &headers, DBInfo &metricDB,
                                  std::map<TaskId, std::vector<std::vector<std::string>>> &dataInventory);

    // baseTimeNs 是 startClockMonotonicRaw。end_time 与它同为开机 monotonic ns，WHERE 直接比较。
    // 返回 false 表示一列数据都没取到(连接失败或所有列查询都失败)；rowCount 是取到的行数，
    // 为 0 表示查询成功但行都被 end_time 条件过滤掉了。
    bool GetTaskBasedData(const std::string &dbPath, const std::vector<std::string> &headers, DBInfo &metricDB,
                          uint64_t baseTimeNs, std::vector<std::vector<std::string>> &oriData, size_t &rowCount);

    bool FormatTaskBasedData(const std::vector<std::vector<std::string>> &oriData,
                             std::map<TaskId, std::vector<std::vector<std::string>>> &processedData,
                             const uint16_t &deviceId);

    std::vector<TableColumn> RemoveNeedlessColumns(std::vector<TableColumn> &tableColumns);

    bool CheckAndGetCubeUsageRelatedDataIndex(const std::vector<std::string> &headers,
                                              std::vector<uint32_t> &neededDataIndex);
    std::tuple<double, uint16_t> GetCubeUsageByDeviceId(uint16_t deviceId);
    bool AddCubeUsageWithoutDur(std::map<TaskId, std::vector<std::vector<std::string>>> &processedData,
                                const std::vector<std::string> &headers);

    bool AddMemoryBound(std::map<TaskId, std::vector<std::vector<std::string>>> &processedData,
                        std::vector<std::string> &headers);

    bool CheckAndGetMemoryBoundRelatedDataIndex(const std::vector<std::string> &headers,
                                                std::vector<uint32_t> &neededDataIndex);

    bool ProcessData(const std::unordered_map<std::string, uint16_t> &dbPathAndDeviceID, DBInfo &metricDB,
                     std::vector<std::string> &headers, DataInventory &dataInventory);

    std::vector<std::string> ModifySummaryHeaders(const std::vector<std::string> &oriHeaders);

    void InsertLineDataToOriData(std::vector<std::vector<std::string>> &oriData,
                                 std::vector<std::tuple<std::string>> &lineData);

   private:
    std::unordered_map<uint16_t, std::tuple<double, uint16_t>> cubeUsageInfo_;
};

}  // namespace Domain
}  // namespace Analysis
#endif  // MSPROF_METRIC_PROCESSOR_H
