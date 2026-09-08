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

#include "analysis/csrc/domain/services/parser/parser_item/acsq_log_parser_item.h"

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

int AcsqLogParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    if (binaryDataSize != sizeof(AcsqLog))
    {
        ERROR("binaryDataSize is not equal to the size of AcsqLog size.");
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    auto *log = ReinterpretConvert<AcsqLog *>(binaryData);

    auto *unionData = ReinterpretConvert<HalLogData *>(halUniData);

    unionData->hd.taskId.streamId = StarsCommon::GetStreamId(log->streamId, log->taskId, expandStatus, log->taskType);
    unionData->hd.taskId.batchId = 0;
    unionData->hd.taskId.taskId = StarsCommon::GetTaskId(log->streamId, log->taskId, expandStatus, log->taskType);
    unionData->hd.taskId.contextId = INVALID_CONTEXT_ID;
    unionData->type = ACSQ_LOG;
    unionData->acsq.isEndTimestamp = log->funcType != PARSER_ITEM_ACSQ_LOG_START;
    unionData->hd.timestamp = log->timestamp;
    unionData->acsq.taskType = log->taskType;
    unionData->acsq.timestamp = log->timestamp;
    return log->cnt;
}

int AcsqLogParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    if (binaryDataSize != sizeof(AcsqLogV6))
    {
        ERROR("binaryDataSize is not equal to the size of AcsqLogV6 size.");
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    auto *log = ReinterpretConvert<AcsqLogV6 *>(binaryData);

    auto *unionData = ReinterpretConvert<HalLogData *>(halUniData);

    unionData->hd.taskId.batchId = 0;
    unionData->hd.taskId.streamId = 0;
    unionData->hd.taskId.taskId = log->taskId;
    unionData->hd.taskId.contextId = INVALID_CONTEXT_ID;
    unionData->type = ACSQ_LOG;
    unionData->acsq.isEndTimestamp = log->funcType != PARSER_ITEM_ACSQ_LOG_START;
    unionData->hd.timestamp = log->timestamp;
    unionData->acsq.taskType = log->taskType;
    unionData->acsq.timestamp = log->timestamp;
    return log->cnt;
}

int BlockLogParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    if (binaryDataSize != sizeof(BlockLogV6))
    {
        ERROR("binaryDataSize is not equal to the size of BlockLogV6 size.");
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    auto *log = ReinterpretConvert<BlockLogV6 *>(binaryData);
    auto *unionData = ReinterpretConvert<HalLogData *>(halUniData);
    // 解析后标记为无效日志不落盘
    unionData->type = INVALID_LOG;
    return log->cnt;
}

REGISTER_PARSER_ITEM(LOG_PARSER, PARSER_ITEM_ACSQ_LOG_START, AcsqLogParseItem);
REGISTER_PARSER_ITEM(LOG_PARSER, PARSER_ITEM_ACSQ_LOG_END, AcsqLogParseItem);
REGISTER_PARSER_ITEM(LOG_PARSER_V6, PARSER_ITEM_ACSQ_LOG_START, AcsqLogParseItem_V6);
REGISTER_PARSER_ITEM(LOG_PARSER_V6, PARSER_ITEM_ACSQ_LOG_END, AcsqLogParseItem_V6);
REGISTER_PARSER_ITEM(LOG_PARSER_V6, PARSER_ITEM_TOPIC_BLK_START_V6, BlockLogParseItem_V6);
REGISTER_PARSER_ITEM(LOG_PARSER_V6, PARSER_ITEM_TOPIC_BLK_END_V6, BlockLogParseItem_V6);
REGISTER_PARSER_ITEM(LOG_PARSER_V6, PARSER_ITEM_BLOCK_LOG_START_V6, BlockLogParseItem_V6);
REGISTER_PARSER_ITEM(LOG_PARSER_V6, PARSER_ITEM_BLOCK_LOG_END_V6, BlockLogParseItem_V6);
}  // namespace Domain
}  // namespace Analysis
