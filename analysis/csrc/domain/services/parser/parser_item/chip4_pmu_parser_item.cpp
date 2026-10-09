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

#include "analysis/csrc/domain/services/parser/parser_item/chip4_pmu_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/stars_common.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/domain/services/parser/pmu/pmu_accelerator_utils.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "securec.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;

int Chip4PmuParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    if (binaryDataSize != sizeof(ContextPmu))
    {
        ERROR("The TrunkSize of PMU is not equal with the ContextPmu struct");
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    auto *contextPmu = ReinterpretConvert<ContextPmu *>(binaryData);
    auto *pmuData = ReinterpretConvert<HalPmuData *>(halUniData);

    pmuData->hd.taskId.streamId = StarsCommon::GetStreamId(contextPmu->streamId, contextPmu->taskId, expandStatus);
    pmuData->hd.taskId.batchId = INVALID_BATCH_ID;
    pmuData->hd.taskId.taskId = StarsCommon::GetTaskId(contextPmu->streamId, contextPmu->taskId, expandStatus);
    if (contextPmu->fftsType == FFTS_PLUS)
    {
        pmuData->hd.taskId.contextId = contextPmu->subTaskId;
    }
    else
    {
        pmuData->hd.taskId.contextId = INVALID_CONTEXT_ID;
    }
    pmuData->hd.timestamp = contextPmu->timeList[1];  // 此处使用的是context级别PMU数据的结束时间
    pmuData->type = PMU;
    pmuData->pmu.acceleratorType = GetAcceleratorTypeByType(contextPmu->subTaskType, contextPmu->fftsType);
    pmuData->pmu.ovFlag = contextPmu->ovFlag;
    if (contextPmu->ovFlag)
    {
        WARN("An overflow in the operator taskId is %, streamId is %, contextId is %", contextPmu->taskId,
             contextPmu->streamId, contextPmu->subTaskId);
    }
    pmuData->pmu.totalCycle = contextPmu->totalCycle;
    std::copy(contextPmu->pmuList, contextPmu->pmuList + DEFAULT_LENGTH, pmuData->pmu.pmuList.begin());
    errno_t resTime = memcpy_s(pmuData->pmu.timeList, sizeof(pmuData->pmu.timeList), contextPmu->timeList,
                               sizeof(contextPmu->timeList));
    if (resTime != EOK)
    {
        ERROR("memcpy contextPmu failed! taskId is %, streamId is %, contextId is %", contextPmu->taskId,
              contextPmu->streamId, contextPmu->subTaskId);
    }
    return contextPmu->cnt;
}

REGISTER_PARSER_ITEM(PMU_PARSER, PARSER_ITEM_CONTEXT_PMU, Chip4PmuParseItem);
}  // namespace Domain
}  // namespace Analysis
