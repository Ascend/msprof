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

#ifndef MSPROF_ANALYSIS_HAL_TRACK_H
#define MSPROF_ANALYSIS_HAL_TRACK_H

#include <unordered_map>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal.h"

namespace Analysis
{
namespace Domain
{
// index_id, model_id, step_start, step_end, iter_id
using StepTraceDataVectorFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;

enum HalTrackType
{
    TS_TASK_FLIP = 0,
    STEP_TRACE = 1,
    TS_TASK_TYPE = 2,
    TS_MEMCPY = 3,
    BLOCK_NUM = 4,
    INVALID_TYPE = 5
};

struct HalTaskFlip
{
    uint64_t timestamp = 0;
    uint16_t flipNum = 0;
};

struct HalStepTrace
{
    uint64_t timestamp = 0;
    uint64_t indexId = 0;
    uint64_t modelId = 0;
    uint16_t tagId = 0;
};

struct HalTaskType
{
    uint64_t taskType = 0;
    uint16_t taskStatus = 0;
};

struct HalTaskMemcpy
{
    uint64_t taskStatus = 0;
};

struct HalBlockNum
{
    uint64_t timestamp = 0;
    uint32_t blockNum = 0;
};

struct HalTrackData
{
    HalUniData hd;
    HalTrackType type{INVALID_TYPE};
    union
    {
        HalTaskFlip flip;
        HalStepTrace stepTrace;
        HalTaskType taskType;
        HalTaskMemcpy taskMemcpy;
        HalBlockNum blockNum;
    };
    HalTrackData() {};
};

std::unordered_map<uint32_t, std::vector<HalTrackData*>> GetFlipData(std::vector<HalTrackData>& trackData);

/**
 * 引用类型跟踪数据，获取指针存于数组中
 * @param trackData 数据数组
 * @param type 类型
 * @return 返回指定类型的数据的指针数组
 */
std::vector<HalTrackData> GetTrackDataByType(std::vector<HalTrackData>& trackData, HalTrackType type);
StepTraceDataVectorFormat GenerateStepTime(std::vector<HalTrackData>& trackDatas);

}  // namespace Domain
}  // namespace Analysis
#endif  // MSPROF_ANALYSIS_HAL_TRACK_H
