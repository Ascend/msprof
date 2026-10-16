/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#ifndef MSPROF_ANALYSIS_HAL_NPU_MODULE_MEM_H
#define MSPROF_ANALYSIS_HAL_NPU_MODULE_MEM_H

#include <cstdint>
#include <string>

namespace Analysis
{
namespace Domain
{
struct HalNpuModuleMemData
{
    uint32_t moduleId{};
    uint64_t syscnt{};
    uint64_t totalSize{};
    std::string deviceType;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_HAL_NPU_MODULE_MEM_H
