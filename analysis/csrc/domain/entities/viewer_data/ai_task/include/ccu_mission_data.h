/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#ifndef ANALYSIS_DOMAIN_CCU_MISSION_DATA_H
#define ANALYSIS_DOMAIN_CCU_MISSION_DATA_H

#include <string>
#include <stdint.h>

namespace Analysis {
namespace Domain {
const std::string CCU_TIME_TYPE_LOOP_GROUP = "LoopGroup";
const std::string CCU_TIME_TYPE_WAIT = "Wait";

struct CCUMissionTimelineData {
    uint64_t timestamp = 0;
    uint64_t dataSize = 0;
    uint64_t maxChannelDelay = 0;
    double duration = 0.0; // ns
    double bandwidth = 0.0;
    uint32_t taskId = UINT32_MAX;
    uint32_t notifyRankId = UINT32_MAX;
    uint32_t mask = UINT32_MAX;
    uint16_t deviceId = UINT16_MAX;
    uint16_t streamId = UINT16_MAX;
    uint16_t instructionId = UINT16_MAX;
    uint16_t maxDelayChannel = UINT16_MAX;
    uint16_t dieId = UINT16_MAX;
    bool hasDieId = false;
    bool hasDataSize = false;
    bool hasBandwidth = false;
    bool hasReduceInfo = false;
    bool hasMask = false;
    bool hasDelayChannel = false;

    std::string timeType;
    std::string reduceOpType;
    std::string inputDataType;
    std::string outputDataType;
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_CCU_MISSION_DATA_H

