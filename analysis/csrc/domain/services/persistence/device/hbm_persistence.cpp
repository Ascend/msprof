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

#include "analysis/csrc/domain/services/persistence/device/hbm_persistence.h"

#include <algorithm>
#include <climits>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_hbm.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/hbm/include/hbm_parser.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
// 与 Python msmodel/hardware/hbm_model.py 的 HbmModel.SCALE 一致：
// HBMC(256) / HBM_EVENT(8) / KILOBYTE(1024.0) / KILOBYTE(1024.0)。
constexpr double HBM_SCALE = 0.000030517578125;
const std::string HBM_ORIGINAL_TABLE = "HBMOriginalData";
const std::string HBM_BW_TABLE = "HBMbwData";

using HbmOriginalRow = std::tuple<uint32_t, uint32_t, double, uint64_t, std::string, uint32_t>;
using HbmQueryRow = std::tuple<uint32_t, double, uint64_t, uint32_t, std::string>;
using HbmBwRow = std::tuple<uint32_t, double, double, uint32_t, std::string>;

struct HbmSampleData
{
    uint32_t deviceId;
    double timestamp;
    uint64_t count;
    uint32_t hbmId;
    std::string eventType;
};

struct HbmBandwidthData
{
    uint32_t deviceId;
    double timestamp;
    double bandwidth;
    uint32_t hbmId;
    std::string eventType;
};

size_t GetTimestampGroupEnd(const std::vector<HbmSampleData> &data, size_t begin)
{
    size_t end = begin + 1;
    // The query orders by timestamp; grouping must use exact SQLite timestamp equality.
    while (end < data.size() && data[end].timestamp == data[begin].timestamp)
    {
        ++end;
    }
    return end;
}

// 复现 Python HbmModel._filter_abnormal_timestamps，按采样组条数选择带宽计算输入。
// 不判断时间戳数值是否异常，也不删除 HBMOriginalData 中的原始记录。
size_t FilterAbnormalTimestamps(std::vector<HbmSampleData> &data, uint32_t deviceId)
{
    std::map<size_t, size_t> countFrequency;
    for (size_t begin = 0; begin < data.size();)
    {
        const size_t end = GetTimestampGroupEnd(data, begin);
        ++countFrequency[end - begin];
        begin = end;
    }

    size_t checkLength = 0;
    size_t maxFrequency = 0;
    for (const auto &item : countFrequency)
    {
        // std::map visits counts in ascending order: ties select the larger window, as in Python.
        if (item.second >= maxFrequency)
        {
            checkLength = item.first;
            maxFrequency = item.second;
        }
    }

    size_t retained = 0;
    for (size_t begin = 0; begin < data.size();)
    {
        const size_t end = GetTimestampGroupEnd(data, begin);
        if (end - begin == checkLength)
        {
            for (size_t index = begin; index < end; ++index, ++retained)
            {
                if (retained != index)
                {
                    data[retained] = std::move(data[index]);
                }
            }
        }
        begin = end;
    }
    if (retained != data.size())
    {
        WARN("HBM data loss: device_id=% total=% check_len=% removed=%.", deviceId, data.size(), checkLength,
             data.size() - retained);
        data.resize(retained);
    }
    return checkLength;
}

std::vector<HbmBandwidthData> CalculateBandwidth(const std::vector<HbmSampleData> &data, size_t checkLength)
{
    std::vector<HbmBandwidthData> result;
    result.reserve(data.size());
    for (size_t index = 0; index < data.size(); ++index)
    {
        const size_t comparedIndex = index >= checkLength ? index - checkLength : data.size() + index - checkLength;
        const auto &sample = data[index];
        const double duration = sample.timestamp - data[comparedIndex].timestamp;
        double bandwidth = static_cast<double>(sample.count);
        if (!Utils::IsDoubleEqual(duration, 0.0))
        {
            bandwidth = std::max(bandwidth * HBM_SCALE / duration * NANO_SECOND, 0.0);
        }
        result.push_back({sample.deviceId, sample.timestamp, bandwidth, sample.hbmId, sample.eventType});
    }
    return result;
}

