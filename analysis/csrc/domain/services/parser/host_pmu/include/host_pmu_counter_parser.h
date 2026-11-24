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
 * MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef ANALYSIS_DOMAIN_HOST_PMU_COUNTER_PARSER_H
#define ANALYSIS_DOMAIN_HOST_PMU_COUNTER_PARSER_H

#include <string>
#include <vector>

#include "analysis/csrc/domain/entities/viewer_data/system/include/pmu_counter_data.h"

namespace Analysis
{
namespace Domain
{
// Reads the host PMU collector's text artifact: <profPath>/data/pmu/host_pmu_<pid|all>.data.
class HostPmuCounterParser
{
   public:
    static std::vector<std::string> LocateFiles(const std::string &profPath);
    static bool ParseFile(const std::string &path, PmuCounterBundle &bundle, std::string &errorMsg);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_HOST_PMU_COUNTER_PARSER_H
