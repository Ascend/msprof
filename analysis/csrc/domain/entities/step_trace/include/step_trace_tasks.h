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

#ifndef MSPROF_ANALYSIS_STEP_TRACE_TASKS_H
#define MSPROF_ANALYSIS_STEP_TRACE_TASKS_H

#include <vector>
#include <map>
#include <stdint.h>

namespace Analysis {
namespace Domain {
struct TimePair {
    uint64_t start = 0;
    uint64_t end = 0;
};

// 记录一个完整迭代内事件信息
struct StepTraceTasks {
    uint32_t indexId = 0;
    uint32_t iterId = 0;
    TimePair stepTrace;
    std::map<uint16_t, std::vector<TimePair>> allReduceTable;  // key 为 streamId
    std::map<uint16_t, std::vector<TimePair>> getNextTable; // key 为 streamId
    std::vector<TimePair> trainingTrace;
};

}
}

#endif // MSPROF_ANALYSIS_STEP_TRACE_TASKS_H
