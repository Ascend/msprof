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
#include "batch_id.h"

#include <climits>

#include "analysis/csrc/infrastructure/dfx/log.h"

namespace Analysis
{

namespace Domain
{

/*!
 * 计算任务中没有反转/翻转的下标
 * @param task : 任务信息数组
 * @param num : 任务信息数组个数
 * @param flipTimestamp : 反转/翻转的时刻
 * @return task下标，>=0，该下标及之前在flipTimestamp之前，-1，没有在flipTimestamp之前的，INT_MIN，空指针异常
 *         例如：[2,3,3,4,4]，flipTimestamp=3，返回的下标是2
 **/
int ModelingComputeBatchIdBinaryAlgo(HalUniData **task, uint32_t num, uint64_t flipTimestamp)
{
    int left = 0;
    int right = static_cast<int>(num - 1);
    int mid = 0;
    while (left <= right)
    {
        mid = (left + right) / 2;  // 2分查找

        if (task[mid] == nullptr)
        {
            ERROR("task[%] null!", mid);
            return INT_MIN;
        }

        if (task[mid]->timestamp <= flipTimestamp)
        {
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }

    return right;
}

/*!
 * 校准任务下标
 * @param task : 任务信息数组
 * @param num : 任务信息数组个数
 * @param cursor : 下标，<=的是否有小于入参taskId，如果有，虽然时间在taskId后，但是也是翻转的任务
 * @param flipTaskId : 翻转任务标识
 * @return 校准后的下标
 **/
int ModelingCalibrateBatchId(HalUniData **task, uint32_t taskNum, int cursor, uint16_t flipTaskId)
{
    if (static_cast<uint32_t>(cursor) >= taskNum)
    {
        return 0;
    }
    // 如果taskId为0xffff表示流正常结束销毁，可以不用往前找了。
    if (flipTaskId == 0 || flipTaskId == 0xffff)
    {
        return cursor;
    }
    while (cursor > 0)
    {
        if (task[cursor]->taskId.taskId > flipTaskId)
        {
            return cursor;
        }
        cursor--;
    }
    return cursor;
}

void ModelingComputeBatchIdBinary(HalUniData **task, uint32_t taskNum, HalUniData **flip, uint16_t flipNum)
{
    if (task == nullptr || taskNum == 0 || flip == nullptr || flipNum == 0)
    {
        ERROR("input error!");
        return;
    }

    uint32_t batchId;
    HalUniData **taskTmp = task;
    uint32_t taskTmpNum = taskNum;
    int cursor = 0;
    std::vector<int> cursorArray(flipNum + 1);
    int cursorNum = 0;

    for (batchId = 0; batchId < flipNum; batchId++)
    {
        if (taskTmpNum == 0)
        {
            break;
        }

        cursor = ModelingComputeBatchIdBinaryAlgo(taskTmp, taskTmpNum, flip[batchId]->timestamp);
        if (cursor == INT_MIN)
        {
            ERROR("error: null pointer!");
            return;
        }

        if (cursor >= 0)
        {
            int tmpCursor = cursor + taskTmp - task;
            // 修正往前找一找，如果task的taskId小于flip的taskId的也是翻转的
            tmpCursor = ModelingCalibrateBatchId(task, taskNum, tmpCursor, flip[batchId]->taskId.taskId);
            cursorArray[cursorNum] = tmpCursor;
            cursorNum++;
        }
        else
        {
            cursorArray[cursorNum] = cursor;
            cursorNum++;
        }

        taskTmp = taskTmp + cursor + 1;
        taskTmpNum = static_cast<uint32_t>(static_cast<int>(taskTmpNum) - cursor - 1);
    }

    if (taskTmpNum > 0)
    {
        cursorArray[cursorNum] = static_cast<int>(taskNum - 1);
        cursorNum++;
    }

    // 批量设置batch id
    int j = 0;
    for (int i = 0; i < cursorNum; i++)
    {
        for (; j <= cursorArray[i]; j++)
        {
            task[j]->taskId.batchId = i;
        }
    }
}

}  // namespace Domain

}  // namespace Analysis
