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

#include "analysis/csrc/domain/services/parser/parser_item_factory.h"

#include "analysis/csrc/infrastructure/dfx/log.h"

namespace Analysis
{
namespace Domain
{
using ItemFunc = std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)>;
using ItemFuncMap = std::unordered_map<uint32_t, ItemFunc>;
ItemFunc ParserItemFactory::GetParseItem(ParserType parserType, uint32_t itemType)
{
    std::unordered_map<ParserType, ItemFuncMap> parserItemFuncs = GetContainer();
    auto it = parserItemFuncs.find(parserType);
    if (it == parserItemFuncs.end())
    {
        ERROR("GetParseItem return nullptr, parserType: %", parserType);
        return nullptr;
    }

    auto parserFunc = it->second;
    auto ans = parserFunc.find(itemType);
    if (ans == parserFunc.end())
    {
        ERROR("GetParseItemFunc return nullptr, parserType: %, itemType: %", parserType, itemType);
        return nullptr;
    }
    return ans->second;
}

ParserItemFactory::ParserItemFactory(ParserType parserType, uint32_t itemType, ItemFunc parserItemFunc)
{
    GetContainer()[parserType].emplace(itemType, parserItemFunc);
}

std::unordered_map<ParserType, ItemFuncMap> &ParserItemFactory::GetContainer()
{
    static std::unordered_map<ParserType, ItemFuncMap> parserItemFuncs;
    return parserItemFuncs;
}
}  // namespace Domain
}  // namespace Analysis