bool InitHbmDatabase(const DeviceContext &deviceContext, DBInfo &dbInfo, std::string &dbPath)
{
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, Infra::HBMDB, false);
    dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, Infra::DBRunner, false, dbPath);
    // Python 会先创建带宽表，即使后续没有计算结果；公共 SaveData 不接受空数据。
    return dbInfo.dbRunner->CreateTable(HBM_BW_TABLE, dbInfo.database->GetTableCols(HBM_BW_TABLE));
}

bool GetHbmTimeContext(const DeviceContext &deviceContext, Utils::LocaltimeContext &timeContext)
{
    DeviceStartLog deviceStartLog;
    deviceContext.Getter(deviceStartLog);
    DeviceStartInfo startInfo;
    deviceContext.Getter(startInfo);
    if (startInfo.collectionTimeBegin > UINT64_MAX / NS_TO_US)
    {
        ERROR("HBM collection start time overflows nanoseconds.");
        return false;
    }
    timeContext.hostMonotonic = deviceContext.GetSyscntConversionParams().hostMonotonic;
    timeContext.deviceMonotonic = deviceStartLog.clockMonotonicRaw;
    timeContext.timeRecord.startTimeNs = startInfo.collectionTimeBegin * NS_TO_US;
    timeContext.timeRecord.baseTimeNs = startInfo.clockMonotonicRaw;
    if (timeContext.timeRecord.startTimeNs < timeContext.timeRecord.baseTimeNs)
    {
        ERROR("HBM collection start time is earlier than the monotonic base.");
        return false;
    }
    return true;
}

bool GenerateOriginalData(const std::vector<HalHbmData> &data, const DeviceContext &deviceContext, uint32_t deviceId,
                          std::vector<HbmSampleData> &result)
{
    const bool isVersionTwo = deviceContext.GetCollectionVersion() == "2.0";
    Utils::LocaltimeContext timeContext;
    if (!isVersionTwo && !GetHbmTimeContext(deviceContext, timeContext))
    {
        return false;
    }
    const Utils::HPFloat zeroTimestamp(0);
    const Utils::HPFloat maxTimestamp(UINT64_MAX);
    result.reserve(data.size());
    for (const auto &item : data)
    {
        if (item.count > INT64_MAX)
        {
            ERROR("HBM count exceeds the SQLite signed integer range: %.", item.count);
            return false;
        }
        uint64_t timestamp = 0;
        if (isVersionTwo)
        {
            if (item.timestamp > INT64_MAX)
            {
                ERROR("HBM timestamp exceeds the SQLite signed integer range: %.", item.timestamp);
                return false;
            }
            timestamp = item.timestamp;
        }
        else
        {
            if (item.timestamp > (UINT64_MAX - timeContext.deviceMonotonic) / NS_TO_US)
            {
                ERROR("HBM sampling timestamp overflows nanoseconds: %.", item.timestamp);
                return false;
            }
            const uint64_t samplingTimeNs = item.timestamp * NS_TO_US + timeContext.deviceMonotonic;
            auto hostTime = Utils::GetTimeBySamplingTimestamp(static_cast<double>(samplingTimeNs),
                                                              timeContext.hostMonotonic, timeContext.deviceMonotonic);
            auto absoluteTime = Utils::GetLocalTime(hostTime, timeContext.timeRecord);
            if (absoluteTime < zeroTimestamp || absoluteTime > maxTimestamp)
            {
                ERROR("HBM absolute timestamp exceeds the unsigned integer range.");
                return false;
            }
            timestamp = absoluteTime.Uint64();
        }
        const std::string eventType = item.eventId == 0 ? "read" : "write";
        result.push_back({deviceId, static_cast<double>(timestamp), item.count, item.hbmId, eventType});
    }
    return true;
}

bool SaveOriginalData(const std::vector<HbmSampleData> &data, DBInfo &dbInfo, std::string &dbPath)
{
    std::vector<HbmOriginalRow> rows;
    rows.reserve(data.size());
    for (const auto &sample : data)
    {
        rows.emplace_back(sample.deviceId, 0, sample.timestamp, sample.count, sample.eventType, sample.hbmId);
    }
    return SaveData(rows, dbInfo, dbPath);
}

