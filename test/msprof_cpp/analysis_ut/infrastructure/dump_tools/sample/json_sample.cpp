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
#include "sample/json_sample.h"
#include "analysis/csrc/infrastructure/dump_tools/include/dump_tool.h"

using namespace Analysis;
using namespace Infra;

JsonWriter& operator<<(JsonWriter& stream, const NestStructure& nestStruct)
{
    // 如果嵌套的结构是个数组，即在json中嵌套的部分是[]，则这里需要增加stream.StartArray()
    // 因为示例中，这里不是一个数组，就没有像下面那个函数一样写成循环了
    // 如果是数组，需要写成循环，参考下面那个函数
    stream.StartObject();
    if (nestStruct.columns.empty() || nestStruct.data.empty()) {
        // 如果column或data为空，是有问题的，非测试用例，这里应该记录日志
        stream.EndObject();
        return stream;
    }
    // get<0>不需要校验，如果tuple数量为0，编译会失败
    stream[nestStruct.columns[0].c_str()] << std::get<0>(nestStruct.data[0]);
    stream.EndObject();

    // 如果嵌套的结构是个数组，即在json中嵌套的部分是[]，则这里需要增加stream.EndArray()
    return stream;
}

JsonWriter& operator<<(JsonWriter& stream, const DumpToolSampleStruct& sample)
{
    stream.StartArray();

    constexpr size_t TP_SIZE = std::tuple_size<DumpToolSampleTp>{};
    std::array<const char*, TP_SIZE> colNames{};
    if (TP_SIZE != sample.columns.size()) {
        // 如果column的数量与tuple size不同，是有问题的，非测试用例，这里应该记录日志
        stream.EndArray();
        return stream;
    }
    // 下面先构造一个array，将columns的表头取出来
    for (size_t i = 0; i < colNames.size(); ++i) {
        colNames[i] = sample.columns[i].c_str();
    }

    for (const auto& oneDatum : sample.data) {
        stream.StartObject();
        DumpInJsonFormat(stream, colNames, oneDatum);
        stream.EndObject();
    }

    stream.EndArray();
    return stream;
}
