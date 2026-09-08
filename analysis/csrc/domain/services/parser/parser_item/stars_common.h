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

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_STARS_COMMON_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_STARS_COMMON_H

#include <cstdint>

/*
 * 在2023年11月 task id底层存在规则变动，取用了stream id中某位作为标记位，基于标记位进行新的task
 * id计算，具体计算规则为： 1、判断stream id的第13位是否为0，为0则直接返回task id，否则 2、取task id低13位，与stream
 * id高3位，组成一个完整taskId
 *
 * 2024年7月 task id规则变动，新增stream id标记位，扩展新的task id计算规则，变更后如下：
 * 1、判断stream id的第13位是否为0
 * 2、不为0，则取task id低13位，与stream id高3位，组成一个完整taskId
 * 3、为0，则继续判断stream id第14位是否为0，是则直接返回task id；不为0 则交换stream id和task id的低12位
 */

namespace Analysis
{
namespace Domain
{
const uint16_t STREAM_LOW_OPERATOR = 1u << 11;
const uint16_t STREAM_JUDGE_BIT12_OPERATOR = 0x1000;
const uint16_t STREAM_JUDGE_BIT13_OPERATOR = 0x2000;
const uint16_t STREAM_JUDGE_BIT15_OPERATOR = 0x8000;
const uint16_t TASK_LOW_OPERATOR = 0x1FFF;
const uint16_t STREAM_HIGH_OPERATOR = 0xE000;
const uint16_t COMMON_LOW_OPERATOR = 0x0FFF;
const uint16_t COMMON_HIGH_OPERATOR = 0xF000;
const uint16_t EXPANDING_LOW_OPERATOR = 0x7FFF;
const uint16_t PLACE_HOLDER_SQE = 3;
const uint16_t EVENT_RECORD_SQE = 4;

class StarsCommon
{
   public:
    static uint16_t GetStreamId(uint16_t streamId, uint16_t taskId, uint16_t expandStatus, uint16_t sqeType = 0)
    {
        if (expandStatus == 1)
        {
            if (sqeType == PLACE_HOLDER_SQE || sqeType == EVENT_RECORD_SQE)
            {
                return streamId & EXPANDING_LOW_OPERATOR;
            }
            if ((streamId & STREAM_JUDGE_BIT15_OPERATOR) != 0)
            {
                return taskId & EXPANDING_LOW_OPERATOR;
            }
            else
            {
                return streamId & EXPANDING_LOW_OPERATOR;
            }
        }
        else
        {
            if ((streamId & STREAM_JUDGE_BIT12_OPERATOR) != 0)
            {
                return streamId % STREAM_LOW_OPERATOR;
            }
            if ((streamId & STREAM_JUDGE_BIT13_OPERATOR) != 0)
            {
                streamId = taskId & COMMON_LOW_OPERATOR;
            }
            auto res = streamId % STREAM_LOW_OPERATOR;
            return res;
        }
    }

    static uint16_t GetTaskId(uint16_t streamId, uint16_t taskId, uint16_t expandStatus, uint16_t sqeType = 0)
    {
        if (expandStatus == 1)
        {
            if (sqeType == PLACE_HOLDER_SQE || sqeType == EVENT_RECORD_SQE)
            {
                return taskId;
            }
            if ((streamId & STREAM_JUDGE_BIT15_OPERATOR) != 0)
            {
                return (taskId & STREAM_JUDGE_BIT15_OPERATOR) | (streamId & EXPANDING_LOW_OPERATOR);
            }
            else
            {
                return taskId;
            }
        }
        else
        {
            if ((streamId & STREAM_JUDGE_BIT12_OPERATOR) != 0)
            {
                taskId = taskId & TASK_LOW_OPERATOR;
                taskId |= (streamId & STREAM_HIGH_OPERATOR);
            }
            else if ((streamId & STREAM_JUDGE_BIT13_OPERATOR) != 0)
            {
                taskId = (streamId & COMMON_LOW_OPERATOR) | (taskId & COMMON_HIGH_OPERATOR);
            }
            return taskId;
        }
    }
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_STARS_COMMON_H
