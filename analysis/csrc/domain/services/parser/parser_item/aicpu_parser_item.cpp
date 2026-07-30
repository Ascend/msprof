/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#include "analysis/csrc/domain/entities/hal/include/aicpu.h"
#include "analysis/csrc/domain/services/parser/parser_item/stars_common.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const int AICPU_NODE_TYPE = static_cast<int>(AicpuType::AICPU_NODE);
const int AICPU_DP_TYPE = static_cast<int>(AicpuType::AICPU_DP);
const int AICPU_MODEL_TYPE = static_cast<int>(AicpuType::AICPU_MODEL);
const int AICPU_MI_TYPE = static_cast<int>(AicpuType::AICPU_MI);
const int KFC_COMM_TURN_TYPE = static_cast<int>(AicpuType::KFC_COMM_TURN);
const int KFC_COMPUTE_TURN_TYPE = static_cast<int>(AicpuType::KFC_COMPUTE_TURN);
const int HCCL_OP_INFO_TYPE = static_cast<int>(AicpuType::HCCL_OP_INFO);
const int AICPU_FLIP_TASK_TYPE = static_cast<int>(AicpuType::AICPU_FLIP_TASK);
const int AICPU_MASTER_STREAM_HCCL_TASK_TYPE = static_cast<int>(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK);
const int KFC_HCCL_INFO_TYPE = static_cast<int>(AicpuType::KFC_HCCL_INFO);
}  // namespace
using namespace Analysis::Utils;

int AicpuNodeParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    data->node = additionalData->aicpuNode;

    data->taskId.streamId =
        StarsCommon::GetStreamId(additionalData->aicpuNode.streamId, additionalData->aicpuNode.taskId, expandStatus);
    data->taskId.batchId = 0;
    data->taskId.taskId =
        StarsCommon::GetTaskId(additionalData->aicpuNode.streamId, additionalData->aicpuNode.taskId, expandStatus);
    data->taskId.contextId = INVALID_CONTEXT_ID;
    data->type = AicpuType::AICPU_NODE;
    return ANALYSIS_OK;
}

int AicpuDpParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    data->dp = additionalData->aicpuDp;
    data->type = AicpuType::AICPU_DP;
    return ANALYSIS_OK;
}

int AicpuModelParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    data->model = additionalData->aicpuModel;
    data->type = AicpuType::AICPU_MODEL;
    return ANALYSIS_OK;
}

int AicpuMiParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    data->mi = additionalData->aicpuMi;
    data->type = AicpuType::AICPU_MI;
    return ANALYSIS_OK;
}

int KfcCommTurnParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    data->commTurn = additionalData->commTurn;

    data->taskId.streamId =
        StarsCommon::GetStreamId(additionalData->commTurn.streamId, additionalData->commTurn.taskId, expandStatus);
    data->taskId.batchId = 0;
    data->taskId.taskId =
        StarsCommon::GetTaskId(additionalData->commTurn.streamId, additionalData->commTurn.taskId, expandStatus);
    data->taskId.contextId = INVALID_CONTEXT_ID;
    data->type = AicpuType::KFC_COMM_TURN;
    return ANALYSIS_OK;
}

int KfcComputeTurnParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    data->computeTurn = additionalData->computeTurn;

    data->taskId.streamId = StarsCommon::GetStreamId(additionalData->computeTurn.streamId,
                                                     additionalData->computeTurn.taskId, expandStatus);
    data->taskId.batchId = 0;
    data->taskId.taskId =
        StarsCommon::GetTaskId(additionalData->computeTurn.streamId, additionalData->computeTurn.taskId, expandStatus);
    data->taskId.contextId = INVALID_CONTEXT_ID;
    data->type = AicpuType::KFC_COMPUTE_TURN;
    return ANALYSIS_OK;
}

int HcclOpInfoParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    data->opInfo = additionalData->opInfo;

    data->taskId.streamId =
        StarsCommon::GetStreamId(additionalData->opInfo.streamId, additionalData->opInfo.taskId, expandStatus);
    data->taskId.batchId = 0;
    data->taskId.taskId =
        StarsCommon::GetTaskId(additionalData->opInfo.streamId, additionalData->opInfo.taskId, expandStatus);
    data->taskId.contextId = INVALID_CONTEXT_ID;
    data->type = AicpuType::HCCL_OP_INFO;
    return ANALYSIS_OK;
}

int AicpuFlipTaskParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    data->flipTask = additionalData->flipTask;

    data->taskId.streamId =
        StarsCommon::GetStreamId(additionalData->flipTask.streamId, additionalData->flipTask.taskId, expandStatus);
    data->taskId.batchId = 0;
    data->taskId.taskId =
        StarsCommon::GetTaskId(additionalData->flipTask.streamId, additionalData->flipTask.taskId, expandStatus);
    data->taskId.contextId = INVALID_CONTEXT_ID;
    data->type = AicpuType::AICPU_FLIP_TASK;
    return ANALYSIS_OK;
}

int AicpuMasterStreamHcclTaskParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData,
                                       uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    data->mainStreamTask = additionalData->mainStreamTask;

    // aicpu task
    data->aicpuTaskId.streamId = StarsCommon::GetStreamId(additionalData->mainStreamTask.aicpuStreamId,
                                                          additionalData->mainStreamTask.aicpuTaskId, expandStatus);
    data->aicpuTaskId.batchId = 0;
    data->aicpuTaskId.taskId = StarsCommon::GetTaskId(additionalData->mainStreamTask.aicpuStreamId,
                                                      additionalData->mainStreamTask.aicpuTaskId, expandStatus);
    data->aicpuTaskId.contextId = INVALID_CONTEXT_ID;

    // aicpu expand task
    data->taskId.streamId = StarsCommon::GetStreamId(additionalData->mainStreamTask.streamId,
                                                     additionalData->mainStreamTask.taskId, expandStatus);
    data->taskId.batchId = 0;
    data->taskId.taskId = StarsCommon::GetTaskId(additionalData->mainStreamTask.streamId,
                                                 additionalData->mainStreamTask.taskId, expandStatus);
    data->taskId.contextId = INVALID_CONTEXT_ID;
    data->type = AicpuType::AICPU_MASTER_STREAM_HCCL_TASK;
    return ANALYSIS_OK;
}

int KfcHcclInfoParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    data->KfcInfos = additionalData->kfcInfos;
    data->type = AicpuType::KFC_HCCL_INFO;
    return ANALYSIS_OK;
}

REGISTER_PARSER_ITEM(AICPU_PARSER, AICPU_NODE_TYPE, AicpuNodeParseItem);
REGISTER_PARSER_ITEM(AICPU_PARSER, AICPU_DP_TYPE, AicpuDpParseItem);
REGISTER_PARSER_ITEM(AICPU_PARSER, AICPU_MODEL_TYPE, AicpuModelParseItem);
REGISTER_PARSER_ITEM(AICPU_PARSER, AICPU_MI_TYPE, AicpuMiParseItem);
REGISTER_PARSER_ITEM(AICPU_PARSER, KFC_COMM_TURN_TYPE, KfcCommTurnParseItem);
REGISTER_PARSER_ITEM(AICPU_PARSER, KFC_COMPUTE_TURN_TYPE, KfcComputeTurnParseItem);
REGISTER_PARSER_ITEM(AICPU_PARSER, HCCL_OP_INFO_TYPE, HcclOpInfoParseItem);
REGISTER_PARSER_ITEM(AICPU_PARSER, AICPU_FLIP_TASK_TYPE, AicpuFlipTaskParseItem);
REGISTER_PARSER_ITEM(AICPU_PARSER, AICPU_MASTER_STREAM_HCCL_TASK_TYPE, AicpuMasterStreamHcclTaskParseItem);
REGISTER_PARSER_ITEM(AICPU_PARSER, KFC_HCCL_INFO_TYPE, KfcHcclInfoParseItem);
}  // namespace Domain
}  // namespace Analysis
