/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#include "analysis/csrc/domain/services/persistence/device/llc_persistence.h"

#include <array>
#include <cmath>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_llc_pcie.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/llc/include/llc_parser.h"
#include "analysis/csrc/domain/services/parser/llc/include/llc_pcie_cpp_enable.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/config.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const std::string LLC_DB_NAME = "llc.db";
const std::string LLC_ORIGINAL_DATA = "LLCOriginalData";
const std::string LLC_EVENTS = "LLCEvents";
const std::string LLC_METRICS = "LLCMetrics";
const std::array<uint32_t, 8> READ_EVENTS{{0x00, 0x01, 0x02, 0x13, 0x20, 0x22, 0x34, 0x36}};
const std::array<uint32_t, 8> WRITE_EVENTS{{0x00, 0x01, 0x03, 0x14, 0x21, 0x23, 0x35, 0x37}};
const long double CACHE_LINE_BYTES = 64.0L;
const long double DECIMAL_SCALE = 1000.0L;

using OriginalRow = std::tuple<uint32_t, double, uint64_t, uint32_t, uint32_t>;
using NullableCount = Infra::NullableValue<uint64_t>;
using EventRow = std::tuple<uint32_t, uint32_t, double, NullableCount, NullableCount, NullableCount, NullableCount,
                            NullableCount, NullableCount, NullableCount, NullableCount>;
using MetricRow = std::tuple<uint32_t, uint32_t, double, double, double>;

struct EventKey
{
    uint32_t deviceId;
    uint32_t l3tid;
    double timestamp;

    bool operator<(const EventKey &other) const
    {
        return std::tie(deviceId, l3tid, timestamp) < std::tie(other.deviceId, other.l3tid, other.timestamp);
    }
};

struct EventValues
{
    std::array<uint64_t, 8> counts{{0, 0, 0, 0, 0, 0, 0, 0}};
    std::array<bool, 8> present{{false, false, false, false, false, false, false, false}};

    void Set(std::size_t index, uint64_t count)
    {
        counts[index] = count;
        present[index] = true;
    }

    bool HasHitRateInputs() const
    {
        return present[2] && present[3] && present[4] && present[5] && present[6] && present[7];
    }

    long double HitRateNumerator() const
    {
        return static_cast<long double>(counts[2]) + counts[3] + counts[5] + counts[7];
    }

    long double HitRateDenominator() const { return static_cast<long double>(counts[4]) + counts[6]; }

    bool HasThroughputInputs() const { return present[0] && present[1]; }

    long double ThroughputCount() const { return static_cast<long double>(counts[0]) + counts[1]; }
};

struct LlcMetricData
{
    uint32_t deviceId;
    uint32_t l3tid;
    double timestamp;
    double hitRate;
    double throughput;
};

using EventMap = std::map<EventKey, EventValues>;

double RoundThreeDecimals(long double value)
{
    return static_cast<double>(std::nearbyint(value * DECIMAL_SCALE) / DECIMAL_SCALE);
}

const std::array<uint32_t, 8> *GetEventMap(const std::string &profiling)
{
    if (profiling == analysis::dvvp::common::config::LLC_PROFILING_READ)
    {
        return &READ_EVENTS;
    }
    if (profiling == analysis::dvvp::common::config::LLC_PROFILING_WRITE)
    {
        return &WRITE_EVENTS;
    }
    ERROR("Invalid llc_profiling option: %", profiling);
    return nullptr;
}

std::vector<OriginalRow> GenerateOriginalRows(const std::vector<HalLlcData> &records)
{
    std::vector<OriginalRow> rows;
    if (!Utils::Reserve(rows, records.size()))
    {
        return {};
    }
    for (const auto &record : records)
    {
        rows.emplace_back(record.deviceId, static_cast<double>(record.timestamp), record.count, record.event,
                          record.l3tid);
    }
    return rows;
}

EventMap GenerateEventRows(const std::vector<HalLlcData> &records, const std::array<uint32_t, 8> &eventCodes)
{
    EventMap rows;
    for (const auto &record : records)
    {
        for (std::size_t index = 0; index < eventCodes.size(); ++index)
        {
            if (record.event != eventCodes[index])
            {
                continue;
            }
            EventKey key{record.deviceId, record.l3tid, static_cast<double>(record.timestamp)};
            rows[key].Set(index, record.count);
            break;
        }
    }
    return rows;
}

