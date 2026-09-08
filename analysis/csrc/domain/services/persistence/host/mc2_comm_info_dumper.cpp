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

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using Environment::Context;

namespace
{
std::string FormatCommStreamIds(const ParserMc2CommInfo &payload)
{
    if (payload.commStreamSize > MSPROF_COMM_STREAM_MAX_NUM)
    {
        return "";
    }
    std::vector<std::string> ids;
    ids.reserve(payload.commStreamSize);
    for (uint32_t i = 0; i < payload.commStreamSize; ++i)
    {
        ids.emplace_back(std::to_string(payload.commStreamIds[i]));
    }
    return Utils::Join(ids, ",");
}
}  // namespace

Mc2CommInfoDumper::Mc2CommInfoDumper(const std::string &hostPath, CaptureStreamInfoData captureData)
    : BaseDumper<Mc2CommInfoDumper>(hostPath, "Mc2CommInfo"), captureData_(std::move(captureData)), hostPath_(hostPath)
{
    MAKE_SHARED0_NO_OPERATION(database_, Mc2CommInfoDB);
}

// 适配15 16 level0 device无streamId场景
bool Mc2CommInfoDumper::NeedMapInvalidStreamId() const
{
    auto &ctx = Context::GetInstance();
    if (!Context::IsChipV6(ctx.GetPlatformVersion()))
    {
        return false;
    }
    return ctx.IsLevel0(Utils::File::PathJoin({hostPath_, ".."}));
}

Mc2CommInfoData Mc2CommInfoDumper::GenerateData(const Mc2RawData &mc2Data)
{
    std::map<uint32_t, std::set<uint32_t>> captureStreamMap;
    for (const auto &capture : captureData_)
    {
        captureStreamMap[capture.originalStreamId].insert(capture.streamId);
    }

    const bool mapInvalidStreamId = NeedMapInvalidStreamId();
    size_t supplementSize = 0;
    std::set<uint32_t> invalidMappedStreamIds;  // 去重：同一 aicpuKfcStreamId 只补一条 65535 映射
    for (const auto &info : mc2Data)
    {
        if (!info)
        {
            continue;
        }
        const auto &payload = info->mc2CommInfo;
        auto captureIt = captureStreamMap.find(payload.aicpuKfcStreamId);
        if (captureIt != captureStreamMap.end())
        {
            supplementSize += captureIt->second.size();
        }
        if (mapInvalidStreamId)
        {
            invalidMappedStreamIds.insert(payload.aicpuKfcStreamId);
        }
    }
    supplementSize += invalidMappedStreamIds.size();

    Mc2CommInfoData output;
    if (!Utils::Reserve(output, mc2Data.size() + supplementSize))
    {
        ERROR("Mc2CommInfoDumper: Reserve data failed.");
        return {};
    }
    uint64_t invalidStreamSizeNum = 0;
    for (const auto &info : mc2Data)
    {
        if (!info)
        {
            continue;
        }
        const auto &payload = info->mc2CommInfo;
        if (payload.commStreamSize > MSPROF_COMM_STREAM_MAX_NUM)
        {
            ++invalidStreamSizeNum;
        }
        output.emplace_back(std::to_string(payload.groupName), payload.rankSize, payload.rankId, payload.usrRankId,
                            payload.aicpuKfcStreamId, FormatCommStreamIds(payload));
    }
    for (const auto &info : mc2Data)
    {
        if (!info)
        {
            continue;
        }
        const auto &payload = info->mc2CommInfo;
        auto captureIt = captureStreamMap.find(payload.aicpuKfcStreamId);
        if (captureIt == captureStreamMap.end())
        {
            continue;
        }
        for (auto modelStreamId : captureIt->second)
        {
            output.emplace_back(std::to_string(payload.groupName), payload.rankSize, payload.rankId, payload.usrRankId,
                                modelStreamId, FormatCommStreamIds(payload));
        }
    }
    if (mapInvalidStreamId)
    {
        for (const auto &info : mc2Data)
        {
            if (!info)
            {
                continue;
            }
            const auto &payload = info->mc2CommInfo;
            // aicpuKfcStreamId 不变，单独补一条 comm stream=65535 的映射；同一流只补一条
            if (invalidMappedStreamIds.erase(payload.aicpuKfcStreamId) == 0)
            {
                continue;
            }
            output.emplace_back(std::to_string(payload.groupName), payload.rankSize, payload.rankId, payload.usrRankId,
                                payload.aicpuKfcStreamId, std::to_string(UINT16_MAX));
        }
    }
    if (invalidStreamSizeNum > 0)
    {
        ERROR("Mc2CommInfoDumper: % records have stream size greater than max stream size %.", invalidStreamSizeNum,
              MSPROF_COMM_STREAM_MAX_NUM);
    }
    return output;
}

}  // namespace Domain
}  // namespace Analysis
