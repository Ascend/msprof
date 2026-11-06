/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include "analysis/csrc/domain/entities/hal/include/aicpu.h"
#include "analysis/csrc/domain/services/adapter/parser_struct_adapter.h"
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

/**
 * @brief V6（chip 15/16/19）场景下的 taskId 计算规则
 *
 * 与 Python 侧 StarsCommon.set_unique_task_id 对齐：16bit taskId 扩展为 32bit，
 * 低 16 位为 streamId、高 16 位为原始 taskId。V6 下 taskId 为 u32，不能再用
 * StarsCommon::GetTaskId 的 16bit 展开规则。
 */
uint32_t GetUniqueTaskIdV6(uint16_t streamId, uint16_t taskId)
{
    return (static_cast<uint32_t>(taskId) << 16) | static_cast<uint32_t>(streamId);
}
}  // namespace
using namespace Analysis::Domain::Adapter;
using namespace Analysis::Utils;

int AicpuNodeParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    if (!ParserAicpuAdapter::AdapterNode(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }

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
    if (!ParserAicpuAdapter::AdapterDp(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }
    data->type = AicpuType::AICPU_DP;
    return ANALYSIS_OK;
}

int AicpuModelParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    if (!ParserAicpuAdapter::AdapterModel(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }
    data->type = AicpuType::AICPU_MODEL;
    return ANALYSIS_OK;
}

int AicpuMiParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    if (!ParserAicpuAdapter::AdapterMi(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }
    data->type = AicpuType::AICPU_MI;
    return ANALYSIS_OK;
}

int KfcCommTurnParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    if (!ParserAicpuAdapter::AdapterCommTurn(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }

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
    if (!ParserAicpuAdapter::AdapterComputeTurn(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }

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
    if (!ParserAicpuAdapter::AdapterOpInfo(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }

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
    if (!ParserAicpuAdapter::AdapterFlipTask(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }

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
    if (!ParserAicpuAdapter::AdapterMainStreamTask(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }

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
    if (!ParserAicpuAdapter::AdapterKfcInfos(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }
    data->type = AicpuType::KFC_HCCL_INFO;
    return ANALYSIS_OK;
}

/*
 * ============================ V6 parser items ============================
 * chip 15/16/19 下 streamId 保持上报原值（Python StarsCommon.set_stream_id 的 v6
 * 分支直接返回原值，不做任何掩码/展开）；taskId 按各记录自身的 need_merge 语义处理：
 *   - need_merge=False（AICPU_NODE / HCCL_OP_INFO）：保持上报原值
 *   - need_merge=True （KFC_COMM_TURN / KFC_COMPUTE_TURN / AICPU_FLIP_TASK /
 *                       AICPU_MASTER_STREAM_HCCL_TASK）：合并为 32bit
 * KFC_HCCL_INFO 在 v6 下 streamId/taskId 均为原值，与非 V6 实现等价，直接复用。
 */
int AicpuNodeParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    if (!ParserAicpuAdapter::AdapterNode(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }

    data->taskId.streamId = additionalData->aicpuNode.streamId;
    data->taskId.batchId = 0;
    // need_merge=False：taskId 保持上报原值（Python AicpuNodeBean 同此语义）
    data->taskId.taskId = additionalData->aicpuNode.taskId;
    data->taskId.contextId = INVALID_CONTEXT_ID;
    data->type = AicpuType::AICPU_NODE;
    return ANALYSIS_OK;
}

int KfcCommTurnParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    if (!ParserAicpuAdapter::AdapterCommTurn(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }

    data->taskId.streamId = additionalData->commTurn.streamId;
    data->taskId.batchId = 0;
    data->taskId.taskId = GetUniqueTaskIdV6(additionalData->commTurn.streamId, additionalData->commTurn.taskId);
    data->taskId.contextId = INVALID_CONTEXT_ID;
    data->type = AicpuType::KFC_COMM_TURN;
    return ANALYSIS_OK;
}

int KfcComputeTurnParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    if (!ParserAicpuAdapter::AdapterComputeTurn(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }

    data->taskId.streamId = additionalData->computeTurn.streamId;
    data->taskId.batchId = 0;
    data->taskId.taskId = GetUniqueTaskIdV6(additionalData->computeTurn.streamId, additionalData->computeTurn.taskId);
    data->taskId.contextId = INVALID_CONTEXT_ID;
    data->type = AicpuType::KFC_COMPUTE_TURN;
    return ANALYSIS_OK;
}

int HcclOpInfoParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    if (!ParserAicpuAdapter::AdapterOpInfo(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }

    data->taskId.streamId = additionalData->opInfo.streamId;
    data->taskId.batchId = 0;
    // need_merge=False：opInfo.taskId 上报即为 u32，直接透传（Python DeviceHcclOpInfoBean）
    data->taskId.taskId = additionalData->opInfo.taskId;
    data->taskId.contextId = INVALID_CONTEXT_ID;
    data->type = AicpuType::HCCL_OP_INFO;
    return ANALYSIS_OK;
}

int AicpuFlipTaskParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    if (!ParserAicpuAdapter::AdapterFlipTask(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }

    data->taskId.streamId = additionalData->flipTask.streamId;
    data->taskId.batchId = 0;
    data->taskId.taskId = GetUniqueTaskIdV6(additionalData->flipTask.streamId, additionalData->flipTask.taskId);
    data->taskId.contextId = INVALID_CONTEXT_ID;
    data->type = AicpuType::AICPU_FLIP_TASK;
    return ANALYSIS_OK;
}

int AicpuMasterStreamHcclTaskParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData,
                                          uint16_t expandStatus)
{
    auto *additionalData = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    auto *data = ReinterpretConvert<AicpuData *>(aicpuData);
    data->timeStamp = additionalData->timeStamp;
    if (!ParserAicpuAdapter::AdapterMainStreamTask(additionalData, data))
    {
        return ANALYSIS_ERROR;
    }

    // V6 下 taskId 需扩展为 32bit（高 16 位原 taskId、低 16 位 streamId），streamId 保持原值。
    // 合并结果同时写入解析侧 union 字段与外层 TaskId：前者是落盘与注入数据的取值来源，
    // 后者供批次计算等按 TaskId 访问的路径使用，两者保持一致。
    // aicpu task
    data->mainStreamTask.aicpuStreamId = additionalData->mainStreamTask.aicpuStreamId;
    data->mainStreamTask.aicpuTaskId =
        GetUniqueTaskIdV6(additionalData->mainStreamTask.aicpuStreamId, additionalData->mainStreamTask.aicpuTaskId);
    data->aicpuTaskId.streamId = data->mainStreamTask.aicpuStreamId;
    data->aicpuTaskId.batchId = 0;
    data->aicpuTaskId.taskId = data->mainStreamTask.aicpuTaskId;
    data->aicpuTaskId.contextId = INVALID_CONTEXT_ID;

    // aicpu expand task
    data->mainStreamTask.streamId = additionalData->mainStreamTask.streamId;
    data->mainStreamTask.taskId =
        GetUniqueTaskIdV6(additionalData->mainStreamTask.streamId, additionalData->mainStreamTask.taskId);
    data->taskId.streamId = data->mainStreamTask.streamId;
    data->taskId.batchId = 0;
    data->taskId.taskId = data->mainStreamTask.taskId;
    data->taskId.contextId = INVALID_CONTEXT_ID;
    data->type = AicpuType::AICPU_MASTER_STREAM_HCCL_TASK;
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

// V6：DP/MODEL/MI 不携带 taskId/streamId，KFC_HCCL_INFO 在 V6 下为原值透传，
// 四者与非 V6 实现完全等价，复用同一函数；其余 6 类需要 V6 专用实现。
REGISTER_PARSER_ITEM(AICPU_PARSER_V6, AICPU_NODE_TYPE, AicpuNodeParseItem_V6);
REGISTER_PARSER_ITEM(AICPU_PARSER_V6, AICPU_DP_TYPE, AicpuDpParseItem);
REGISTER_PARSER_ITEM(AICPU_PARSER_V6, AICPU_MODEL_TYPE, AicpuModelParseItem);
REGISTER_PARSER_ITEM(AICPU_PARSER_V6, AICPU_MI_TYPE, AicpuMiParseItem);
REGISTER_PARSER_ITEM(AICPU_PARSER_V6, KFC_COMM_TURN_TYPE, KfcCommTurnParseItem_V6);
REGISTER_PARSER_ITEM(AICPU_PARSER_V6, KFC_COMPUTE_TURN_TYPE, KfcComputeTurnParseItem_V6);
REGISTER_PARSER_ITEM(AICPU_PARSER_V6, HCCL_OP_INFO_TYPE, HcclOpInfoParseItem_V6);
REGISTER_PARSER_ITEM(AICPU_PARSER_V6, AICPU_FLIP_TASK_TYPE, AicpuFlipTaskParseItem_V6);
REGISTER_PARSER_ITEM(AICPU_PARSER_V6, AICPU_MASTER_STREAM_HCCL_TASK_TYPE, AicpuMasterStreamHcclTaskParseItem_V6);
REGISTER_PARSER_ITEM(AICPU_PARSER_V6, KFC_HCCL_INFO_TYPE, KfcHcclInfoParseItem);
}  // namespace Domain
}  // namespace Analysis
