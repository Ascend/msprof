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

#include "type_data.h"

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{
using namespace Utils;
namespace
{
const std::unordered_map<uint64_t, std::string> whiteTypeData{{27, "DYNAMIC_ENABLE"}, {28, "DYNAMIC_DISABLE"}};
}

bool TypeData::Load(const std::string &path)
{
    auto files = File::GetOriginData(path, filePrefix_, fileFilter_);
    if (files.empty())
    {
        WARN("No type data found.");
        return false;
    }
    return ReadFiles(files);
}

std::string TypeData::Get(uint16_t level, uint64_t type)
{
    if (typeData_.find(level) == typeData_.end())
    {
        ERROR("Failed to found str for level: %", level);
        return std::to_string(type);
    }
    if (typeData_[level].find(type) == typeData_[level].end())
    {
        auto it = whiteTypeData.find(type);
        if (it != whiteTypeData.end())
        {
            return it->second;
        }
        ERROR("Failed to found str for type: %", type);
        return std::to_string(type);
    }
    return typeData_[level][type];
}

std::unordered_map<uint16_t, std::unordered_map<uint64_t, std::string>> &TypeData::GetAll() { return typeData_; }

bool TypeData::ReadFiles(const std::vector<std::string> &files)
{
    const int expectTokenSize = 2;       // 2代表：前后的key和value
    const int expectInnerTokenSize = 2;  // 2代表_前后的level和type
    for (auto &file : files)
    {
        FileReader reader(file);
        std::vector<std::string> text;
        auto status = reader.ReadText(text);
        if (status != ANALYSIS_OK)
        {
            ERROR("Read txt from % error", file);
            continue;
        }
        for (const auto &line : text)
        {
            const int splitPosition = 1;
            auto tokens = Utils::Split(line, ":", splitPosition);
            if (tokens.size() != expectTokenSize)
            {
                ERROR("Found illegal type line: %", line);
                continue;
            }
            auto typeTokens = Utils::Split(tokens[0], "_");
            if (typeTokens.size() != expectInnerTokenSize)
            {
                ERROR("Found illegal type key: %", tokens[0]);
                continue;
            }
            uint16_t level = 0;
            uint64_t type = 0;
            status = Utils::StrToU16(level, typeTokens[0]);
            if (status != ANALYSIS_OK)
            {
                ERROR("Convert str % to uint16 error", typeTokens[0]);
                continue;
            }
            status = Utils::StrToU64(type, typeTokens[1]);
            if (status != ANALYSIS_OK)
            {
                ERROR("Convert str % to uint64 error", typeTokens[1]);
                continue;
            }
            if (typeData_.find(level) != typeData_.end() and typeData_[level].find(type) != typeData_[level].end())
            {
                // 采集侧修复后该行变为ERROR
                WARN("Duplicate type found level: %, type: %", level, type);
            }
            typeData_[level][type] = tokens[1];
        }
    }
    return true;
}

void TypeData::Clear() const { typeData_.clear(); }
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis
