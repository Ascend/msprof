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
#include <vector>
#include <gtest/gtest.h>
#include "test/msprof_cpp/analysis_ut/infrastructure/dump_tools/sample/sample_types.h"
#include "test/msprof_cpp/analysis_ut/infrastructure/dump_tools/sample/json_sample.h"
#include "analysis/csrc/infrastructure/dump_tools/json_tool/include/json_writer.h"

using namespace std;
using namespace testing;
using namespace Analysis;
using namespace Infra;

TEST(JsonDumpUTest, ShouldGetRightJsonWhenGivenSpecificData)
{
    // given
    float fl = -1.5f; // float类型
    uint64_t u64 = 10823372036854775807ull;  // u64类型
    std::string str = "string sample!";  // string类型
    JsonWriter jsonStream;

    // when
    jsonStream.StartObject();  // 必须有这个，不然会异常
    jsonStream["fl_column"] << fl;
    jsonStream["u64_col"] << u64;
    jsonStream["str_col"] << str;
    jsonStream["temp"] << 'a';   // char 类型
    jsonStream["temp"] << "some thing!"; // 字符串
    jsonStream.EndObject();

    // then
    std::string expectJsonStr = "{\"fl_column\":-1.5,\"u64_col\":10823372036854775807,"
                                "\"str_col\":\"string sample!\",\"temp\":97,\"temp\":true}";
    EXPECT_EQ(expectJsonStr, jsonStream.GetString());
}

TEST(JsonDumpUTest, ShouldGetRightJsonWhenGivenTupleFormatData)
{
    // given
    DumpToolSampleStruct sampleStruct = {
        {"u64", "i64", "string", "db", "fl", "NestStr", "i", "bl"},
        {
            {9823372036854775807ull, 9223372036854775807ll, "test_string", 3.1415926535897932384626, 2.7182818,
                {{"n_string"}, {{"nest struct"}}}, // nest
                100, true},
            {200, -100, "test_string2", -3.1415926535897932384626, -2.7182818,
                {{"n_string"}, {{"nest struct2"}}}, // nest
                -100, false}
        }
    };


    // when
    JsonWriter jsonStream;
    jsonStream << sampleStruct;

    // then
    string expectStr = "[{\"u64\":9823372036854775807,\"i64\":9223372036854775807,\"string\":\"test_string\","
        "\"db\":3.141592653589793,\"fl\":2.7182817459106445,\"NestStr\":{\"n_string\":\"nest struct\"},"
        "\"i\":100,\"bl\":true},{\"u64\":200,\"i64\":-100,\"string\":\"test_string2\",\"db\":-3.141592653589793,"
        "\"fl\":-2.7182817459106445,\"NestStr\":{\"n_string\":\"nest struct2\"},\"i\":-100,\"bl\":false}]";
    EXPECT_EQ(expectStr, jsonStream.GetString());
}

TEST(JsonDumpUTest, ShouldGetNullJsonStringWhenColumnSizeWrong)
{
    // given
    DumpToolSampleStruct sampleStruct = {
        {"u64", "i64", "string", "db", "fl", "NestStr", "i"}, // 和正常相比，少了一个
        {{0, 0, "", 0, 0, {{""}, {{""}}}, 100, true}}
    };

    // when
    JsonWriter jsonStream;
    jsonStream << sampleStruct;

    std::string expectStr = "[]";
    EXPECT_EQ(expectStr, jsonStream.GetString());
}

TEST(JsonDumpUTest, ShouldGetNullNestJsonStringWhenNestColumnSizeWrong)
{
    // given
    DumpToolSampleStruct sampleStruct = {
            {"u64", "i64", "string", "db", "fl", "NestStr", "i", "bl"},
            {{0, 0, "", 0, 0, {{}, {{""}}}, // 和正常相比，没有写column
              100, true}}
    };

    // when
    JsonWriter jsonStream;
    jsonStream << sampleStruct;

    std::string expectStr = "[{\"u64\":0,\"i64\":0,\"string\":\"\",\"db\":0.0,\"fl\":0.0,"
                            "\"NestStr\":{},"  // 校验这里是{}
                            "\"i\":100,\"bl\":true}]";
    EXPECT_EQ(expectStr, jsonStream.GetString());
}
