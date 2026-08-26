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
#include "analysis/csrc/domain/services/persistence/host/model_name_db_dumper.h"

#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/domain/services/persistence/host/number_mapping.h"

namespace Analysis
{
namespace Domain
{
using HashData = Analysis::Domain::Host::Cann::HashData;
ModelNameDBDumper::ModelNameDBDumper(const std::string &hostFilePath)
    : BaseDumper<ModelNameDBDumper>(hostFilePath, "ModelName")
{
    MAKE_SHARED0_NO_OPERATION(database_, GeModelInfoDB);
}

ModelNameData ModelNameDBDumper::GenerateData(const GraphIdMaps &graphIdMaps)
{
    ModelNameData data;
    if (Utils::Reserve(data, graphIdMaps.size()))
    {
        for (auto &graphIdMap : graphIdMaps)
        {
            auto idMapStruct = graphIdMap->graphIdInfo;
            auto modelId = idMapStruct.modelId;
            auto modelName = (idMapStruct.modelName == 0) ? "0" : HashData::GetInstance().Get(idMapStruct.modelName);
            data.emplace_back(modelId, modelName);
        }
    }
    return data;
}
}  // namespace Domain
}  // namespace Analysis