NullableCount GetNullableCount(const EventValues &values, std::size_t index)
{
    return values.present[index] ? NullableCount(values.counts[index]) : NullableCount();
}

std::vector<EventRow> GenerateEventDbRows(const EventMap &events)
{
    std::vector<EventRow> rows;
    if (!Utils::Reserve(rows, events.size()))
    {
        return {};
    }
    for (const auto &item : events)
    {
        const auto &key = item.first;
        const auto &values = item.second;
        rows.emplace_back(key.deviceId, key.l3tid, key.timestamp, GetNullableCount(values, 0),
                          GetNullableCount(values, 1), GetNullableCount(values, 2), GetNullableCount(values, 3),
                          GetNullableCount(values, 4), GetNullableCount(values, 5), GetNullableCount(values, 6),
                          GetNullableCount(values, 7));
    }
    return rows;
}

double CalculateHitRate(const EventValues &values)
{
    if (!values.HasHitRateInputs() || values.HitRateDenominator() == 0.0L)
    {
        return 0.0;
    }
    return RoundThreeDecimals(values.HitRateNumerator() / values.HitRateDenominator());
}

double CalculateThroughput(const EventValues &values, double timestamp, double previousTimestamp, bool first)
{
    if (first || !values.HasThroughputInputs())
    {
        return 0.0;
    }
    const double timestampDelta = timestamp - previousTimestamp;
    if (Utils::IsDoubleEqual(timestampDelta, 0.0))
    {
        return 0.0;
    }
    const long double transferredBytes = values.ThroughputCount() / (1.0L / CACHE_LINE_BYTES);
    const long double normalizedTimeInterval =
        static_cast<long double>(timestampDelta) / CACHE_LINE_BYTES / NANO_SECOND;
    const long double throughput = transferredBytes / BYTE_SIZE / BYTE_SIZE / normalizedTimeInterval;
    return RoundThreeDecimals(throughput);
}

std::vector<LlcMetricData> CalculateMetrics(const EventMap &events, uint32_t l3Count)
{
    std::vector<LlcMetricData> metrics;
    for (uint32_t l3tid = 0; l3tid < l3Count; ++l3tid)
    {
        bool first = true;
        uint32_t previousDeviceId = 0;
        double previousTimestamp = 0.0;
        for (const auto &item : events)
        {
            const auto &eventKey = item.first;
            if (eventKey.l3tid != l3tid)
            {
                continue;
            }
            const double hitRate = CalculateHitRate(item.second);
            const bool firstForDevice = first || eventKey.deviceId != previousDeviceId;
            const double throughput =
                CalculateThroughput(item.second, eventKey.timestamp, previousTimestamp, firstForDevice);
            metrics.push_back({eventKey.deviceId, eventKey.l3tid, eventKey.timestamp, hitRate, throughput});
            previousDeviceId = eventKey.deviceId;
            previousTimestamp = eventKey.timestamp;
            first = false;
        }
    }
    return metrics;
}

std::vector<MetricRow> GenerateMetricRows(const std::vector<LlcMetricData> &metrics)
{
    std::vector<MetricRow> rows;
    if (!Utils::Reserve(rows, metrics.size()))
    {
        return {};
    }
    for (const auto &metric : metrics)
    {
        rows.emplace_back(metric.deviceId, metric.l3tid, metric.timestamp, metric.hitRate, metric.throughput);
    }
    return rows;
}

uint32_t GetL3Count(uint32_t chipId)
{
    if (chipId == CHIP_V1_1_1 || chipId == CHIP_V1_1_2 || chipId == CHIP_V1_1_3)
    {
        return 1;
    }
    if (chipId == CHIP_V4_1_0)
    {
        return 2;
    }
    return 4;
}

bool ResetLlcTables(Infra::DBRunner &dbRunner)
{
    const std::array<std::string, 3> tableNames{{LLC_ORIGINAL_DATA, LLC_EVENTS, LLC_METRICS}};
    for (const auto &tableName : tableNames)
    {
        if (dbRunner.CheckTableExists(tableName) && !dbRunner.DropTable(tableName))
        {
            return false;
        }
    }
    return true;
}

