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

#include "analysis/csrc/domain/services/persistence/host/capture_stream_info_dumper.h"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

namespace Analysis
{
namespace Domain
{
namespace
{
std::string FormatModelIds(const std::set<uint32_t> &modelIds)
{
    std::ostringstream stream;
    stream << '{';
    bool first = true;
    for (auto modelId : modelIds)
    {
        if (!first)
        {
            stream << ',';
        }
        stream << modelId;
        first = false;
    }
    stream << '}';
    return stream.str();
}
}  // namespace

CaptureStreamInfoDumper::CaptureStreamInfoDumper(const std::string &hostPath)
    : BaseDumper<CaptureStreamInfoDumper>(hostPath, "CaptureStreamInfo")
{
    MAKE_SHARED0_NO_OPERATION(database_, StreamInfoDB);
}

CaptureStreamInfoData CaptureStreamInfoDumper::FormatData(const std::vector<std::shared_ptr<ParserCompactInfo>> &input)
{
    auto sortedInput = input;
    std::stable_sort(sortedInput.begin(), sortedInput.end(),
                     [](const std::shared_ptr<ParserCompactInfo> &left, const std::shared_ptr<ParserCompactInfo> &right)
                     {
                         if (!left)
                         {
                             return false;
                         }
                         if (!right)
                         {
                             return true;
                         }
                         return left->timeStamp < right->timeStamp;
                     });

    CaptureStreamInfoData output;
    if (!Utils::Reserve(output, sortedInput.size()))
    {
        ERROR("CaptureStreamInfoDumper: Reserve data failed.");
        return {};
    }
    std::set<uint32_t> startModelSet;
    std::set<uint32_t> endModelSet;
    using RecordKey = std::tuple<uint16_t, uint32_t, uint32_t, uint32_t, uint16_t, uint64_t>;
    std::set<RecordKey> seenRecords;
    std::map<std::pair<uint16_t, uint32_t>, uint32_t> batchIdMap;
    uint64_t repeatedNum = 0;

    for (const auto &info : sortedInput)
    {
        if (!info)
        {
            continue;
        }
        const auto &record = info->data.captureStreamInfo;
        const auto deviceStreamKey = std::make_pair(record.deviceId, record.streamId);
        const uint32_t batchId = batchIdMap[deviceStreamKey];
        const RecordKey recordKey = std::make_tuple(record.deviceId, record.modelId, record.originalStreamId,
                                                    record.streamId, record.captureStatus, info->timeStamp);
        if (seenRecords.find(recordKey) != seenRecords.end())
        {
            ++repeatedNum;
            continue;
        }
        if (record.captureStatus == 0)
        {
            startModelSet.insert(record.modelId);
        }
        if (record.captureStatus == 1 && !endModelSet.insert(record.modelId).second)
        {
            continue;
        }
        batchIdMap[deviceStreamKey] = batchId + 1;
        seenRecords.insert(recordKey);
        output.emplace_back(record.deviceId, record.modelId, record.originalStreamId, record.streamId, batchId,
                            record.captureStatus, info->timeStamp);
    }

    if (startModelSet != endModelSet)
    {
        WARN("CaptureStreamInfoDumper: Capture start model ids are %, end model ids are %.",
             FormatModelIds(startModelSet), FormatModelIds(endModelSet));
    }
    if (repeatedNum > 0)
    {
        WARN("CaptureStreamInfoDumper: There are % duplicate records.", repeatedNum);
    }
    return output;
}

CaptureStreamInfoDBData CaptureStreamInfoDumper::GenerateData(const CaptureStreamInfoData &input)
{
    CaptureStreamInfoDBData dbData;
    if (!Utils::Reserve(dbData, input.size()))
    {
        ERROR("CaptureStreamInfoDumper: Reserve DB data failed.");
        return {};
    }
    for (const auto &item : input)
    {
        dbData.emplace_back(item.deviceId, item.modelId, item.originalStreamId, item.streamId, item.batchId,
                            item.captureStatus, item.timeStamp);
    }
    return dbData;
}

}  // namespace Domain
}  // namespace Analysis
