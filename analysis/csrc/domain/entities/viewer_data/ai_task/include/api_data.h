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

#ifndef ANALYSIS_DOMAIN_API_DATA_H
#define ANALYSIS_DOMAIN_API_DATA_H

#include <string>

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis
{
namespace Domain
{
struct ApiData : public BasicData
{
    uint16_t level = UINT16_MAX;
    uint32_t threadId = UINT32_MAX;
    uint64_t end = UINT64_MAX;
    uint64_t connectionId = UINT64_MAX;
    uint64_t event_id = 0;
    std::string apiName;
    std::string structType;  // api数据中的structType字段为runtime、model、node层数据的apiName
    std::string id;          // api数据中的id字段为acl层数据的apiName
    std::string itemId;      // api数据中hccl层数据的apiName
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_API_DATA_H
