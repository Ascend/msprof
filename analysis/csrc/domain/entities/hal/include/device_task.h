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

#ifndef MSPROF_ANALYSIS_DEVICE_TASK_H
#define MSPROF_ANALYSIS_DEVICE_TASK_H

#include <memory>

#include "analysis/csrc/domain/entities/hal/include/hal.h"
#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/entities/pmu/include/pmu_info.h"

namespace Analysis
{
namespace Domain
{

struct DeviceTask
{
    HalLogType logType = INVALID_LOG;
    uint32_t taskType = 0;
    uint64_t taskStart = 0;
    uint64_t taskEnd = 0;
    uint64_t sysEndLogTime = 0;
    uint32_t blockNum = 0;
    uint32_t mixBlockNum = 0;
    AcceleratorType acceleratorType = INVALID;
    std::unique_ptr<PmuBaseInfo> pmuInfo = nullptr;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // MSPROF_ANALYSIS_DEVICE_TASK_H
