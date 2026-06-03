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

#include "analysis/csrc/domain/data_process/system/aicore_freq_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

AicoreFreqProcessor::AicoreFreqProcessor(const std::string& profPath) : DataProcessor(profPath) {}

bool AicoreFreqProcessor::Process(DataInventory& dataInventory)
{
    auto version = Context::GetInstance().GetPlatformVersion(DEFAULT_DEVICE_ID, profPath_);
    if (version != static_cast<int>(Chip::CHIP_V4_1_0) && version != static_cast<int>(Chip::CHIP_V1_1_1))
    {
        // 当前仅支持910b 和 310b处理低功耗变频数据
        WARN("This platformVersion: % does not support the processing of aicore freq data.", version);
        return true;
    }
    bool flag = true;
    std::vector<LowPowerData> res;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto& devicePath : deviceList)
    {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        if (deviceId == INVALID_DEVICE_ID)
        {
            ERROR("The invalid deviceId cannot to be identified.");
            flag = false;
            continue;
        }
        ProfTimeRecord timeRecord;
        if (!Context::GetInstance().GetProfTimeRecordInfo(timeRecord, profPath_, deviceId))
        {
            ERROR("Failed to GetProfTimeRecordInfo, fileDir is %, device id is %.", profPath_, deviceId);
            flag = false;
            continue;
        }
        Utils::SyscntConversionParams params;
        if (!Context::GetInstance().GetSyscntConversionParams(params, deviceId, profPath_))
        {
            ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
            flag = false;
            continue;
        }
        AicoreFreqProcessor::OriFreqDataFormat freqData;
        if (!ProcessData(devicePath, freqData))
        {
            ERROR("Get original data failed.");
            flag = false;
            continue;
        }
        std::vector<LowPowerData> processedData;
        if (!FormatData(deviceId, timeRecord, params, freqData, processedData))
        {
            ERROR("FormatData failed, fileDir is %.", devicePath);
            flag = false;
            continue;
        }
        res.insert(res.end(), processedData.begin(), processedData.end());
    }
    if (!SaveToDataInventory<LowPowerData>(std::move(res), dataInventory, PROCESSOR_NAME_AICORE_FREQ))
    {
        flag = false;
        ERROR("Save data failed, %.", PROCESSOR_NAME_AICORE_FREQ);
    }
    return flag;
}

bool AicoreFreqProcessor::ProcessData(const std::string& devicePath, AicoreFreqProcessor::OriFreqDataFormat& oriData)
{
    DBInfo dbInfo("freq.db", "FreqParse");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, dbInfo.dbName});
    if (!dbInfo.ConstructDBRunner(dbPath))
    {
        return false;
    }
    auto status = CheckPathAndTable(dbPath, dbInfo);
    if (status == CHECK_FAILED)
    {
        return false;
    }
    else if (status == CHECK_SUCCESS)
    {
        oriData = LoadData(dbPath, dbInfo);
    }
    if (oriData.empty())
    {
        WARN("Original data is empty. DBPath is %", dbPath);
    }
    return true;
}

AicoreFreqProcessor::OriFreqDataFormat AicoreFreqProcessor::LoadData(const std::string& dbPath, DBInfo& freqDB)
{
    INFO("AicoreFreqProcessor GetData, dir is %", dbPath);
    AicoreFreqProcessor::OriFreqDataFormat oriData;
    if (freqDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql = "SELECT syscnt, freq FROM " + freqDB.tableName;
    if (!freqDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query freq data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

bool AicoreFreqProcessor::FormatData(const uint16_t& deviceId, const ProfTimeRecord& timeRecord,
                                     const Utils::SyscntConversionParams& params,
                                     const AicoreFreqProcessor::OriFreqDataFormat& freqData,
                                     std::vector<LowPowerData>& processedData)
{
    INFO("AicoreFreqProcessor FormatData.");
    double freq = 0.0;
    if (!Context::GetInstance().GetPmuFreq(freq, deviceId, profPath_))
    {
        ERROR("Get default freq data failed.");
        return false;
    }
    if (freqData.empty())
    {
        WARN("freq original data is empty, it will use default freq.");
    }
    // 2是开始和结束的两条记录
    if (!Reserve(processedData, freqData.size() + 2))
    {
        ERROR("Reserve for freq data failed.");
        return false;
    }
    LowPowerData lowPowerData;
    // 将频率置为默认频率，避免最后插入结束记录时 频率为0
    lowPowerData.freq = freq;
    lowPowerData.deviceId = deviceId;
    for (auto& row : freqData)
    {
        uint64_t devSyscnt;
        std::tie(devSyscnt, lowPowerData.freq) = row;
        HPFloat hostTimestamp{GetTimeFromSyscnt(devSyscnt, params)};
        lowPowerData.timestamp = GetLocalTime(hostTimestamp, timeRecord).Uint64();
        if (lowPowerData.timestamp < timeRecord.startTimeNs)
        {
            freq = lowPowerData.freq;
            continue;
        }
        processedData.push_back(lowPowerData);
    }
    if (timeRecord.endTimeNs != timeRecord.startTimeNs + DEFAULT_DURATION_TIME_NS)
    {
        lowPowerData.timestamp = timeRecord.endTimeNs;
        // 结束时间的freq为最后一条记录的freq
        processedData.push_back(lowPowerData);
    }
    // 如果开始时间前无记录，则使用默认freq；如果有记录，则使用开始时间前的freq(此时添加
    // db里的数据将为乱序，但不影响数据读取)
    lowPowerData.timestamp = timeRecord.startTimeNs;
    lowPowerData.freq = freq;
    processedData.push_back(lowPowerData);
    std::sort(processedData.begin(), processedData.end(),
              [](const LowPowerData& ld, const LowPowerData& rd) { return ld.timestamp < rd.timestamp; });
    return true;
}
}  // namespace Domain
}  // namespace Analysis
