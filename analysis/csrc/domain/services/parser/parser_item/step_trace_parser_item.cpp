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

#include "analysis/csrc/domain/services/parser/parser_item/step_trace_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/stars_common.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Utils;

int StepTraceParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    if (binaryDataSize != sizeof(StepTrace))
    {
        ERROR("StepTraceParseItem failure, struct is StepTrace");
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto *uni = ReinterpretConvert<HalTrackData *>(halUniData);
    uni->hd.taskId.batchId = INVALID_BATCH_ID;
    uni->hd.taskId.contextId = INVALID_CONTEXT_ID;
    uni->type = STEP_TRACE;

    auto *bin = ReinterpretConvert<StepTrace *>(binaryData);
    uni->hd.taskId.streamId = bin->streamId % STREAM_LOW_OPERATOR;
    uni->hd.taskId.taskId = bin->taskId;
    uni->hd.timestamp = bin->timestamp;
    uni->stepTrace.timestamp = bin->timestamp;
    uni->stepTrace.indexId = bin->indexId;
    uni->stepTrace.modelId = bin->modelId;
    uni->stepTrace.tagId = bin->tagId;
    return DEFAULT_CNT;
}

int StepTraceParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    if (binaryDataSize != sizeof(StepTraceV6))
    {
        ERROR("StepTraceParseItem_V6 failure, struct is StepTraceV6");
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto *bin = ReinterpretConvert<StepTraceV6 *>(binaryData);
    auto *uni = ReinterpretConvert<HalTrackData *>(halUniData);
    uni->hd.taskId.streamId = bin->streamId;
    uni->hd.taskId.taskId = bin->taskId;
    uni->hd.taskId.batchId = INVALID_BATCH_ID;
    uni->hd.taskId.contextId = INVALID_CONTEXT_ID;
    uni->hd.timestamp = bin->timestamp;
    uni->type = STEP_TRACE;
    uni->stepTrace.timestamp = bin->timestamp;
    uni->stepTrace.indexId = bin->indexId;
    uni->stepTrace.modelId = bin->modelId;
    uni->stepTrace.tagId = bin->tagId;
    return DEFAULT_CNT;
}

REGISTER_PARSER_ITEM(TRACK_PARSER, PARSER_ITEM_STEP_TRACE, StepTraceParseItem);
// REGISTER_PARSER_ITEM(TRACK_PARSER_V6, PARSER_ITEM_STEP_TRACE, StepTraceParseItem_V6);
}  // namespace Domain
}  // namespace Analysis
