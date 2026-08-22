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

#include "analysis/csrc/domain/data_process/ai_task/hccl_statistic_processor.h"

#include <algorithm>
#include <cmath>

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

const std::string HCCL_OP_REPORT_TABLE = "HcclOpReport";
const std::string KFC_OP_REPORT_TABLE = "KfcOpReport";
const std::string HCCL_STATISTIC_DB_NAME = "hccl_single_device.db";

HcclStatisticProcessor::HcclStatisticProcessor(const std::string& profPaths) : DataProcessor(profPaths) {}
OriHcclDataFormat HcclStatisticProcessor::LoadData(const DBInfo& dbInfo, const std::string& dbPath)
{
    OriHcclDataFormat oriData;
    if (dbInfo.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql{
        "SELECT op_type, occurrences, round(total_time/1000.0, 3), round(min/1000.0, 3), "
        "round(avg/1000.0, 3), round(max/1000.0, 3) FROM " +
        dbInfo.tableName};
    if (!dbInfo.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", dbInfo.tableName);
    }
    return oriData;
}

OriHcclDataFormat HcclStatisticProcessor::LoadReportData(const std::string& devicePath, const std::string& tableName,
                                                         bool& flag)
{
    OriHcclDataFormat oriData;
    DBInfo dbInfo(HCCL_STATISTIC_DB_NAME, tableName);
    std::string dbPath = File::PathJoin({devicePath, SQLITE, dbInfo.dbName});
    if (!dbInfo.ConstructDBRunner(dbPath))
    {
        ERROR("Construct % table failed.", tableName);
        flag = false;
        return oriData;
    }
    auto status = CheckPathAndTable(dbPath, dbInfo, false);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            flag = false;
        }
        return oriData;
    }
    return LoadData(dbInfo, dbPath);
}

std::vector<HcclStatisticData> HcclStatisticProcessor::FormatData(const OriHcclDataFormat& oriData,
                                                                  const uint16_t deviceId)
{
    std::vector<HcclStatisticData> processedData;
    if (!Reserve(processedData, oriData.size()))
    {
        ERROR("Reserve for Hccl Statistic data failed.");
        return processedData;
    }
    double totalTimeSum = 0.0;
    for (const auto& row : oriData)
    {
        totalTimeSum += std::get<2>(row);
    }
    for (const auto& row : oriData)
    {
        HcclStatisticData data;
        data.deviceId = deviceId;
        std::tie(data.opType, data.count, data.totalTime, data.min, data.avg, data.max) = row;
        double ratio = totalTimeSum > 0.0 ? data.totalTime * 100.0 / totalTimeSum : 0.0;
        data.ratio = std::round(ratio * 1000.0) / 1000.0;
        processedData.push_back(data);
    }
    std::sort(processedData.begin(), processedData.end(),
              [](const HcclStatisticData& left, const HcclStatisticData& right)
              { return left.totalTime > right.totalTime; });
    return processedData;
}

bool HcclStatisticProcessor::Process(Analysis::Infra::DataInventory& dataInventory)
{
    if (Context::GetInstance().IsLevel0(profPath_))
    {
        return true;
    }
    bool flag = true;
    std::vector<HcclStatisticData> res;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto& devicePath : deviceList)
    {
        auto deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        if (deviceId == INVALID_DEVICE_ID)
        {
            ERROR("the invalid deviceId cannot to be identified.");
            return false;
        }
        OriHcclDataFormat oriData = LoadReportData(devicePath, HCCL_OP_REPORT_TABLE, flag);
        auto kfcData = LoadReportData(devicePath, KFC_OP_REPORT_TABLE, flag);
        oriData.insert(oriData.end(), kfcData.begin(), kfcData.end());
        if (oriData.empty())
        {
            INFO("Hccl Statistics original data is empty, skip this device. DBPath is %", devicePath);
            continue;
        }
        auto formatData = FormatData(oriData, deviceId);
        if (formatData.empty())
        {
            ERROR("Hccl Statistics data format failed, DBPath is %", devicePath);
            flag = false;
            continue;
        }
        res.insert(res.end(), formatData.begin(), formatData.end());
    }
    if (!SaveToDataInventory<HcclStatisticData>(std::move(res), dataInventory, PROCESSOR_NAME_COMM_STATISTIC))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_COMM_STATISTIC);
        flag = false;
    }
    return flag;
}

}  // namespace Domain
}  // namespace Analysis
