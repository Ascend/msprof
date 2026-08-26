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
#ifndef ANALYSIS_CSRC_VIEWER_DATABASE_MODEL_NAME_DB_DUMPER_H
#define ANALYSIS_CSRC_VIEWER_DATABASE_MODEL_NAME_DB_DUMPER_H

#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"

namespace Analysis
{
namespace Domain
{
using ModelNameData = std::vector<std::tuple<uint32_t, std::string>>;
class ModelNameDBDumper : public BaseDumper<ModelNameDBDumper>
{
    using GraphIdMaps = std::vector<std::shared_ptr<ParserAdditionalInfo>>;

   public:
    explicit ModelNameDBDumper(const std::string &hostFilePath);
    ModelNameData GenerateData(const GraphIdMaps &graphIdMaps);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_CSRC_VIEWER_DATABASE_MODEL_NAME_DB_DUMPER_H
