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

#ifndef ANALYSIS_DOMAIN_ENTITIES_METRIC_METRIC_H
#define ANALYSIS_DOMAIN_ENTITIES_METRIC_METRIC_H

#include <string>

#include "analysis/csrc/infrastructure/dfx/log.h"

namespace Analysis
{
namespace Domain
{
const std::string INVALID_HEADER = "INVALID";

// PipeUtilizationExct对应910B芯片的AIV的PipeUtilization,两者属于等价分组
enum class PipeUtilizationExctIndex
{
    // 计算该分组的PMU时，以ratio/ratio_extra结尾的metric需要计算时间
    MacTime = 0,
    MacRatioExtra,
    ScalarTime,
    ScalarRatio,
    Mte1Time,
    Mte1RatioExtra,
    Mte2Time,
    Mte2Ratio,
    FixPipeTime,
    FixPipeRatio,
    ICacheMissRate,
};

enum class ArithMetricIndex
{
    MacFp16Ratio = 0,
    MacInt8Ratio,
    VecFp32Ratio,
    VecFp16Ratio,
    VecInt32Ratio,
    VecMiscRatio,
    CubeFops,
    VectorFops,
};

enum class PipeLineUtIndex
{
    // 计算该分组的PMU时，以ratio/ratio_extra结尾的metric需要计算时间
    VecTime = 0,
    VecRatio,
    MacTime,
    MacRatio,
    ScalarTime,
    ScalarRatio,
    Mte1Time,
    Mte1Ratio,
    Mte2Time,
    Mte2Ratio,
    Mte3Time,
    Mte3Ratio,
    FixPipeTime,
    FixPipeRatio,
    ICacheMissRate,
};

enum class MemoryIndex
{
    UBReadBw = 0,
    UBWriteBw,
    L1ReadBw,
    L1WriteBw,
    MainMemReadBw,
    MainMemWriteBw,
    L2ReadBw,
    L2WriteBw,
};

enum class MemoryL0Index
{
    L0aReadBw = 0,
    L0aWriteBw,
    L0bReadBw,
    L0bWriteBw,
    L0cReadBw,
    L0cWriteBw,
    L0cReadBwCube,
    L0cWriteBwCube,
};

enum class ResourceConflictIndex
{
    VecBankGroupCfltRatio = 0,
    VecBankCfltRatio,
    VecRescCfltRatio,
};

enum class MemoryUBIndex
{
    UbReadBwVector = 0,
    UbWriteBwVector,
    UbReadBwScalar,
    UbWriteBwScalar,
    Fixp2UbWriteBw,  // 仅v6支持
};

enum class L2CacheIndex
{
    WriteCacheHit = 0,
    WriteCacheMissAllocate,
    R0ReadCacheHit,
    R0ReadCacheMissAllocate,
    R1ReadCacheHit,
    R1ReadCacheMissAllocate,
    // 以下仅v6支持
    ReadLocalL2Hit,
    ReadLocalL2Miss,
    ReadLocalL2Victim,
    WriteLocalL2Hit,
    WriteLocalL2Miss,
    WriteLocalL2Victim,
};

enum class MemoryAccessIndex
{
    ReadMainMemoryData = 0,
    WriteMainMemoryData,
    GmToL1Data,
    L0CToL1Data,
    L0CToGmData,
    GmToUbData,
    UbToGmData,
};

class Metric
{
   public:
    // 各指标枚举与表头的绑定见metric.cpp，枚举项与表头字符串直接结对，与枚举数值无关
    static std::string GetMetricHeaderString(PipeUtilizationExctIndex type);
    static std::string GetMetricHeaderString(ArithMetricIndex type);
    static std::string GetMetricHeaderString(PipeLineUtIndex type);
    static std::string GetMetricHeaderString(MemoryIndex type);
    static std::string GetMetricHeaderString(MemoryL0Index type);
    static std::string GetMetricHeaderString(ResourceConflictIndex type);
    static std::string GetMetricHeaderString(MemoryUBIndex type);
    static std::string GetMetricHeaderString(L2CacheIndex type);
    static std::string GetMetricHeaderString(MemoryAccessIndex type);

    // 兜底：未绑定表头的枚举类型
    template <typename T>
    static std::string GetMetricHeaderString(T)
    {
        ERROR("Invalid enumType");
        return INVALID_HEADER;
    }
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_ENTITIES_METRIC_METRIC_H
