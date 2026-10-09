/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include "analysis/csrc/domain/services/parser/parser_item/ffts_plus_log_parser_item.h"

#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/stars_common.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;

int FftsPlusLogParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    if (binaryDataSize != sizeof(FftsPlusLog))
    {
        ERROR("binaryDataSize is not equal to the size of binaryData");
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto *log = ReinterpretConvert<FftsPlusLog *>(binaryData);

    auto *unionData = ReinterpretConvert<HalLogData *>(halUniData);

    unionData->hd.taskId.streamId = StarsCommon::GetStreamId(log->streamId, log->taskId, expandStatus, log->taskType);
    unionData->hd.taskId.batchId = 0;
    unionData->hd.taskId.taskId = StarsCommon::GetTaskId(log->streamId, log->taskId, expandStatus, log->taskType);
    unionData->hd.taskId.contextId = log->subTaskId;

    unionData->type = FFTS_LOG;

    if (log->funcType == PARSER_ITEM_FFTS_PLUS_LOG_START)
    {
        unionData->ffts.isEndTimestamp = false;
    }
    else
    {
        unionData->ffts.isEndTimestamp = true;
    }
    unionData->ffts.timestamp = log->timestamp;
    unionData->hd.timestamp = log->timestamp;
    unionData->ffts.fftsType = log->fftsType;
    unionData->ffts.subTaskType = log->subTaskType;
    unionData->ffts.threadId = log->threadId;
    return log->cnt;
}

REGISTER_PARSER_ITEM(LOG_PARSER, PARSER_ITEM_FFTS_PLUS_LOG_START, FftsPlusLogParseItem);
REGISTER_PARSER_ITEM(LOG_PARSER, PARSER_ITEM_FFTS_PLUS_LOG_END, FftsPlusLogParseItem);
}  // namespace Domain
}  // namespace Analysis
