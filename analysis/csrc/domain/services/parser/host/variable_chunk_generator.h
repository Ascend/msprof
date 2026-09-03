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

#ifndef ANALYSIS_PARSER_VARIABLE_CHUNK_GENERATOR_H
#define ANALYSIS_PARSER_VARIABLE_CHUNK_GENERATOR_H

#include <deque>
#include <sstream>
#include <string>
#include <vector>

#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
// 按 MsprofVariableInfo 切变长记录：记录大小 = 24 字节公共头 + dataLen
class VariableChunkGenerator
{
   public:
    VariableChunkGenerator(const std::string &path, const std::vector<std::string> &filePrefix);
    virtual ~VariableChunkGenerator();

    int ReadChunk();
    Utils::CHAR_PTR Pop();
    uint32_t LastRecordSize() const;
    bool Empty() const;
    size_t Size() const;

   private:
    std::stringstream ss_;
    std::deque<std::string> readFiles_;
    size_t remainSize_ = 0;
    uint32_t lastRecordSize_ = 0;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_PARSER_VARIABLE_CHUNK_GENERATOR_H
