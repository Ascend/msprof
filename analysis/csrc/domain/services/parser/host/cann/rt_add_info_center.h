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
 * -------------------------------------------------------------------------
 */

#ifndef ANALYSIS_PARSER_HOST_CANN_RT_ADD_INFO_CENTER_H
#define ANALYSIS_PARSER_HOST_CANN_RT_ADD_INFO_CENTER_H

#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/ascend_obj.h"
#include "analysis/csrc/infrastructure/utils/singleton.h"

class RTAddInfoCenterUTest;

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{

using CaptureKey = std::tuple<uint16_t, uint32_t, uint32_t>;

using TimeRangeInfo = std::tuple<uint64_t, uint64_t, uint64_t>;

static constexpr uint64_t DEFAULT_MODEL_ID = UINT32_MAX;

static constexpr uint16_t CAPTURE_STATUS_START = 0;

static constexpr uint16_t CAPTURE_STATUS_END = 1;

// 该类是runtime算子补充信息数据单例类
class RTAddInfoCenter : public Utils::Singleton<RTAddInfoCenter>
{
    friend class ::RTAddInfoCenterUTest;

   public:
    void Load(const std::string &path);
    void SetCaptureStreamInfoData(const std::vector<CaptureStreamInfo> &data);
    void Add(const RuntimeOpInfo &info);
    RuntimeOpInfo Get(uint16_t deviceId, uint32_t streamId, uint32_t taskId);
    const std::unordered_map<std::string, RuntimeOpInfo> &GetAll() const;
    const std::vector<RuntimeOpInfo> &GetDumpList() const;
    bool Empty() const;
    bool LoadedFromBinary() const;
    uint64_t GetModelId(uint16_t deviceId, uint32_t streamId, uint32_t batchId, uint64_t timestamp);

   private:
    void LoadDB(const std::string &path);
    void BuildCaptureInfoTimeRange();
    std::unordered_map<std::string, RuntimeOpInfo> runtimeOpInfoData_;
    std::vector<RuntimeOpInfo> dumpList_;
    std::vector<CaptureStreamInfo> captureStreamInfoData_;
    std::map<CaptureKey, TimeRangeInfo> captureInfoTimeRangeDict_;
    bool loadedFromBinary_ = false;
};  // class RTAddInfoCenter
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_PARSER_HOST_CANN_RT_ADD_INFO_CENTER_H
