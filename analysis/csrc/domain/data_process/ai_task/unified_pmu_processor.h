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
#ifndef ANALYSIS_DOMAIN_UNIFIED_PMU_PROCESSOR_H
#define ANALYSIS_DOMAIN_UNIFIED_PMU_PROCESSOR_H

#include <tuple>

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/unified_pmu_data.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis
{
namespace Domain
{

class UnifiedPmuProcessor : public DataProcessor
{
    // Original Sample Timeline Format:aicore/ai_vector_core中的AICoreOriginalData
    // timestamp, task_cyc, coreid
    using OSTFormat = std::vector<std::tuple<uint64_t, std::string, uint32_t>>;
    // Original Sample Summary Format:aicore/ai_vector_core中的MetricSummary
    // metric, value, coreid
    using OSSFormat = std::vector<std::tuple<std::string, double, uint32_t>>;
    // Original Task Format: 只取id + 对应字段的value + 该行的算子时间列end_time
    // stream_id, task_id, subtask_id, batch_id, value, end_time(host开机monotonic ns，Format时再GetLocalTime)
    using OTFormat = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, double, uint64_t>>;

   public:
    UnifiedPmuProcessor() = default;
    explicit UnifiedPmuProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    bool TaskBasedProcess(DataInventory &data_inventory);
    bool SampleBasedProcess(DataInventory &data_inventory);
    static std::vector<std::string> GetAndCheckTableColumns(
        const std::unordered_map<std::string, uint16_t> &dbPathAndDeviceID, Analysis::Infra::DBInfo &metricDB);
    bool TaskBasedProcess(const std::string &fileDir);
    bool ProcessTaskBasedData(const std::unordered_map<std::string, uint16_t> &dbPathAndDeviceId,
                              Analysis::Infra::DBInfo &metricDB, std::vector<std::string> &headers,
                              DataInventory &dataInventory);
    bool ProcessTaskBasedDataByHeader(const std::pair<std::string, uint16_t> &dbPathAndDeviceId,
                                      Analysis::Infra::DBInfo &metricDB, const Utils::ProfTimeRecord &record,
                                      const std::string &header, std::vector<UnifiedTaskPmu> &processedData);
    UnifiedPmuProcessor::OTFormat GetTaskBasedData(const std::string &dbPath, const std::string &columnName,
                                                   DBInfo &metricDB);
    static uint64_t UpdateColumnName(std::string &columnName);
    bool FormatTaskBasedData(const OTFormat &oriData, std::vector<UnifiedTaskPmu> &processedData,
                             std::string columnName, const uint16_t &deviceId, const Utils::ProfTimeRecord &record);
    bool SampleBasedProcess(const std::string &fileDir);
    bool SampleBasedTimelineProcess(const std::unordered_map<std::string, std::tuple<uint16_t, uint64_t>> &dbPathTable,
                                    DataInventory &dataInventory);
    UnifiedPmuProcessor::OSTFormat GetSampleBasedTimelineData(const std::string &dbPath);
    bool FormatSampleBasedTimelineData(const OSTFormat &oriData, std::vector<UnifiedSampleTimelinePmu> &processedData,
                                       const Utils::LocaltimeContext &localtimeContext, const double freq,
                                       const uint64_t coreType);
    bool SampleBasedSummaryProcess(const std::unordered_map<std::string, std::tuple<uint16_t, uint64_t>> &dbPathTable,
                                   DataInventory &dataInventory);
    UnifiedPmuProcessor::OSSFormat GetSampleBasedSummaryData(const std::string &dbPath);
    bool FormatSampleBasedSummaryData(const OSSFormat &oriData, std::vector<UnifiedSampleSummaryPmu> &processedData,
                                      const uint16_t deviceId, const uint64_t coreType);
};

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_UNIFIED_PMU_PROCESSOR_H
