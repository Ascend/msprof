/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of the Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/services/persistence/host/ge_logic_stream_dumper.h"

namespace Analysis
{
namespace Domain
{
bool FormatLogicStreamInfo(const std::vector<std::shared_ptr<ParserAdditionalInfo>> &records, GeLogicStreamRows &rows)
{
    rows.clear();
    for (const auto &info : records)
    {
        if (!info)
        {
            continue;
        }
        const auto &data = info->logicStreamInfo;
        if (data.physicStreamNum > GE_LOGIC_STREAM_PHYSIC_SLOT)
        {
            ERROR("GE physical stream count exceeds %.", GE_LOGIC_STREAM_PHYSIC_SLOT);
            rows.clear();
            return false;
        }
        for (uint32_t index = 0; index < data.physicStreamNum; ++index)
        {
            rows.emplace_back(data.physicStreamIds[index], data.logicStreamId);
        }
    }
    return true;
}

GeLogicStreamDumper::GeLogicStreamDumper(const std::string &hostPath)
    : BaseDumper<GeLogicStreamDumper>(hostPath, "GeLogicStreamInfo")
{
    MAKE_SHARED0_NO_OPERATION(database_, GeLogicStreamDB);
}

GeLogicStreamRows GeLogicStreamDumper::GenerateData(const GeLogicStreamRows &rows) { return rows; }
}  // namespace Domain
}  // namespace Analysis
