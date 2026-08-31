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

#include "analysis/csrc/domain/services/parser/parser_item/task_block_num_parser_item.h"

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

int BlockNumParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    if (binaryDataSize != sizeof(BlockNum))
    {
        ERROR("BlockNumParseItem failure, struct is BlockNum");
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto *bin = ReinterpretConvert<BlockNum *>(binaryData);
    auto *uni = ReinterpretConvert<HalTrackData *>(halUniData);
    if (bin == nullptr || uni == nullptr)
    {
        ERROR("Struct of block num reinterpret convert failed.");
        return DEFAULT_CNT;
    }

    uni->hd.taskId.streamId = StarsCommon::GetStreamId(bin->streamId, bin->taskId, expandStatus);
    uni->hd.taskId.taskId = StarsCommon::GetTaskId(bin->streamId, bin->taskId, expandStatus);
    uni->hd.taskId.batchId = INVALID_BATCH_ID;
    uni->hd.taskId.contextId = INVALID_CONTEXT_ID;
    uni->hd.timestamp = bin->timestamp;

    uni->type = BLOCK_NUM;

    uni->blockNum.timestamp = bin->timestamp;
    uni->blockNum.blockNum = bin->blockNum;
    return DEFAULT_CNT;
}

int BlockNumParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    if (binaryDataSize != sizeof(BlockNum))
    {
        ERROR("BlockNumParseItem_V6 failure, struct is BlockNum");
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto *bin = ReinterpretConvert<BlockNum *>(binaryData);
    auto *uni = ReinterpretConvert<HalTrackData *>(halUniData);
    if (bin == nullptr || uni == nullptr)
    {
        ERROR("Struct of block num reinterpret convert failed.");
        return DEFAULT_CNT;
    }

    uni->hd.taskId.streamId = bin->streamId;
    uni->hd.taskId.taskId = (static_cast<uint32_t>(bin->taskId) << 16) | bin->streamId;
    uni->hd.taskId.batchId = INVALID_BATCH_ID;
    uni->hd.taskId.contextId = INVALID_CONTEXT_ID;
    uni->hd.timestamp = bin->timestamp;
    uni->type = BLOCK_NUM;
    uni->blockNum.timestamp = bin->timestamp;
    uni->blockNum.blockNum = bin->blockNum;
    return DEFAULT_CNT;
}

REGISTER_PARSER_ITEM(TRACK_PARSER, PARSER_ITEM_BLOCK_NUM, BlockNumParseItem);
// REGISTER_PARSER_ITEM(TRACK_PARSER_V6, PARSER_ITEM_BLOCK_NUM, BlockNumParseItem_V6);
}  // namespace Domain
}  // namespace Analysis
