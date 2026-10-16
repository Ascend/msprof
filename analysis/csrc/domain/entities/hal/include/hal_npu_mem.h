/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#ifndef MSPROF_ANALYSIS_HAL_NPU_MEM_H
#define MSPROF_ANALYSIS_HAL_NPU_MEM_H

#include <cstdint>

namespace Analysis
{
namespace Domain
{
struct HalNpuMemData
{
    uint32_t event{};
    uint64_t ddr{};
    uint64_t hbm{};
    uint64_t timestamp{};
};
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_HAL_NPU_MEM_H
