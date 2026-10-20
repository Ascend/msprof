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

#include "analysis/csrc/domain/services/parser/parser_item/sio_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;

namespace
{
// 非 V6 与 V6 的 SIO 带宽字段名一致，按成员名统一填充。
template <typename RawData>
void FillSioBandwidth(const RawData* source, HalSioData& sio)
{
    sio.bandwidth[0] = source->reqRx;
    sio.bandwidth[1] = source->rspRx;
    sio.bandwidth[2] = source->snpRx;
    sio.bandwidth[3] = source->datRx;
    sio.bandwidth[4] = source->reqTx;
    sio.bandwidth[5] = source->rspTx;
    sio.bandwidth[6] = source->snpTx;
    sio.bandwidth[7] = source->datTx;
}
}  // namespace

// 非 V6 结构：SIO_FMT = "HHIQHH11I"
int SioParserItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus)
{
    (void)expandStatus;
    if (binaryData == nullptr || halUniData == nullptr || binaryDataSize != STARS_SOC_PROFILE_STRUCT_SIZE)
    {
        ERROR("The SIO record size is invalid, actual: %, expected: %", binaryDataSize, STARS_SOC_PROFILE_STRUCT_SIZE);
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto* target = ReinterpretConvert<HalLogData*>(halUniData);
    auto* source = ReinterpretConvert<SioV4RawData*>(binaryData);
    target->type = SOC_PROFILE_SIO;
    target->sio.sysCnt = source->sysCnt;
    target->sio.accId = source->accId;
    FillSioBandwidth(source, target->sio);
    return DEFAULT_CNT;
}

// V6 结构：V6_SIO_FMT = "BBHIQQ10I"
int SioParserItemV6(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus)
{
    (void)expandStatus;
    if (binaryData == nullptr || halUniData == nullptr || binaryDataSize != STARS_SOC_PROFILE_STRUCT_SIZE)
    {
        ERROR("The SIO record size is invalid, actual: %, expected: %", binaryDataSize, STARS_SOC_PROFILE_STRUCT_SIZE);
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto* target = ReinterpretConvert<HalLogData*>(halUniData);
    auto* source = ReinterpretConvert<SioV6RawData*>(binaryData);
    target->type = SOC_PROFILE_SIO;
    target->sio.sysCnt = source->sysCnt;
    // 与 Python SioDecoderV6 一致：只取第二字节的高 6 位，低 2 位属于 cnt，可非零。
    target->sio.accId = (source->accIdByte & 0xFCU) >> 2U;
    FillSioBandwidth(source, target->sio);
    return DEFAULT_CNT;
}

REGISTER_PARSER_ITEM(SOC_PROFILE_PARSER, PARSER_ITEM_SIO, SioParserItem);
REGISTER_PARSER_ITEM(SOC_PROFILE_PARSER_V6, PARSER_ITEM_SIO, SioParserItemV6);

}  // namespace Domain
}  // namespace Analysis
