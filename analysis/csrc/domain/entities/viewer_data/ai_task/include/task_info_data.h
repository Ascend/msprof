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
#ifndef ANALYSIS_DOMAIN_TASK_INFO_DATA_H
#define ANALYSIS_DOMAIN_TASK_INFO_DATA_H

#include <stdint.h>

#include <string>

namespace Analysis
{
namespace Domain
{
const uint8_t DEFAULT_MULTIPLE_SIZE = 2;
const std::string SINGLE_OPERATOR = "\"";
const std::string CSV_OPERATOR = R"(""")";

struct ShapeInfo
{
    std::string inputFormats;
    std::string inputDataTypes;
    std::string inputShapes;
    std::string outputFormats;
    std::string outputDataTypes;
    std::string outputShapes;
};

struct TaskInfoData
{
    uint16_t deviceId = UINT16_MAX;
    uint32_t modelId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t blockNum = UINT32_MAX;
    uint32_t mixBlockNum = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    uint32_t contextId = UINT32_MAX;
    std::string opState;
    std::string hashId;
    std::string opName;
    std::string taskType;
    std::string opType;
    std::string opFlag;
    std::string inputFormats;
    std::string inputDataTypes;
    std::string inputShapes;
    std::string outputFormats;
    std::string outputDataTypes;
    std::string outputShapes;
    std::string gridDim;
    std::string blockDim;

    TaskInfoData &operator=(const ShapeInfo &shapeInfo)
    {
        inputFormats = escapeQuotes(shapeInfo.inputFormats);
        inputDataTypes = escapeQuotes(shapeInfo.inputDataTypes);
        inputShapes = escapeQuotes(shapeInfo.inputShapes);
        outputFormats = escapeQuotes(shapeInfo.outputFormats);
        outputDataTypes = escapeQuotes(shapeInfo.outputDataTypes);
        outputShapes = escapeQuotes(shapeInfo.outputShapes);
        return *this;
    }

   private:
    std::string escapeQuotes(const std::string &input)
    {
        std::string res = input;
        res.reserve(input.size() * DEFAULT_MULTIPLE_SIZE);
        std::string::size_type pos = 0;
        while ((pos = res.find(SINGLE_OPERATOR, pos)) != std::string::npos)
        {
            res.replace(pos, SINGLE_OPERATOR.length(), CSV_OPERATOR);
            pos += CSV_OPERATOR.length();
        }
        return res;
    }
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_TASK_INFO_DATA_H
