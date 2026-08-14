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
#include "analysis/csrc/domain/services/persistence/host/hash_db_dumper.h"

namespace Analysis
{
namespace Domain
{
HashDBDumper::HashDBDumper(const std::string &hostFilePath) : BaseDumper<HashDBDumper>(hostFilePath, "GeHashInfo")
{
    MAKE_SHARED0_NO_OPERATION(database_, HashDB);
}

std::vector<std::tuple<std::string, std::string>> HashDBDumper::GenerateData(const HashDataMap &hashData)
{
    std::vector<std::tuple<std::string, std::string>> data;
    if (!Utils::Reserve(data, hashData.size()))
    {
        ERROR("Can't reserve vector");
        return data;
    }
    for (auto &pair : hashData)
    {
        data.emplace_back(std::to_string(pair.first), pair.second);
    }
    return data;
}
}  // namespace Domain
}  // namespace Analysis
