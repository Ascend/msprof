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
#ifndef ANALYSIS_UT_INFRASTRUCTURE_DUMP_TOOLS_SAMPLE_TYPES_H
#define ANALYSIS_UT_INFRASTRUCTURE_DUMP_TOOLS_SAMPLE_TYPES_H

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

// 如下的例子，是Json嵌套的情况，定义的结构中， DumpToolSampleStruct 中嵌套了 NestStructure
using NestStructTp = std::tuple<std::string>;
struct NestStructure
{
    std::vector<std::string> columns;
    std::vector<NestStructTp> data;
};

using DumpToolSampleTp = std::tuple<uint64_t, int64_t, std::string, double, float, NestStructure, int, bool>;
struct DumpToolSampleStruct
{
    std::vector<std::string> columns;
    std::vector<DumpToolSampleTp> data;
};

// 与Json不同在于，Csv不支持嵌套
using DumpToolSampleCsvTp = std::tuple<uint64_t, int64_t, std::string, double, float, int, bool>;
struct DumpToolSampleCsvStruct
{
    std::vector<std::string> columns;
    std::vector<DumpToolSampleCsvTp> data;
};

#endif
