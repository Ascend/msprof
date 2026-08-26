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

#ifndef ANALYSIS_UTILS_ADAPTER_PARSER_STRUCT_ADAPTER_H
#define ANALYSIS_UTILS_ADAPTER_PARSER_STRUCT_ADAPTER_H

#include "analysis/csrc/domain/entities/hal/include/aicpu.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"

namespace Analysis
{
namespace Domain
{
namespace Adapter
{
using namespace Analysis::Domain;
class ParserCompactInfoAdapter
{
   public:
    static bool AdapterCompactInfo(const MsprofCompactInfo* compact, ParserCompactInfo* parsed,
                                   CompactInfoFormat parserType = CompactInfoFormat::COMPACT_INFO_TYPE);
    static bool AdapterRuntimeTrack(const MsprofCompactInfo* compact, RuntimeTrackFormat format,
                                    ParserCompactInfo* parsed);
    static bool AdapterCaptureStreamInfo(const MsprofCompactInfo* compact, CaptureStreamFormat format,
                                         ParserCompactInfo* parsed);
    static void AdapterDpuTrack(const MsprofCompactInfo* compact, ParserCompactInfo* parsed);
    static void AdapterNodeBasicInfo(const MsprofCompactInfo* compact, ParserCompactInfo* parsed);
    static void AdapterAttrInfo(const MsprofCompactInfo* compact, ParserCompactInfo* parsed);
    static void AdapterHcclopInfo(const MsprofCompactInfo* compact, ParserCompactInfo* parsed);
    static void AdapterMemcpyInfo(const MsprofCompactInfo* compact, ParserCompactInfo* parsed);
};

class ParserApiAdapter
{
   public:
    static bool AdapterApi(const MsprofApi* apiData, ParserApi* parsed);
};

class ParserAdditionalInfoAdapter
{
   public:
    static bool AdapterAdditionalInfo(MsprofAdditionalInfo* addition, ParserAdditionalInfo* parsed,
                                      AdditionalInfoFormat parserType = AdditionalInfoFormat::ADDITIONAL_INFO_TYPE);
    static void AdapterContextIdInfo(const MsprofAdditionalInfo* addition, ParserAdditionalInfo* parsed);
    static void AdapterFusionOpInfo(const MsprofAdditionalInfo* addition, ParserAdditionalInfo* parsed);
    static bool AdapterGraphId(MsprofAdditionalInfo* addition, ParserAdditionalInfo* parsed);
    static void AdapterHcclInfo(const MsprofAdditionalInfo* addition, ParserAdditionalInfo* parsed);
    static void AdapterMultiThread(const MsprofAdditionalInfo* addition, ParserAdditionalInfo* parsed);
};

class ParserAicpuAdapter
{
   public:
    static bool AdapterNode(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData);
    static bool AdapterModel(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData);
    static bool AdapterDp(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData);
    static bool AdapterMi(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData);
    static bool AdapterCommTurn(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData);
    static bool AdapterComputeTurn(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData);
    static bool AdapterOpInfo(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData);
    static bool AdapterFlipTask(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData);
    static bool AdapterMainStreamTask(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData);
    static bool AdapterKfcInfos(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData);
};

}  // namespace Adapter
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_UTILS_ADAPTER_PARSER_STRUCT_ADAPTER_H
