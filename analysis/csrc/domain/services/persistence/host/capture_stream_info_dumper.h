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

#ifndef ANALYSIS_PERSISTENCE_HOST_CAPTURE_STREAM_INFO_DUMPER_H
#define ANALYSIS_PERSISTENCE_HOST_CAPTURE_STREAM_INFO_DUMPER_H

#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"

namespace Analysis
{
namespace Domain
{

struct CaptureStreamInfoDataItem
{
    uint16_t deviceId = 0;
    uint32_t modelId = 0;
    uint32_t originalStreamId = 0;
    uint32_t streamId = 0;
    uint32_t batchId = 0;
    uint16_t captureStatus = 0;
    uint64_t timeStamp = 0;

    CaptureStreamInfoDataItem() = default;
    CaptureStreamInfoDataItem(uint16_t deviceIdValue, uint32_t modelIdValue, uint32_t originalStreamIdValue,
                              uint32_t streamIdValue, uint32_t batchIdValue, uint16_t captureStatusValue,
                              uint64_t timeStampValue)
        : deviceId(deviceIdValue),
          modelId(modelIdValue),
          originalStreamId(originalStreamIdValue),
          streamId(streamIdValue),
          batchId(batchIdValue),
          captureStatus(captureStatusValue),
          timeStamp(timeStampValue)
    {
    }
};

using CaptureStreamInfoData = std::vector<CaptureStreamInfoDataItem>;
using CaptureStreamInfoDBData =
    std::vector<std::tuple<uint16_t, uint32_t, uint32_t, uint32_t, uint32_t, uint16_t, uint64_t>>;

class CaptureStreamInfoDumper : public BaseDumper<CaptureStreamInfoDumper>
{
   public:
    explicit CaptureStreamInfoDumper(const std::string &hostPath);
    CaptureStreamInfoData FormatData(const std::vector<std::shared_ptr<ParserCompactInfo>> &input);
    CaptureStreamInfoDBData GenerateData(const CaptureStreamInfoData &input);
};

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_PERSISTENCE_HOST_CAPTURE_STREAM_INFO_DUMPER_H
