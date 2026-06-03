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

#ifndef ANALYSIS_DOMAIN_SIO_FORMAT_DATA_H
#define ANALYSIS_DOMAIN_SIO_FORMAT_DATA_H

#include <string>

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis
{
namespace Domain
{
struct SioData : public BasicData
{
    uint16_t deviceId = UINT16_MAX;
    uint16_t dieId = UINT16_MAX;  // 0 ~ 1
    double reqRxBandwidth = 0;    // 请求流通道, MB/s
    double rspRxBandwidth = 0;    // 回应流通道, MB/s
    double snpRxBandwidth = 0;    // 侦听流通道, MB/s
    double datRxBandwidth = 0;    // 数据流通道, MB/s
    double reqTxBandwidth = 0;
    double rspTxBandwidth = 0;
    double snpTxBandwidth = 0;
    double datTxBandwidth = 0;
    std::string name;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SIO_FORMAT_DATA_H
