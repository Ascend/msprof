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

#ifndef ANALYSIS_DOMAIN_ENTITIES_HOST_NPU_OP_MEM_H
#define ANALYSIS_DOMAIN_ENTITIES_HOST_NPU_OP_MEM_H

#include <cstdint>
#include <string>
#include <vector>

namespace Analysis
{
namespace Domain
{
namespace Host
{

struct NpuOpMemRecordData
{
    std::string component = "GE";
    uint64_t timestamp = 0;
    uint64_t totalReserveMemory = 0;
    uint64_t totalAllocateMemory = 0;
    std::string deviceType;
};

struct NpuOpMemLifecycleData
{
    uint64_t operatorId = 0;
    int64_t size = 0;
    uint64_t allocationTime = 0;
    uint64_t releaseTime = 0;
    int64_t duration = 0;
    uint64_t allocationTotalAllocated = 0;
    uint64_t allocationTotalReserved = 0;
    uint64_t releaseTotalAllocated = 0;
    uint64_t releaseTotalReserved = 0;
    std::string deviceType;
    std::string name;
};

struct NpuOpMemCalculationResult
{
    std::vector<NpuOpMemRecordData> memoryRecords;
    std::vector<NpuOpMemLifecycleData> operatorMemory;
};

}  // namespace Host
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_ENTITIES_HOST_NPU_OP_MEM_H
