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
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
using namespace Analysis::Common;

namespace Analysis
{
namespace Domain
{

using namespace Infra;
using namespace Utils;
using DeviceTaskSummary = std::map<TaskId, std::vector<DeviceTask>>;
DeviceTaskSummary deviceTaskSummary;

uint32_t DeviceTaskProcess::ProcessEntry(DataInventory& dataInventory, const Infra::Context& context)
{
    const auto& deviceContext = dynamic_cast<const DeviceContext&>(context);
    std::string sqlitePath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE});
    if (!File::CreateDir(sqlitePath))
    {
        ERROR("Failed to create %. Please check that the path is accessible or the disk space is enough", sqlitePath);
        return ANALYSIS_ERROR;
    }
    std::shared_ptr<std::map<TaskId, std::vector<DeviceTask>>> data;
    MAKE_SHARED_RETURN_VALUE(data, DeviceTaskSummary, Analysis::ANALYSIS_ERROR, std::move(deviceTaskSummary));
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
