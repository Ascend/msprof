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

#include "analysis/csrc/domain/services/parser/parser_item/chip6_pmu_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "securec.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;

namespace
{
int ParseChip6Pmu(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, HalPmuType type)
{
    if (binaryDataSize != sizeof(Chip6Pmu))
    {
        ERROR("The TrunkSize of PMU is not equal with the Chip6Pmu struct");
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto *chip6Pmu = ReinterpretConvert<Chip6Pmu *>(binaryData);
    auto *pmuData = ReinterpretConvert<HalPmuData *>(halUniData);
    pmuData->hd.taskId.streamId = UINT16_MAX;
    pmuData->hd.taskId.batchId = INVALID_BATCH_ID;
    pmuData->hd.taskId.taskId = chip6Pmu->taskId;
    pmuData->hd.taskId.contextId = INVALID_CONTEXT_ID;
    pmuData->hd.timestamp = chip6Pmu->endTime;

    pmuData->type = type;

    // V6 chip: determine acceleratorType by coreType and mix flag
    uint8_t mst = chip6Pmu->flags & 0x1;         // bit0: 主核标志
    uint8_t mix = (chip6Pmu->flags >> 1) & 0x1;  // bit1: mix标志
    uint8_t ovFlag = (chip6Pmu->flags >> 2) & 0x1;
    if (mix)
    {
        pmuData->pmu.acceleratorType = (chip6Pmu->coreType == 0) ? MIX_AIC : MIX_AIV;
    }
    else
    {
        pmuData->pmu.acceleratorType = (chip6Pmu->coreType == 0) ? AIC : AIV;
    }

    pmuData->pmu.ovFlag = ovFlag;
    pmuData->pmu.mst = mst;
    if (ovFlag)
    {
        WARN("An overflow in the operator taskId is %", chip6Pmu->taskId);
    }

    pmuData->pmu.totalCycle = chip6Pmu->totalCycle;
    pmuData->pmu.subBlockId = chip6Pmu->subBlockId;
    pmuData->pmu.blockId = chip6Pmu->blockId;
    pmuData->pmu.coreType = chip6Pmu->coreType;
    pmuData->pmu.coreId = chip6Pmu->coreId;

    pmuData->pmu.pmuList.resize(V6_PMU_LENGTH);
    std::copy(chip6Pmu->pmuList, chip6Pmu->pmuList + V6_PMU_LENGTH, pmuData->pmu.pmuList.begin());

    pmuData->pmu.timeList[0] = chip6Pmu->startTime;
    pmuData->pmu.timeList[1] = chip6Pmu->endTime;

    return chip6Pmu->cnt;
}
}  // namespace

int Chip6PmuParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    return ParseChip6Pmu(binaryData, binaryDataSize, halUniData, PMU);
}

int Chip6BlockPmuParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    return ParseChip6Pmu(binaryData, binaryDataSize, halUniData, BLOCK_PMU);
}

REGISTER_PARSER_ITEM(PMU_PARSER_V6, PARSER_ITEM_V6_CONTEXT_PMU, Chip6PmuParseItem);
REGISTER_PARSER_ITEM(PMU_PARSER_V6, PARSER_ITEM_V6_BLOCK_PMU, Chip6BlockPmuParseItem);
}  // namespace Domain
}  // namespace Analysis
