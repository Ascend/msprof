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

#include "analysis/csrc/domain/services/parser/parser_item/task_flip_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Utils;

int TaskFlipParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    if (binaryDataSize != sizeof(TaskFlip))
    {
        ERROR("TaskFlipParseItem failure, struct is TaskFlip");
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto *bin = ReinterpretConvert<TaskFlip *>(binaryData);
    auto *uni = ReinterpretConvert<HalTrackData *>(halUniData);

    uni->hd.taskId.streamId = bin->streamId;
    uni->hd.taskId.batchId = INVALID_BATCH_ID;
    uni->hd.taskId.taskId = bin->taskId;
    uni->hd.taskId.contextId = INVALID_CONTEXT_ID;
    uni->hd.timestamp = bin->timestamp;

    uni->type = TS_TASK_FLIP;

    uni->flip.timestamp = bin->timestamp;
    uni->flip.flipNum = bin->flipNum;

    return DEFAULT_CNT;
}

REGISTER_PARSER_ITEM(TRACK_PARSER, PARSER_ITEM_TASK_FLIP, TaskFlipParseItem);
// REGISTER_PARSER_ITEM(TRACK_PARSER_V6, PARSER_ITEM_TASK_FLIP, TaskFlipParseItem);
}  // namespace Domain
}  // namespace Analysis