bool SaveLlcData(const std::string &dbPath, const std::vector<OriginalRow> &originalRows,
                 const std::vector<EventRow> &eventRows, const std::vector<MetricRow> &metricRows)
{
    Infra::LLCDB database;
    Infra::DBRunner dbRunner(dbPath);
    if (!ResetLlcTables(dbRunner) ||
        !dbRunner.CreateTable(LLC_ORIGINAL_DATA, database.GetTableCols(LLC_ORIGINAL_DATA)) ||
        !dbRunner.CreateTableWithPrimaryKeys(LLC_EVENTS, database.GetTableCols(LLC_EVENTS),
                                             {"device_id", "l3tid", "timestamp"}) ||
        !dbRunner.InsertData(LLC_ORIGINAL_DATA, originalRows) || !dbRunner.InsertData(LLC_EVENTS, eventRows) ||
        !dbRunner.CreateTable(LLC_METRICS, database.GetTableCols(LLC_METRICS)))
    {
        ERROR("Save LLC data failed: %", dbPath);
        return false;
    }
    if (!metricRows.empty() && !dbRunner.InsertData(LLC_METRICS, metricRows))
    {
        ERROR("Save LLC metrics failed: %", dbPath);
        return false;
    }
    return true;
}
}  // namespace

uint32_t LlcPersistence::ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    auto records = dataInventory.GetPtr<std::vector<HalLlcData>>();
    if (records == nullptr)
    {
        ERROR("LLC data is null.");
        return ANALYSIS_ERROR;
    }
    if (records->empty())
    {
        INFO("There is no LLC data, don't need to persistence.");
        return ANALYSIS_OK;
    }

    const auto *deviceContext = dynamic_cast<const DeviceContext *>(&context);
    if (deviceContext == nullptr)
    {
        ERROR("LLC persistence requires DeviceContext.");
        return ANALYSIS_ERROR;
    }
    DeviceInfo deviceInfo;
    SampleInfo sampleInfo;
    deviceContext->Getter(deviceInfo);
    deviceContext->Getter(sampleInfo);
    const std::string dbPath = Utils::File::PathJoin({deviceContext->GetDeviceFilePath(), "sqlite", LLC_DB_NAME});
    const auto originalRows = GenerateOriginalRows(*records);
    if (originalRows.size() != records->size())
    {
        ERROR("Generate LLCOriginalData failed.");
        return ANALYSIS_ERROR;
    }
    const auto *eventCodes = GetEventMap(sampleInfo.llcProfiling);
    if (eventCodes == nullptr)
    {
        return ANALYSIS_ERROR;
    }
    const auto events = GenerateEventRows(*records, *eventCodes);
    const auto eventRows = GenerateEventDbRows(events);
    if (eventRows.size() != events.size())
    {
        ERROR("Generate LLCEvents failed.");
        return ANALYSIS_ERROR;
    }
    const auto metrics = CalculateMetrics(events, GetL3Count(deviceInfo.chipID));
    const auto metricRows = GenerateMetricRows(metrics);
    if (metricRows.size() != metrics.size())
    {
        ERROR("Generate LLCMetrics failed.");
        return ANALYSIS_ERROR;
    }
    if (!SaveLlcData(dbPath, originalRows, eventRows, metricRows))
    {
        ERROR("Persist LLC data failed: %", dbPath);
        return ANALYSIS_ERROR;
    }
    INFO("Process LLC data done: %", dbPath);
    return ANALYSIS_OK;
}

// REGISTER_PROCESS_SEQUENCE(LlcPersistence, true, LlcParser);
// REGISTER_PROCESS_DEPENDENT_DATA(LlcPersistence, std::vector<HalLlcData>);
// REGISTER_PROCESS_SUPPORT_CHIP(LlcPersistence, CHIP_V2_1_0, CHIP_V3_1_0, CHIP_V3_2_0, CHIP_V3_3_0, CHIP_V4_1_0,
//                               CHIP_V1_1_1, CHIP_V1_1_2, CHIP_V1_1_3, CHIP_V6_1_0, CHIP_V6_1_1, CHIP_V6_2_0);
}  // namespace Domain
}  // namespace Analysis
