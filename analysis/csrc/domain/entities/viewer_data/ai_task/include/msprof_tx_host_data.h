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

#ifndef ANALYSIS_DOMAIN_MSPROF_TX_HOST_DATA_H
#define ANALYSIS_DOMAIN_MSPROF_TX_HOST_DATA_H

#include <string>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Application;
struct MsprofTxHostData : public BasicData
{
    uint16_t eventType = UINT16_MAX;
    uint32_t pid = UINT32_MAX;
    uint32_t tid = UINT32_MAX;
    uint32_t category = UINT32_MAX;
    uint32_t payloadType = UINT32_MAX;
    uint32_t messageType = UINT32_MAX;
    uint64_t payloadValue = UINT64_MAX;
    uint64_t end = UINT64_MAX;
    uint64_t connectionId = DEFAULT_CONNECTION_ID_MSTX;
    std::string domain;
    std::string message;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_MSPROF_TX_HOST_DATA_H
