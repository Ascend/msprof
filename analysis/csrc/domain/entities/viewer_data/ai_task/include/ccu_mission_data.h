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

#include <stdint.h>

#include <string>

namespace Analysis
{
namespace Domain
{
const std::string CCU_TIME_TYPE_LOOP_GROUP = "LoopGroup";
const std::string CCU_TIME_TYPE_WAIT = "Wait";
const std::string CCU_TIME_TYPE_SETCKB = "SetCKB";

struct CCUInstructionArgs
{
    uint64_t dataSize = 0;
    uint64_t maxChannelDelay = 0;
    double bandwidth = 0.0;
    uint32_t mask = UINT32_MAX;
    uint16_t maxDelayChannel = UINT16_MAX;
    uint16_t dieId = UINT16_MAX;
    bool hasDieId = false;
    bool hasDataSize = false;
    bool hasBandwidth = false;
    bool hasReduceInfo = false;
    bool hasMask = false;
    bool hasDelayChannel = false;

    std::string reduceOpType;
    std::string inputDataType;
    std::string outputDataType;
};

struct CCUMissionTimelineData : public CCUInstructionArgs
{
    uint64_t timestamp = 0;
    double duration = 0.0;  // ns
    uint32_t taskId = UINT32_MAX;
    uint32_t notifyRankId = UINT32_MAX;
    uint16_t deviceId = UINT16_MAX;
    uint32_t streamId = UINT32_MAX;
    uint16_t instructionId = UINT16_MAX;
    std::string timeType;
};

struct CCUV62TimelineData : public CCUInstructionArgs
{
    uint64_t timestamp = 0;
    uint64_t waitEndSyscnt = 0;
    double duration = 0.0;
    int64_t setckbCkbinfo = 0;
    double longestDuration = 0.0;
    uint32_t taskId = UINT32_MAX;
    uint16_t deviceId = UINT16_MAX;
    uint16_t instructionId = UINT16_MAX;
    uint16_t earliestLoopId = UINT16_MAX;
    uint16_t latestLoopId = UINT16_MAX;
    uint16_t longestLoopId = UINT16_MAX;
    uint8_t lpFlg = UINT8_MAX;
    bool hasUdf = false;
    std::string timeType;
    std::string udfProfilingData;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_CCU_MISSION_DATA_H
