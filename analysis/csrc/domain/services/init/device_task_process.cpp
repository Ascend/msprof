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
#include "analysis/csrc/domain/services/init/include/device_task_process.h"

#include <map>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{

using namespace Infra;
using DeviceTaskSummary = std::map<TaskId, std::vector<DeviceTask>>;

uint32_t DeviceTaskProcess::ProcessEntry(DataInventory& dataInventory, const Infra::Context& context)
{
    (void)context;
    // 本流程不产出数据，只负责向下游提供一个空的 DeviceTask 容器：LoadHostData 会
    // GetPtr 到这个类型并就地填充，容器缺失时它直接报错返回（见 load_host_data.cpp
    // 的 ProcessEntry）。容器是就地填充的，因此这里每次都必须注入一个全新的空 map；
    // 不能改成静态/全局对象持有跨调用状态，否则下游写过的内容会漏给下一个 device，
    // 而且本流程在多 device 下是并发执行的，共享对象还会引入数据竞争。
    std::shared_ptr<std::map<TaskId, std::vector<DeviceTask>>> data;
    MAKE_SHARED_RETURN_VALUE(data, DeviceTaskSummary, Analysis::ANALYSIS_ERROR, DeviceTaskSummary{});
    if (dataInventory.Inject(data))
    {
        return Analysis::ANALYSIS_OK;
    }
    else
    {
        ERROR("Init DeviceTask failed");
        return Analysis::ANALYSIS_ERROR;
    }
}

REGISTER_PROCESS_SEQUENCE(DeviceTaskProcess, true);
REGISTER_PROCESS_SUPPORT_CHIP(DeviceTaskProcess, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