bool LoadBandwidthSamples(DBInfo &dbInfo, uint32_t deviceId, std::vector<HbmSampleData> &result)
{
    std::vector<HbmQueryRow> rows;
    const std::string sql =
        "select device_id,timestamp,counts,hbmId,event_type from HBMOriginalData where "
        "device_id=" +
        std::to_string(deviceId) + " group by timestamp,event_type,hbmId,counts order by timestamp";
    if (!dbInfo.dbRunner->QueryData(sql, rows))
    {
        return false;
    }
    result.reserve(rows.size());
    for (const auto &row : rows)
    {
        result.push_back({std::get<0>(row), std::get<1>(row), std::get<2>(row), std::get<3>(row), std::get<4>(row)});
    }
    return true;
}

bool SaveBandwidthData(const std::vector<HbmBandwidthData> &data, DBInfo &dbInfo, std::string &dbPath)
{
    std::vector<HbmBwRow> rows;
    rows.reserve(data.size());
    for (const auto &sample : data)
    {
        rows.emplace_back(sample.deviceId, sample.timestamp, sample.bandwidth, sample.hbmId, sample.eventType);
    }
    dbInfo.tableName = HBM_BW_TABLE;
    return SaveData(rows, dbInfo, dbPath);
}

bool ProcessBandwidthData(const DeviceContext &deviceContext, uint32_t deviceId, DBInfo &dbInfo, std::string &dbPath)
{
    SampleInfo sampleInfo;
    deviceContext.Getter(sampleInfo);
    const size_t eventCount = static_cast<size_t>(std::count(sampleInfo.hbmProfilingEvents.begin(),
                                                             sampleInfo.hbmProfilingEvents.end(), ',')) +
                              1;
    if (eventCount != 1 && eventCount != 2)
    {
        ERROR("Invalid HBM profiling events: %.", sampleInfo.hbmProfilingEvents);
        return false;
    }

    std::vector<HbmSampleData> samples;
    if (!LoadBandwidthSamples(dbInfo, deviceId, samples))
    {
        return false;
    }
    const size_t checkLength = FilterAbnormalTimestamps(samples, deviceId);
    if (checkLength == 0 || samples.empty())
    {
        return true;
    }
    return SaveBandwidthData(CalculateBandwidth(samples, checkLength), dbInfo, dbPath);
}
}  // namespace

uint32_t HbmPersistence::ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    const auto &deviceContext = static_cast<const DeviceContext &>(context);
    const auto data = dataInventory.GetPtr<std::vector<HalHbmData>>();
    if (data == nullptr)
    {
        ERROR("HBM raw data is null.");
        return ANALYSIS_ERROR;
    }
    if (data->empty())
    {
        return ANALYSIS_OK;
    }

    DBInfo dbInfo("hbm.db", HBM_ORIGINAL_TABLE);
    std::string dbPath;
    if (!InitHbmDatabase(deviceContext, dbInfo, dbPath))
    {
        ERROR("Create HBM database failed.");
        return ANALYSIS_ERROR;
    }
    DeviceInfo deviceInfo;
    deviceContext.Getter(deviceInfo);
    std::vector<HbmSampleData> originalData;
    if (!GenerateOriginalData(*data, deviceContext, deviceInfo.deviceId, originalData) ||
        !SaveOriginalData(originalData, dbInfo, dbPath))
    {
        ERROR("Persist HBM original data failed.");
        return ANALYSIS_ERROR;
    }
    if (!ProcessBandwidthData(deviceContext, deviceInfo.deviceId, dbInfo, dbPath))
    {
        ERROR("Persist HBM bandwidth data failed.");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

// 暂不启用 HBM C++ 持久化，避免覆盖 Python 解析结果。
// REGISTER_PROCESS_SEQUENCE(HbmPersistence, false, HbmParser);
// REGISTER_PROCESS_DEPENDENT_DATA(HbmPersistence, std::vector<HalHbmData>);
// REGISTER_PROCESS_SUPPORT_CHIP(HbmPersistence, CHIP_V2_1_0, CHIP_V3_1_0, CHIP_V3_2_0, CHIP_V3_3_0,
//                               CHIP_V4_1_0, CHIP_V1_1_1, CHIP_V1_1_2, CHIP_V1_1_3, CHIP_V6_1_0, CHIP_V6_2_0);
}  // namespace Domain
}  // namespace Analysis
