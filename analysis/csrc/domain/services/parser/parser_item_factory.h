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

#ifndef MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_FACTORY_H
#define MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_FACTORY_H

#include <cstdint>
#include <functional>
#include <unordered_map>

namespace Analysis {
namespace Domain {
enum ParserType {
    LOG_PARSER,
    LOG_PARSER_V6,
    PMU_PARSER,
    TRACK_PARSER,
    FREQ_PARSER
};
const int MIN_COUNT = 2;
const int VALID_CNT = 15;
const int DEFAULT_CNT = -1;

class ParserItemFactory {
    using ItemFunc = std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)>;
    using ItemFuncMap = std::unordered_map<uint32_t, ItemFunc>;
public:
    ParserItemFactory(ParserType parserType, uint32_t itemType,
                      std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> parserItemFunc);
    static std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> GetParseItem(ParserType parserType, uint32_t itemType);
private:
    static std::unordered_map<ParserType, ItemFuncMap>& GetContainer();
};

#define PARSER_FACTORY_LINENAME(name, line) name##line
#define PARSER_FACTORY_LINENAME_HELPER(name, line) PARSER_FACTORY_LINENAME(name, line)
/**
 * @brief 注册Parser可以解析的数据类型
 *
 * @param parserType: Parser的类型(枚举类), 参考Domain::ParserType
 * @param itemType：可解析数据的类型(通常为FuncType)
 * @param Func: 实际解析数据函数
 */
#define REGISTER_PARSER_ITEM(parserType, itemType, Func) \
static Analysis::Domain::ParserItemFactory               \
    PARSER_FACTORY_LINENAME_HELPER(parserType, __LINE__)(parserType, itemType, Func)
}
}
#endif // MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_FACTORY_H
