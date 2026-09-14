/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/data_process/system/host_platform_processor.h"

#include <memory>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/entities/viewer_data/system/include/host_platform_data.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const std::string HOST_DIR = "host";
const std::string SQLITE_DIR = "sqlite";
using ThreadRow = std::tuple<int64_t, int64_t, std::string, int64_t, int64_t, int64_t, int64_t>;
using ProcessRow = std::tuple<int64_t, int64_t, std::string, int64_t, int64_t>;
using MetricDescRow = std::tuple<int64_t, std::string, std::string, std::string>;
using MetricRow = std::tuple<int64_t, int64_t, double, int64_t, int64_t, int64_t>;
using NumaLevelsHierarchyRow = std::tuple<uint64_t, uint64_t, uint64_t, uint64_t>;
using NumaMetricsRow = std::tuple<uint64_t, uint64_t, double, uint64_t>;
using NumaScalingValuesRow = std::tuple<uint64_t, uint64_t, double>;
using NumaTitlesNamesRow = std::tuple<uint64_t, std::string, std::string, uint64_t, std::string, uint64_t>;

void ConvertThreadRow(const ThreadRow& row, HostCoreThreadData& item)
{
    std::tie(item.id, item.tid, item.name, item.processId, item.parentId, item.startTs, item.endTs) = row;
}

void ConvertProcessRow(const ProcessRow& row, HostCoreProcessData& item)
{
    std::tie(item.id, item.pid, item.name, item.startTs, item.endTs) = row;
}

void ConvertMetricDescRow(const MetricDescRow& row, HostCoreMetricDescData& item)
{
    std::tie(item.id, item.name, item.description, item.measurementUnit) = row;
}

void ConvertMetricRow(const MetricRow& row, HostCoreMetricData& item)
{
    std::tie(item.id, item.timestamp, item.value, item.descId, item.tidId, item.cpuId) = row;
}

void ConvertNumaLevelsHierarchyRow(const NumaLevelsHierarchyRow& row, NumaLevelsHierarchyData& item)
{
    std::tie(item.id, item.title0_id, item.title1_id, item.title2_id) = row;
}

void ConvertNumaMetricsRow(const NumaMetricsRow& row, NumaMetricsData& item)
{
    std::tie(item.id, item.ts, item.value, item.levels_id) = row;
}

void ConvertNumaScalingValuesRow(const NumaScalingValuesRow& row, NumaScalingValuesData& item)
{
    std::tie(item.id, item.level_id, item.max_value) = row;
}

void ConvertNumaTitlesNamesRow(const NumaTitlesNamesRow& row, NumaTitlesNamesData& item)
{
    std::tie(item.id, item.name, item.description, item.summary_flag, item.measurement_unit, item.unique_id) = row;
}

template <typename DataT, typename TupleT>
bool ReadHostPlatformTable(Infra::DBRunner& dbRunner, const std::string& tableName, const std::string& sql,
                           std::vector<DataT>& data, void (*convert)(const TupleT&, DataT&))
{
    if (!dbRunner.CheckTableExists(tableName))
    {
        return true;
    }
    std::vector<TupleT> rows;
    if (!dbRunner.QueryData(sql, rows))
    {
        ERROR("Read host platform table % failed.", tableName);
        return false;
    }
    if (!Utils::Reserve(data, data.size() + rows.size()))
    {
        ERROR("Reserve host platform table % data failed.", tableName);
        return false;
    }
    for (const auto& row : rows)
    {
        DataT item;
        convert(row, item);
        data.push_back(std::move(item));
    }
    return true;
}

bool ReadHostNumaData(Infra::DBRunner& dbRunner, std::vector<NumaLevelsHierarchyData>& hierarchies,
                      std::vector<NumaMetricsData>& metrics, std::vector<NumaScalingValuesData>& scalingValues,
                      std::vector<NumaTitlesNamesData>& titlesNames)
{
    return ReadHostPlatformTable(dbRunner, "p_levels_hierarchy_names",
                                 "SELECT id, title0_id, title1_id, title2_id FROM p_levels_hierarchy_names",
                                 hierarchies, ConvertNumaLevelsHierarchyRow) &&
           ReadHostPlatformTable(dbRunner, "p_metrics", "SELECT id, ts, value, levels_id FROM p_metrics", metrics,
                                 ConvertNumaMetricsRow) &&
           ReadHostPlatformTable(dbRunner, "p_scaling_values", "SELECT id, level_id, max_value FROM p_scaling_values",
                                 scalingValues, ConvertNumaScalingValuesRow) &&
           ReadHostPlatformTable(dbRunner, "p_titles_names",
                                 "SELECT id, name, description, summary_flag, measurement_unit, unique_id "
                                 "FROM p_titles_names",
                                 titlesNames, ConvertNumaTitlesNamesRow);
}

