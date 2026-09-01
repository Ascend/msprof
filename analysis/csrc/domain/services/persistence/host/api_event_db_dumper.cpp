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

#include "analysis/csrc/domain/services/persistence/host/api_event_db_dumper.h"

#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"
#include "analysis/csrc/domain/services/persistence/host/number_mapping.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const uint32_t TWO_BYTES = 16;
}
using HashData = Analysis::Domain::Host::Cann::HashData;
using TypeData = Analysis::Domain::Host::Cann::TypeData;
const std::string GE_STEP_INFO_API_TYPE = "step_info";

ApiEventDBDumper::ApiEventDBDumper(const std::string &hostFilePath)
    : BaseDumper<ApiEventDBDumper>(hostFilePath, "ApiData")
{
    MAKE_SHARED0_NO_OPERATION(database_, ApiEventDB);
}

EventData ApiEventDBDumper::GenerateData(const ApiData &apiTraces)
{
    EventData data;
    if (!Utils::Reserve(data, apiTraces.size()))
    {
        return data;
    }
    for (auto &traceEvent : apiTraces)
    {
        std::string structType;
        std::string id;
        auto trace = traceEvent->apiPtr;
        // 除了acl层数据，其他数据的id设置为"0"
        // 对于acl层数据，type这个字段，高16位表示类型，低16位表示API的hash，所以这里右移
        if (trace->level == MSPROF_REPORT_ACL_LEVEL)
        {
            structType = NumberMapping::Get(NumberMapping::MappingType::ACL_API_TAG, trace->type >> TWO_BYTES);
            id = TypeData::GetInstance().Get(trace->level, trace->type);
        }
        else
        {
            structType = TypeData::GetInstance().Get(trace->level, trace->type);
            id = "0";
        }
        std::string itemId;
        if (trace->level == MSPROF_REPORT_MODEL_LEVEL ||
            (structType == GE_STEP_INFO_API_TYPE && trace->level == MSPROF_REPORT_NODE_LEVEL))
        {
            // node层的step_info以及model层, 原来的itemId出现了60000，
            // 60001等异常（预期为1）情况，所以使用structType作为判断依据
            itemId = std::to_string(trace->itemId);
        }
        else
        {
            itemId = (trace->itemId == 0) ? "0" : HashData::GetInstance().Get(trace->itemId);
        }
        if (trace->beginTime == 0)
        {
            // 过滤开始时间小于0的数据
            WARN("struct_type: %, item_id: %, id: % begin time is %, it will be filtered out", structType, itemId, id,
                 trace->beginTime);
            continue;
        }
        data.emplace_back(structType, id, NumberMapping::Get(NumberMapping::MappingType::LEVEL, trace->level),
                          trace->threadId, itemId, trace->beginTime, trace->endTime, traceEvent->id, traceEvent->key);
    }
    return data;
}
}  // namespace Domain
}  // namespace Analysis
