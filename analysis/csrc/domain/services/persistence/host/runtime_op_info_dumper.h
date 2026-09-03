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

#ifndef ANALYSIS_VIEWER_DATABASE_RUNTIME_OP_INFO_DUMPER_H
#define ANALYSIS_VIEWER_DATABASE_RUNTIME_OP_INFO_DUMPER_H

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/ascend_obj.h"
#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"

namespace Analysis
{
namespace Domain
{
using RuntimeOpInfo = Analysis::Domain::RuntimeOpInfo;
using RuntimeOpInfoMap = std::unordered_map<std::string, RuntimeOpInfo>;
using RuntimeOpInfoDumpData = std::vector<
    std::tuple<std::string, std::string, uint32_t, uint64_t, uint16_t, uint64_t, uint32_t, uint32_t, std::string,
               std::string, std::string, std::string, uint16_t, uint16_t, uint16_t, std::string, uint16_t, std::string,
               std::string, std::string, std::string, std::string, std::string>>;

class RuntimeOpInfoDumper final : public BaseDumper<RuntimeOpInfoDumper>
{
   public:
    explicit RuntimeOpInfoDumper(const std::string &hostFilePath);
    RuntimeOpInfoDumpData GenerateData(const std::vector<RuntimeOpInfo> &opInfoList);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_VIEWER_DATABASE_RUNTIME_OP_INFO_DUMPER_H