bool ReadHostCoreData(Infra::DBRunner& dbRunner, std::vector<HostCoreThreadData>& threads,
                      std::vector<HostCoreProcessData>& processes, std::vector<HostCoreMetricDescData>& metricDescs,
                      std::vector<HostCoreMetricData>& metrics)
{
    return ReadHostPlatformTable(dbRunner, "p_thread",
                                 "SELECT id, tid, name, process_id, parent_id, start_ts, end_ts FROM p_thread", threads,
                                 ConvertThreadRow) &&
           ReadHostPlatformTable(dbRunner, "p_process", "SELECT id, pid, name, start_ts, end_ts FROM p_process",
                                 processes, ConvertProcessRow) &&
           ReadHostPlatformTable(dbRunner, "p_core_metric_desc",
                                 "SELECT id, name, description, measurement_unit FROM p_core_metric_desc", metricDescs,
                                 ConvertMetricDescRow) &&
           ReadHostPlatformTable(dbRunner, "p_core_metric",
                                 "SELECT id, ts, value, desc_id, tid_id, cpu_id FROM p_core_metric", metrics,
                                 ConvertMetricRow);
}
}  // namespace

bool HostPlatformProcessor::Process(Infra::DataInventory& dataInventory)
{
    const Infra::HostNuma hostNuma;
    const Infra::HostCore hostCore;
    std::vector<NumaLevelsHierarchyData> hierarchies;
    std::vector<NumaMetricsData> numaMetrics;
    std::vector<NumaScalingValuesData> scalingValues;
    std::vector<NumaTitlesNamesData> titlesNames;
    std::vector<HostCoreThreadData> threads;
    std::vector<HostCoreProcessData> processes;
    std::vector<HostCoreMetricDescData> metricDescs;
    std::vector<HostCoreMetricData> metrics;
    const std::string numaDbPath = Utils::File::PathJoin({profPath_, HOST_DIR, SQLITE_DIR, hostNuma.GetDBName()});
    if (Utils::File::Exist(numaDbPath) && Utils::File::Check(numaDbPath))
    {
        Infra::DBRunner dbRunner(numaDbPath);
        if (!ReadHostNumaData(dbRunner, hierarchies, numaMetrics, scalingValues, titlesNames))
        {
            ERROR("Read host NUMA database failed, path is %.", numaDbPath);
            return false;
        }
    }

    const std::string coreDbPath = Utils::File::PathJoin({profPath_, HOST_DIR, SQLITE_DIR, hostCore.GetDBName()});
    if (Utils::File::Exist(coreDbPath) && Utils::File::Check(coreDbPath))
    {
        Infra::DBRunner dbRunner(coreDbPath);
        if (!ReadHostCoreData(dbRunner, threads, processes, metricDescs, metrics))
        {
            ERROR("Read host core database failed, path is %.", coreDbPath);
            return false;
        }
    }
    return SaveToDataInventory(std::move(hierarchies), dataInventory, PROCESSOR_NAME_HOST_PLATFORM) &&
           SaveToDataInventory(std::move(numaMetrics), dataInventory, PROCESSOR_NAME_HOST_PLATFORM) &&
           SaveToDataInventory(std::move(scalingValues), dataInventory, PROCESSOR_NAME_HOST_PLATFORM) &&
           SaveToDataInventory(std::move(titlesNames), dataInventory, PROCESSOR_NAME_HOST_PLATFORM) &&
           SaveToDataInventory(std::move(threads), dataInventory, PROCESSOR_NAME_HOST_PLATFORM) &&
           SaveToDataInventory(std::move(processes), dataInventory, PROCESSOR_NAME_HOST_PLATFORM) &&
           SaveToDataInventory(std::move(metricDescs), dataInventory, PROCESSOR_NAME_HOST_PLATFORM) &&
           SaveToDataInventory(std::move(metrics), dataInventory, PROCESSOR_NAME_HOST_PLATFORM);
}
}  // namespace Domain
}  // namespace Analysis
