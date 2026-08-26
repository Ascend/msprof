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

#include "analysis/csrc/domain/services/parser/parser_item/acc_pmu_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
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

int AccPmuParserItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    if (binaryDataSize != sizeof(AccPmu))
    {
        ERROR("The trunkSize of accPmu is not equal with the AccPmu struct");
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    auto *accPmu = ReinterpretConvert<AccPmu *>(binaryData);
    auto *unionData = ReinterpretConvert<HalLogData *>(halUniData);
    unionData->type = ACC_PMU;
    unionData->accPmu.timestamp = accPmu->timestamp;
    unionData->accPmu.accId = accPmu->accId;
    unionData->hd.timestamp = accPmu->timestamp;
    errno_t resBandwidth = memcpy_s(unionData->accPmu.bandwidth, sizeof(unionData->accPmu.bandwidth), accPmu->bandwidth,
                                    sizeof(accPmu->bandwidth));
    errno_t resOst = memcpy_s(unionData->accPmu.ost, sizeof(unionData->accPmu.ost), accPmu->ost, sizeof(accPmu->ost));
    if (resBandwidth != EOK || resOst != EOK)
    {
        ERROR("memcpy accPmu failed! accId is %", accPmu->accId);
    }
    return accPmu->cnt;
}

REGISTER_PARSER_ITEM(LOG_PARSER, PARSER_ITEM_ACC_PMU, AccPmuParserItem);
}  // namespace Domain
}  // namespace Analysis
