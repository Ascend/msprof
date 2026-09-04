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

#include "analysis/csrc/domain/services/persistence/host/mc2_comm_info_dumper.h"

#include <map>
#include <set>
#include <sstream>

namespace Analysis
{
namespace Domain
{
using Host::Cann::MC2_COMM_STREAM_MAX_NUM;
using Host::Cann::MsprofMc2CommInfo;

namespace
{
std::string JoinCommStreamIds(const MsprofMc2CommInfo &payload)
{
    if (payload.streamSize > MC2_COMM_STREAM_MAX_NUM)
    {
        return "";
    }
    std::ostringstream stream;
    for (uint32_t i = 0; i < payload.streamSize; ++i)
    {
        if (i > 0)
        {
            stream << ',';
        }
        stream << payload.commStreamIds[i];
    }
    return stream.str();
}
}  // namespace

Mc2CommInfoDumper::Mc2CommInfoDumper(const std::string &hostPath)
    : BaseDumper<Mc2CommInfoDumper>(hostPath, "Mc2CommInfo")
{
    MAKE_SHARED0_NO_OPERATION(database_, Mc2CommInfoDB);
}

Mc2CommInfoData Mc2CommInfoDumper::GenerateData(const Mc2CommInfoInput &input)
{
    std::map<uint32_t, std::set<uint32_t>> captureStreamMap;
    for (const auto &capture : input.captureData)
    {
        captureStreamMap[capture.originalStreamId].insert(capture.streamId);
    }

    size_t supplementSize = 0;
    for (const auto &info : input.mc2Data)
    {
        if (!info)
        {
            continue;
        }
        const auto payload = Utils::ReinterpretConvert<const MsprofMc2CommInfo *>(info->data);
        auto captureIt = captureStreamMap.find(payload->streamId);
        if (captureIt != captureStreamMap.end())
        {
            supplementSize += captureIt->second.size();
        }
    }

    Mc2CommInfoData output;
    if (!Utils::Reserve(output, input.mc2Data.size() + supplementSize))
    {
        ERROR("Mc2CommInfoDumper: Reserve data failed.");
        return {};
    }
    uint64_t invalidStreamSizeNum = 0;
    for (const auto &info : input.mc2Data)
    {
        if (!info)
        {
            continue;
        }
        const auto payload = Utils::ReinterpretConvert<const MsprofMc2CommInfo *>(info->data);
        if (payload->streamSize > MC2_COMM_STREAM_MAX_NUM)
        {
            ++invalidStreamSizeNum;
        }
        output.emplace_back(std::to_string(payload->groupName), payload->rankSize, payload->rankId, payload->usrRankId,
                            payload->streamId, JoinCommStreamIds(*payload));
    }
    for (const auto &info : input.mc2Data)
    {
        if (!info)
        {
            continue;
        }
        const auto payload = Utils::ReinterpretConvert<const MsprofMc2CommInfo *>(info->data);
        auto captureIt = captureStreamMap.find(payload->streamId);
        if (captureIt == captureStreamMap.end())
        {
            continue;
        }
        for (auto modelStreamId : captureIt->second)
        {
            output.emplace_back(std::to_string(payload->groupName), payload->rankSize, payload->rankId,
                                payload->usrRankId, modelStreamId, JoinCommStreamIds(*payload));
        }
    }
    if (invalidStreamSizeNum > 0)
    {
        ERROR("Mc2CommInfoDumper: % records have stream size greater than max stream size %.", invalidStreamSizeNum,
              MC2_COMM_STREAM_MAX_NUM);
    }
    return output;
}

}  // namespace Domain
}  // namespace Analysis
