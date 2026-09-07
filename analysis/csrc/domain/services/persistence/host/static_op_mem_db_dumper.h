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

#ifndef ANALYSIS_PERSISTENCE_HOST_STATIC_OP_MEM_DB_DUMPER_H
#define ANALYSIS_PERSISTENCE_HOST_STATIC_OP_MEM_DB_DUMPER_H

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"

namespace Analysis
{
namespace Domain
{
using StaticOpMemInfos = std::vector<std::shared_ptr<ParserAdditionalInfo>>;
using StaticOpMemDumpData = std::vector<std::tuple<std::string, std::string, uint32_t, uint64_t, uint64_t, double>>;

class StaticOpMemDBDumper final : public BaseDumper<StaticOpMemDBDumper>
{
   public:
    explicit StaticOpMemDBDumper(const std::string &hostFilePath);
    StaticOpMemDumpData GenerateData(const StaticOpMemInfos &staticOpMemInfos);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_PERSISTENCE_HOST_STATIC_OP_MEM_DB_DUMPER_H
