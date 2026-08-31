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

#ifndef ANALYSIS_PARSER_HOST_CANN_ADDITION_INFO_PARSER_H
#define ANALYSIS_PARSER_HOST_CANN_ADDITION_INFO_PARSER_H

#include <string>
#include <vector>

#include "analysis/csrc/domain/services/parser/host/base_parser.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"
#include "analysis/csrc/infrastructure/utils/prof_struct.h"

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{
// 该类的作用是Addition数据的解析
class AdditionInfoParser : public BaseParser<AdditionInfoParser>
{
   public:
    explicit AdditionInfoParser(const std::string &path, const std::string &parserName) : BaseParser(path, parserName)
    {
    }
    void Init(const std::vector<std::string> &filePrefix);
    template <typename T>
    std::vector<std::shared_ptr<T>> GetData();
    AdditionalInfoFormat parserType_ = AdditionalInfoFormat::ADDITIONAL_INFO_TYPE;

   protected:
    int ProduceData() override;
    virtual bool IsDataValid(const MsprofAdditionalInfo &additionalInfo) const;

   protected:
    std::vector<std::shared_ptr<ParserAdditionalInfo>> additionalData_;      // not owned
    std::vector<std::shared_ptr<ParserConcatTensorInfo>> concatTensorData_;  // not owned
};  // class AdditionInfoParser

// 该类的作用是CtxId数据的解析
class CtxIdParser final : public AdditionInfoParser
{
   public:
    explicit CtxIdParser(const std::string &path) : AdditionInfoParser(path, "CtxIdParser")
    {
        parserType_ = AdditionalInfoFormat::CONTEXT_ID_INFO_TYPE;
        Init(filePrefix_);
    }

   private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.context_id_info.slice",
        "aging.additional.context_id_info.slice",
    };
};  // class CtxIdParser

// 该类的作用是fusion op数据的解析
class FusionOpInfoParser final : public AdditionInfoParser
{
   public:
    explicit FusionOpInfoParser(const std::string &path) : AdditionInfoParser(path, "FusionOpInfoParser")
    {
        parserType_ = AdditionalInfoFormat::FUSION_OP_INFO_TYPE;
        Init(filePrefix_);
    }

   private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.fusion_op_info.slice",
        "aging.additional.fusion_op_info.slice",
    };
};  // class FusionOpInfoParser

// 该类的作用是graph_id_map数据的解析
class GraphIdParser final : public AdditionInfoParser
{
   public:
    explicit GraphIdParser(const std::string &path) : AdditionInfoParser(path, "GraphIdParser")
    {
        parserType_ = AdditionalInfoFormat::GRAPH_ID_TYPE;
        Init(filePrefix_);
    }

   private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.graph_id_map.slice",
        "aging.additional.graph_id_map.slice",
    };
};  // class GraphIdParser

// 该类的作用是tensor info数据的解析
class TensorInfoParser final : public AdditionInfoParser
{
   public:
    explicit TensorInfoParser(const std::string &path) : AdditionInfoParser(path, "TensorInfoParser")
    {
        parserType_ = AdditionalInfoFormat::TENSOR_INFO_TYPE;
        Init(filePrefix_);
    }

   private:
    int ProduceData() override;

   private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.tensor_info.slice",
        "aging.additional.tensor_info.slice",
    };
};  // class TensorInfoParser

// 该类的作用是hccl info数据的解析
class HcclInfoParser final : public AdditionInfoParser
{
   public:
    explicit HcclInfoParser(const std::string &path) : AdditionInfoParser(path, "HcclInfoParser")
    {
        parserType_ = AdditionalInfoFormat::HCCL_INFO_TYPE;
        Init(filePrefix_);
    }

   private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.hccl_info.slice",
        "aging.additional.hccl_info.slice",
    };
};  // class HcclInfoParser

// 该类的作用是memory application数据的解析
class MemoryApplicationParser final : public AdditionInfoParser
{
   public:
    explicit MemoryApplicationParser(const std::string &path) : AdditionInfoParser(path, "MemoryApplicationParser")
    {
        parserType_ = AdditionalInfoFormat::MEMORY_APPLICATION_TYPE;
        Init(filePrefix_);
    }

   private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.memory_application.slice",
        "aging.additional.memory_application.slice",
    };
};  // class MemoryApplicationParser

// 该类的作用是解析NPU算子内存数据
class TaskMemoryParser final : public AdditionInfoParser
{
   public:
    explicit TaskMemoryParser(const std::string &path) : AdditionInfoParser(path, "TaskMemoryParser")
    {
        parserType_ = AdditionalInfoFormat::TASK_MEMORY_INFO_TYPE;
        Init(filePrefix_);
    }

   private:
    bool IsDataValid(const MsprofAdditionalInfo &additionalInfo) const override;

   private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.task_memory_info.slice",
        "aging.additional.task_memory_info.slice",
    };
};  // class TaskMemoryParser

// 该类的作用是multi thread数据的解析
class MultiThreadParser final : public AdditionInfoParser
{
   public:
    explicit MultiThreadParser(const std::string &path) : AdditionInfoParser(path, "MultiThreadParser")
    {
        parserType_ = AdditionalInfoFormat::MULTI_THREAD_TYPE;
        Init(filePrefix_);
    }

   private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.Multi_Thread.slice",
        "aging.additional.Multi_Thread.slice",
    };
};  // class MultiThreadParser
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_PARSER_HOST_CANN_ADDITION_INFO_PARSER_H
