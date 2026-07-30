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

#ifndef MSPROF_ANALYSIS_AICPU
#define MSPROF_ANALYSIS_AICPU

#include <cstdint>
#include <string>

#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/infrastructure/utils/prof_common.h"

namespace Analysis
{
namespace Domain
{
enum class AicpuType
{
    AICPU_NODE = 0,
    AICPU_DP,
    AICPU_MODEL,
    AICPU_MI,
    KFC_COMM_TURN,
    KFC_COMPUTE_TURN,
    HCCL_OP_INFO = 10,
    AICPU_FLIP_TASK,
    AICPU_MASTER_STREAM_HCCL_TASK,
    KFC_HCCL_INFO
};

struct AicpuData
{
    uint64_t timeStamp;
    AicpuType type;
    TaskId taskId;
    TaskId aicpuTaskId;
    union
    {
        MsprofAicpuNodeAdditionalData node;
        MsprofAicpuDpAdditionalData dp;
        MsprofAicpuModelAdditionalData model;
        MsprofAicpuMiAdditionalData mi;
        AicpuKfcProfCommTurn commTurn;
        AicpuKfcProfComputeTurn computeTurn;
        MsprofAicpuHCCLOPInfo opInfo;
        MsporfAicpuFlipTask flipTask;
        MsprofAicpuHcclMainStreamTask mainStreamTask;
        MsprofKfcInfos KfcInfos;
    };
    AicpuData() {}
};
}  // namespace Domain
}  // namespace Analysis
#endif  // MSPROF_ANALYSIS_AICPU
