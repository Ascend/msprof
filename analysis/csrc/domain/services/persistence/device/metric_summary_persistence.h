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

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_METRIC_SUMMARY_PERSISTENCE_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_METRIC_SUMMARY_PERSISTENCE_H

#include "analysis/csrc/domain/services/association/calculator/include/metric_calculator_factory.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"

namespace Analysis
{
namespace Domain
{
enum PmuHeaderType
{
    TASK_ID = 0,
    AIC_TOTAL_CYCLE,
    AIC_TOTAL_TIME,
    AIC_PMU_RESULT,
    AIV_TOTAL_CYCLE,
    AIV_TOTAL_TIME,
    AIV_PMU_RESULT
};
using namespace Infra;
using namespace Analysis::Application;
class MetricSummaryPersistence : public Process
{
   private:
    ~MetricSummaryPersistence() override;
    uint32_t ProcessEntry(DataInventory& dataInventory, const Context& context) override;
    TableColumns GetTableColumn(const DeviceContext& context);
    uint32_t GenerateAndSavePmuData(std::map<TaskId, std::vector<DeviceTask>>& deviceTask, std::string& dbPath);
    uint32_t SaveDataToDb(std::map<TaskId, std::vector<DeviceTask>>& deviceTask, std::string& dbPath, DBInfo& dbInfo);
    bool BindAndExecuteInsert(std::unordered_map<PmuHeaderType, std::vector<uint64_t>>& ids,
                              std::unordered_map<PmuHeaderType, std::vector<double>>& pmu);
    bool ConstructData(std::unordered_map<PmuHeaderType, std::vector<uint64_t>>& ids,
                       std::unordered_map<PmuHeaderType, std::vector<double>>& result, DeviceTask& task, int aicLength,
                       int aivLength);
    void CreateConnection(const std::string& dbPath);

   private:
    std::unique_ptr<MetricCalculator> aicCalculator_;
    std::unique_ptr<MetricCalculator> aivCalculator_;
    int aicLength_ = 0;
    int aivLength_ = 0;
    sqlite3* db_ = nullptr;
    sqlite3_stmt* stmt_ = nullptr;
    bool dynamicFlag = false;
};

class MetricSummaryDB : public Database
{
   public:
    MetricSummaryDB(TableColumns columns);
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_METRIC_SUMMARY_PERSISTENCE_H
