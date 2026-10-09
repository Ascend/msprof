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

#include "analysis/csrc/domain/services/parser/parser_item/block_pmu_parser_item.h"

#include <iostream>

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

int BlockPmuParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    if (binaryDataSize != sizeof(BlockPmu))
    {
        ERROR("The TrunkSize of PMU is not equal with the ContextPmu struct");
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto *blockPmu = ReinterpretConvert<BlockPmu *>(binaryData);
    auto *pmuData = ReinterpretConvert<HalPmuData *>(halUniData);

    pmuData->hd.taskId.streamId = StarsCommon::GetStreamId(blockPmu->streamId, blockPmu->taskId, expandStatus);
    pmuData->hd.taskId.batchId = INVALID_BATCH_ID;
    pmuData->hd.taskId.taskId = StarsCommon::GetTaskId(blockPmu->streamId, blockPmu->taskId, expandStatus);
    pmuData->hd.taskId.contextId = blockPmu->subTaskId;
    pmuData->hd.timestamp = blockPmu->timeList[1];  // 使用结束时间进行batchId的匹配

    pmuData->type = BLOCK_PMU;
    pmuData->pmu.acceleratorType = GetAcceleratorTypeByType(blockPmu->subTaskType, blockPmu->fftsType);
    pmuData->pmu.subBlockId = blockPmu->subBlockId;
    pmuData->pmu.blockId = blockPmu->blockId;
    pmuData->pmu.totalCycle = blockPmu->totalCycle;
    pmuData->pmu.coreType = blockPmu->coreType;
    pmuData->pmu.coreId = blockPmu->coreId;
    std::copy(blockPmu->pmuList, blockPmu->pmuList + DEFAULT_LENGTH, pmuData->pmu.pmuList.begin());
    errno_t resTime =
        memcpy_s(pmuData->pmu.timeList, sizeof(pmuData->pmu.timeList), blockPmu->timeList, sizeof(blockPmu->timeList));
    if (resTime != EOK)
    {
        ERROR("memcpy blockPmu failed! taskId is %, streamId is %, contextId is %", blockPmu->taskId,
              blockPmu->streamId, blockPmu->subTaskId);
    }
    return blockPmu->cnt;
}

REGISTER_PARSER_ITEM(PMU_PARSER, PARSER_ITEM_BLOCK_PMU, BlockPmuParseItem);
}  // namespace Domain
}  // namespace Analysis
