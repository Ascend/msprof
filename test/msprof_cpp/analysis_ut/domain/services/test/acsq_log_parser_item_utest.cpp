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

#include "gtest/gtest.h"
#include "analysis/csrc/domain/services/parser/parser_item/acsq_log_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"

namespace Analysis {
    using namespace testing;
    using namespace Analysis::Domain;

    class AcsqLogParserItemUtest : public Test {
    protected:
        void SetUp() override
        {
        }
    };

    TEST_F(AcsqLogParserItemUtest, TestAcsqLogParseItemV6ShouldReturnErrorWhenSizeNotMatch)
    {
        AcsqLogV6 acsqLogV6;
        HalLogData halLogData;
        ASSERT_EQ(AcsqLogParseItem_V6(reinterpret_cast<uint8_t*>(&acsqLogV6),
            99, reinterpret_cast<uint8_t*>(&halLogData), false), Analysis::PARSER_ERROR_SIZE_MISMATCH);
    }

    TEST_F(AcsqLogParserItemUtest, TestAcsqLogParseItemV6Success)
    {
        AcsqLogV6 acsqLogV6;
        acsqLogV6.cnt = 9;
        HalLogData halLogData;
        ASSERT_EQ(AcsqLogParseItem_V6(reinterpret_cast<uint8_t*>(&acsqLogV6),
            32, reinterpret_cast<uint8_t*>(&halLogData), false), 9);
    }
}
