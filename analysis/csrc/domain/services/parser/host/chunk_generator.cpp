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

#include "analysis/csrc/domain/services/parser/host/chunk_generator.h"

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;

ChunkGenerator::ChunkGenerator(uint32_t chunkSize, const std::string &path, const std::vector<std::string> &filePrefix)
    : chunkSize_(chunkSize)
{
    auto files = File::GetOriginData(path, filePrefix, {"done", "complete"});
    files = File::SortFilesByAgingAndSliceNum(files);
    std::move(files.begin(), files.end(), back_inserter(readFiles_));
}

ChunkGenerator::~ChunkGenerator()
{
    readFiles_.clear();
    remainSize_ = 0;
    ss_.clear();
    ss_.str("");
}

int ChunkGenerator::ReadChunk()
{
    if (chunkSize_ == 0)
    {
        ERROR("Chunk size is invalid");
        return ANALYSIS_ERROR;
    }
    while (!readFiles_.empty())
    {
        auto file = readFiles_.front();
        readFiles_.pop_front();
        FileReader fd(file, std::ios::in | std::ios::binary);
        if (fd.ReadBinary(ss_) != ANALYSIS_OK)
        {
            ERROR("The read chunk binary failed: %", file);
            return ANALYSIS_ERROR;
        }
        auto fileSize = File::Size(file);
        if (fileSize % static_cast<uint64_t>(chunkSize_) != 0)
        {
            ERROR("The read chunk file size is invalid: %", file);
            return ANALYSIS_ERROR;
        }
        remainSize_ += fileSize / static_cast<uint64_t>(chunkSize_);
    }
    return ANALYSIS_OK;
}

CHAR_PTR ChunkGenerator::Pop()
{
    if (remainSize_ == 0)
    {
        WARN("Nothing remain to Pop.");
        return nullptr;
    }
    auto chunk = new (std::nothrow) char[chunkSize_];
    if (!chunk)
    {
        ERROR("New chunk failed.");
        return nullptr;
    }
    ss_.read(chunk, chunkSize_);
    --remainSize_;
    return chunk;
}

bool ChunkGenerator::Empty() const { return remainSize_ == 0; }

size_t ChunkGenerator::Size() const { return remainSize_; }
}  // namespace Domain
}  // namespace Analysis
