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

#ifndef ANALYSIS_PARSER_HOST_CANN_COMPACT_INFO_PARSER_H
#define ANALYSIS_PARSER_HOST_CANN_COMPACT_INFO_PARSER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/domain/services/adapter/flip.h"
#include "analysis/csrc/domain/services/parser/host/base_parser.h"
#include "analysis/csrc/infrastructure/utils/prof_common.h"

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{

// 该类的作用是Compact数据的解析
class CompactInfoParser : public BaseParser<CompactInfoParser>
{
   public:
    explicit CompactInfoParser(const std::string &path, const std::string &parserName) : BaseParser(path, parserName) {}
    void Init(const std::vector<std::string> &filePrefix);
    template <typename T>
    std::vector<std::shared_ptr<T>> GetData();

   protected:
    int ProduceData() override;

   protected:
    std::vector<std::shared_ptr<MsprofCompactInfo>> compactData_;   // not owned
    std::vector<std::shared_ptr<Adapter::FlipTask>> flipTaskData_;  // not owned
};  // class CompactInfoParser

// 该类的作用是node basic info数据的解析
class NodeBasicInfoParser final : public CompactInfoParser
{
   public:
    explicit NodeBasicInfoParser(const std::string &path) : CompactInfoParser(path, "NodeBasicInfoParser")
    {
        Init(filePrefix_);
    }

   private:
    int ProduceData() override;

   private:
    enum class OpType : uint8_t
    {
        STATIC_OP = 0,
        DYNAMIC_OP = 1,
    };
    std::vector<std::string> filePrefix_ = {
        "aging.compact.node_basic_info.slice",
    };
    std::vector<std::string> staticFilePrefix_ = {
        "unaging.compact.node_basic_info.slice",
    };
};  // class NodeBasicInfoParser

// 该类的作用是node attr info数据的解析
class NodeAttrInfoParser final : public CompactInfoParser
{
   public:
    explicit NodeAttrInfoParser(const std::string &path) : CompactInfoParser(path, "NodeAttrInfoParser")
    {
        Init(filePrefix_);
    }

   private:
    std::vector<std::string> filePrefix_ = {
        "unaging.compact.node_attr_info.slice",
        "aging.compact.node_attr_info.slice",
    };
};  // class NodeAttrInfoParser

// 该类的作用是memcpy info数据的解析
class MemcpyInfoParser final : public CompactInfoParser
{
   public:
    explicit MemcpyInfoParser(const std::string &path) : CompactInfoParser(path, "MemcpyInfoParser")
    {
        Init(filePrefix_);
    }

   private:
    std::vector<std::string> filePrefix_ = {
        "unaging.compact.memcpy_info.slice",
        "aging.compact.memcpy_info.slice",
    };
};  // class MemcpyInfoParser

// 该类的作用是task track数据的解析
class TaskTrackParser final : public CompactInfoParser
{
   public:
    explicit TaskTrackParser(const std::string &path) : CompactInfoParser(path, "TaskTrackParser")
    {
        Init(filePrefix_);
    }
    // Get DPU kernel name map: key = ((uint64_t)deviceId << 32) | taskId
    const std::unordered_map<uint64_t, uint64_t> &GetDpuKernelNameMap() const;

   private:
    int ProduceData() override;

   private:
    std::vector<std::string> filePrefix_ = {
        "unaging.compact.task_track.slice",
        "aging.compact.task_track.slice",
    };
    std::unordered_map<uint64_t, uint64_t> dpuKernelNameMap_;
};  // class TaskTrackParser

// 该类的作用是dpu task track数据的解析
class DpuTaskTrackParser final : public CompactInfoParser
{
   public:
    explicit DpuTaskTrackParser(const std::string &path) : CompactInfoParser(path, "DpuTaskTrackParser")
    {
        Init(filePrefix_);
    }

   private:
    std::vector<std::string> filePrefix_ = {
        "aging.compact.dpu_track.slice",
        "unaging.compact.dpu_track.slice",
    };
    std::vector<std::shared_ptr<MsprofCompactInfo>> dpuTrackData_;
};  // class DpuTaskTrackParser

// 该类的作用是hccl op info info数据的解析
class HcclOpInfoParser final : public CompactInfoParser
{
   public:
    explicit HcclOpInfoParser(const std::string &path) : CompactInfoParser(path, "HcclOpInfoParser")
    {
        Init(filePrefix_);
    }

   private:
    std::vector<std::string> filePrefix_ = {
        "unaging.compact.hccl_op_info.slice",
        "aging.compact.hccl_op_info.slice",
    };
};  // class HcclOpInfoParser
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_PARSER_HOST_CANN_COMPACT_INFO_PARSER_H
