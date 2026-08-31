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

#ifndef ANALYSIS_PARSER_BASE_PARSER_H
#define ANALYSIS_PARSER_BASE_PARSER_H

#include <memory>
#include <string>
#include <vector>

#include "analysis/csrc/domain/services/parser/host/chunk_generator.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"

namespace Analysis
{
namespace Domain
{
enum class ParserStatus
{
    SUCCESS,
    NOT_EXIST,
    ERROR,
};

// 该类是数据解析的对外接口，提供接口ParseData()，解析不同类型数据并返回
// ParseData通过shared_ptr返回数据，数据生命周期由调用者持有
template <typename ParserType>
class BaseParser
{
   public:
    explicit BaseParser(std::string path, std::string parserName)
        : path_(std::move(path)), parserName_(std::move(parserName))
    {
    }

    template <typename T>
    std::vector<std::shared_ptr<T>> ParseData()
    {
        status_ = ParserStatus::ERROR;
        if (!chunkProducer_)
        {
            ERROR("%: The chunk producer is null.", parserName_);
            return {};
        }
        if (chunkProducer_->ReadChunk() != ANALYSIS_OK)
        {
            ERROR("%: Read Chunk failed.", parserName_);
            return {};
        }
        const bool hasInputData = !chunkProducer_->Empty();
        if (ProduceData() != ANALYSIS_OK)
        {
            ERROR("%: Format data failed.", parserName_);
            return {};
        }
        status_ = hasInputData ? ParserStatus::SUCCESS : ParserStatus::NOT_EXIST;
        return static_cast<ParserType*>(this)->template GetData<T>();
    }

    ParserStatus GetStatus() const { return status_; }

   protected:
    virtual int ProduceData() = 0;

   protected:
    std::string path_;
    std::string parserName_;
    std::shared_ptr<ChunkGenerator> chunkProducer_;
    ParserStatus status_ = ParserStatus::NOT_EXIST;
};  // class BaseParser
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_PARSER_BASE_PARSER_H
