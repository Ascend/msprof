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
#include "analysis/csrc/domain/data_process/system/qos_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
constexpr double TMP_UINT = NANO_SECOND / (BYTE_SIZE * BYTE_SIZE);

QosProcessor::QosProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool QosProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    std::vector<QosData> allProcessedData;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        flag = ProcessSingleDevice(devicePath, allProcessedData) && flag;
    }
    if (!SaveToDataInventory<QosData>(std::move(allProcessedData), dataInventory, PROCESSOR_NAME_QOS))
    {
        flag = false;
        ERROR("Save qos Data To DataInventory failed, profPath is %.", profPath_);
    }
    return flag;
}

bool QosProcessor::ProcessSingleDevice(const std::string &devicePath, std::vector<QosData> &allProcessedData)
{
    LocaltimeContext localtimeContext;
    localtimeContext.deviceId = GetDeviceIdByDevicePath(devicePath);
    if (localtimeContext.deviceId == INVALID_DEVICE_ID)
    {
        ERROR("the invalid deviceId cannot to be identified, profPath is %.", profPath_);
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(localtimeContext.timeRecord, profPath_,
                                                      localtimeContext.deviceId))
    {
        ERROR(
            "Failed to obtain the time in start_info and end_info, "
            "profPath is %, device id is %.",
            profPath_, localtimeContext.deviceId);
        return false;
    }
    DBInfo qosDB("qos.db", "QosBwData");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, qosDB.dbName});
    if (!qosDB.ConstructDBRunner(dbPath) || qosDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    // 并不是所有场景都有qos数据
    auto status = CheckPathAndTable(dbPath, qosDB);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    OriQosData oriData = LoadData(qosDB);
    if (oriData.empty())
    {
        ERROR("Get % data failed, profPath is %, device is %", qosDB.tableName, profPath_, localtimeContext.deviceId);
        return false;
    }
    auto processedData = FormatData(oriData, localtimeContext);
    if (processedData.empty())
    {
        ERROR("Format qos data error, dbPath is %.", dbPath);
        return false;
    }
    FilterDataByStartTime(processedData, localtimeContext.timeRecord.startTimeNs, PROCESSOR_NAME_QOS);
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());
    return true;
}

OriQosData QosProcessor::LoadData(const DBInfo &qosDB)
{
    // A2/A3场景无dieId 新场景中仅后8个内容有效
    OriQosData oriData;
    std::string sql{"SELECT timestamp, -1, bw1, bw2, bw3, bw4, bw5, bw6, bw7, bw8, bw9, bw10 FROM " + qosDB.tableName};
    auto version = Context::GetInstance().GetPlatformVersion(DEFAULT_DEVICE_ID, profPath_);
    if (Context::GetInstance().IsChipV6(version))
    {
        sql = "SELECT timestamp, die_id, bw3, bw4, bw5, bw6, bw7, bw8, bw9, bw10, 0, 0 FROM " + qosDB.tableName;
    }

    if (!qosDB.dbRunner->QueryData(sql, oriData) || oriData.empty())
    {
        ERROR("Failed to obtain data from the % table.", qosDB.tableName);
    }
    return oriData;
}

std::vector<QosData> QosProcessor::FormatData(const OriQosData &oriData, const LocaltimeContext &localtimeContext)
{
    std::vector<QosData> formatData;
    if (!Reserve(formatData, oriData.size()))
    {
        ERROR("Reserve for qos data failed, profPath is %, deviceId is %.", profPath_, localtimeContext.deviceId);
        return formatData;
    }
    QosData tempData;
    tempData.deviceId = localtimeContext.deviceId;
    double oriTimestamp;
    for (const auto &row : oriData)
    {
        std::tie(oriTimestamp, tempData.dieId, tempData.bw1, tempData.bw2, tempData.bw3, tempData.bw4, tempData.bw5,
                 tempData.bw6, tempData.bw7, tempData.bw8, tempData.bw9, tempData.bw10) = row;
        HPFloat timestamp = oriTimestamp;
        tempData.timestamp = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        formatData.push_back(tempData);
    }
    return formatData;
}
}  // namespace Domain
}  // namespace Analysis
