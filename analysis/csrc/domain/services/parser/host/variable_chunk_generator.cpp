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

#include "analysis/csrc/domain/services/parser/host/variable_chunk_generator.h"

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/prof_struct.h"
#include "securec.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;

namespace
{
constexpr uint32_t kVariableHeaderSize =
    sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t);
}

VariableChunkGenerator::VariableChunkGenerator(const std::string &path, const std::vector<std::string> &filePrefix)
{
    auto files = File::GetOriginData(path, filePrefix, {"done", "complete"});
    files = File::SortFilesByAgingAndSliceNum(files);
    std::move(files.begin(), files.end(), back_inserter(readFiles_));
}

VariableChunkGenerator::~VariableChunkGenerator()
{
    readFiles_.clear();
    remainSize_ = 0;
    ss_.clear();
    ss_.str("");
}

int VariableChunkGenerator::ReadChunk()
{
    while (!readFiles_.empty())
    {
        auto file = readFiles_.front();
        readFiles_.pop_front();
        FileReader fd(file, std::ios::in | std::ios::binary);
        if (fd.ReadBinary(ss_) != ANALYSIS_OK)
        {
            ERROR("The read variable chunk binary failed: %", file);
            return ANALYSIS_ERROR;
        }
        remainSize_ += static_cast<size_t>(File::Size(file));
    }
    return ANALYSIS_OK;
}

CHAR_PTR VariableChunkGenerator::Pop()
{
    lastRecordSize_ = 0;
    if (remainSize_ < kVariableHeaderSize)
    {
        WARN("Nothing remain to Pop.");
        return nullptr;
    }
    MsprofVariableInfo header{};
    ss_.read(ReinterpretConvert<char *>(&header), kVariableHeaderSize);
    remainSize_ -= kVariableHeaderSize;
    const uint32_t recordSize = kVariableHeaderSize + header.dataLen;
    if (remainSize_ < header.dataLen)
    {
        ERROR("Variable record dataLen % exceeds remain size %.", header.dataLen, remainSize_);
        remainSize_ = 0;
        return nullptr;
    }
    auto chunk = new (std::nothrow) char[recordSize];
    if (!chunk)
    {
        ERROR("New variable chunk failed.");
        return nullptr;
    }
    if (memcpy_s(chunk, recordSize, &header, kVariableHeaderSize) != EOK)
    {
        ERROR("Copy variable header failed.");
        delete[] chunk;
        return nullptr;
    }
    if (header.dataLen > 0)
    {
        ss_.read(chunk + kVariableHeaderSize, header.dataLen);
        remainSize_ -= header.dataLen;
    }
    lastRecordSize_ = recordSize;
    return chunk;
}

uint32_t VariableChunkGenerator::LastRecordSize() const { return lastRecordSize_; }

bool VariableChunkGenerator::Empty() const { return remainSize_ < kVariableHeaderSize; }

size_t VariableChunkGenerator::Size() const { return remainSize_; }
}  // namespace Domain
}  // namespace Analysis
