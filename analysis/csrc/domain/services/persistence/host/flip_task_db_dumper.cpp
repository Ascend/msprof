/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include "analysis/csrc/domain/services/persistence/host/flip_task_db_dumper.h"

namespace Analysis
{
namespace Domain
{
FlipTaskDBDumper::FlipTaskDBDumper(const std::string &hostFilePath)
    : BaseDumper<FlipTaskDBDumper>(hostFilePath, "HostTaskFlip")
{
    MAKE_SHARED0_NO_OPERATION(database_, RtsTrackDB);
}

FlipTaskData FlipTaskDBDumper::GenerateData(const FlipTasks &flipTasks)
{
    FlipTaskData data;
    if (Utils::Reserve(data, flipTasks.size()))
    {
        for (auto &flipTask : flipTasks)
        {
            data.emplace_back(flipTask->streamId, flipTask->timeStamp, flipTask->taskId, flipTask->flipNum);
        }
    }
    return data;
}
}  // namespace Domain
}  // namespace Analysis
