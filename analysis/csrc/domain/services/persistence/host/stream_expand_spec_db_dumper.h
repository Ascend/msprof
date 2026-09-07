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

#ifndef ANALYSIS_PERSISTENCE_HOST_STREAM_EXPAND_SPEC_DB_DUMPER_H
#define ANALYSIS_PERSISTENCE_HOST_STREAM_EXPAND_SPEC_DB_DUMPER_H

#include <memory>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"

namespace Analysis
{
namespace Domain
{
using StreamExpandSpecInfos = std::vector<std::shared_ptr<ParserCompactInfo>>;
using StreamExpandSpecDumpData = std::vector<std::tuple<uint16_t>>;

class StreamExpandSpecDBDumper final : public BaseDumper<StreamExpandSpecDBDumper>
{
   public:
    explicit StreamExpandSpecDBDumper(const std::string &hostFilePath);
    StreamExpandSpecDumpData GenerateData(const StreamExpandSpecInfos &streamExpandSpecInfos);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_PERSISTENCE_HOST_STREAM_EXPAND_SPEC_DB_DUMPER_H
