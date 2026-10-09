/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include "analysis/csrc/domain/services/persistence/host/type_info_db_dumper.h"

#include "analysis/csrc/domain/services/persistence/host/number_mapping.h"

namespace Analysis
{
namespace Domain
{

TypeInfoDBDumper::TypeInfoDBDumper(const std::string &hostFilePath)
    : BaseDumper<TypeInfoDBDumper>(hostFilePath, "TypeHashInfo")
{
    MAKE_SHARED0_NO_OPERATION(database_, HashDB);
}

std::vector<std::tuple<std::string, std::string, std::string>> TypeInfoDBDumper::GenerateData(
    const TypeInfoData &typeInfoData)
{
    std::vector<std::tuple<std::string, std::string, std::string>> data;
    if (!Utils::Reserve(data, typeInfoData.size()))
    {
        ERROR("Can't reserve vector");
        return data;
    }
    for (const auto &levelPair : typeInfoData)
    {
        for (const auto &hashPair : levelPair.second)
        {
            data.emplace_back(std::to_string(hashPair.first), hashPair.second,
                              NumberMapping::Get(NumberMapping::MappingType::LEVEL, levelPair.first));
        }
    }
    return data;
}
}  // namespace Domain
}  // namespace Analysis
