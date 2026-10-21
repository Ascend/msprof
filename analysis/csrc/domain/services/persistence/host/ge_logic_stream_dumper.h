/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of the Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_CSRC_DOMAIN_SERVICES_PERSISTENCE_HOST_GE_LOGIC_STREAM_DUMPER_H
#define ANALYSIS_CSRC_DOMAIN_SERVICES_PERSISTENCE_HOST_GE_LOGIC_STREAM_DUMPER_H

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
using GeLogicStreamRow = std::tuple<uint32_t, uint32_t>;
using GeLogicStreamRows = std::vector<GeLogicStreamRow>;

bool FormatLogicStreamInfo(const std::vector<std::shared_ptr<ParserAdditionalInfo>> &records, GeLogicStreamRows &rows);

class GeLogicStreamDumper : public BaseDumper<GeLogicStreamDumper>
{
   public:
    explicit GeLogicStreamDumper(const std::string &hostPath);
    GeLogicStreamRows GenerateData(const GeLogicStreamRows &rows);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_CSRC_DOMAIN_SERVICES_PERSISTENCE_HOST_GE_LOGIC_STREAM_DUMPER_H
