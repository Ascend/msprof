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

#include "analysis/csrc/domain/entities/metric/include/metric.h"

#include <map>

namespace Analysis
{
namespace Domain
{
namespace
{
template <typename T>
std::string LookupHeader(const std::map<T, std::string>& table, T type)
{
    auto it = table.find(type);
    return it != table.end() ? it->second : INVALID_HEADER;
}
}  // namespace

std::string Metric::GetMetricHeaderString(PipeUtilizationExctIndex type)
{
    static const std::map<PipeUtilizationExctIndex, std::string> table = {
        {PipeUtilizationExctIndex::MacTime, "mac_time"},
        {PipeUtilizationExctIndex::MacRatioExtra, "mac_ratio_extra"},
        {PipeUtilizationExctIndex::ScalarTime, "scalar_time"},
        {PipeUtilizationExctIndex::ScalarRatio, "scalar_ratio"},
        {PipeUtilizationExctIndex::Mte1Time, "mte1_time"},
        {PipeUtilizationExctIndex::Mte1RatioExtra, "mte1_ratio_extra"},
        {PipeUtilizationExctIndex::Mte2Time, "mte2_time"},
        {PipeUtilizationExctIndex::Mte2Ratio, "mte2_ratio"},
        {PipeUtilizationExctIndex::FixPipeTime, "fixpipe_time"},
        {PipeUtilizationExctIndex::FixPipeRatio, "fixpipe_ratio"},
        {PipeUtilizationExctIndex::ICacheMissRate, "icache_miss_rate"}};
    return LookupHeader(table, type);
}

std::string Metric::GetMetricHeaderString(ArithMetricIndex type)
{
    static const std::map<ArithMetricIndex, std::string> table = {{ArithMetricIndex::MacFp16Ratio, "mac_fp16_ratio"},
                                                                  {ArithMetricIndex::MacInt8Ratio, "mac_int8_ratio"},
                                                                  {ArithMetricIndex::VecFp32Ratio, "vec_fp32_ratio"},
                                                                  {ArithMetricIndex::VecFp16Ratio, "vec_fp16_ratio"},
                                                                  {ArithMetricIndex::VecInt32Ratio, "vec_int32_ratio"},
                                                                  {ArithMetricIndex::VecMiscRatio, "vec_misc_ratio"},
                                                                  {ArithMetricIndex::CubeFops, "cube_fops"},
                                                                  {ArithMetricIndex::VectorFops, "vector_fops"}};
    return LookupHeader(table, type);
}

std::string Metric::GetMetricHeaderString(PipeLineUtIndex type)
{
    static const std::map<PipeLineUtIndex, std::string> table = {{PipeLineUtIndex::VecTime, "vec_time"},
                                                                 {PipeLineUtIndex::VecRatio, "vec_ratio"},
                                                                 {PipeLineUtIndex::MacTime, "mac_time"},
                                                                 {PipeLineUtIndex::MacRatio, "mac_ratio"},
                                                                 {PipeLineUtIndex::ScalarTime, "scalar_time"},
                                                                 {PipeLineUtIndex::ScalarRatio, "scalar_ratio"},
                                                                 {PipeLineUtIndex::Mte1Time, "mte1_time"},
                                                                 {PipeLineUtIndex::Mte1Ratio, "mte1_ratio"},
                                                                 {PipeLineUtIndex::Mte2Time, "mte2_time"},
                                                                 {PipeLineUtIndex::Mte2Ratio, "mte2_ratio"},
                                                                 {PipeLineUtIndex::Mte3Time, "mte3_time"},
                                                                 {PipeLineUtIndex::Mte3Ratio, "mte3_ratio"},
                                                                 {PipeLineUtIndex::FixPipeTime, "fixpipe_time"},
                                                                 {PipeLineUtIndex::FixPipeRatio, "fixpipe_ratio"},
                                                                 {PipeLineUtIndex::ICacheMissRate, "icache_miss_rate"}};
    return LookupHeader(table, type);
}

std::string Metric::GetMetricHeaderString(MemoryIndex type)
{
    static const std::map<MemoryIndex, std::string> table = {{MemoryIndex::UBReadBw, "ub_read_bw"},
                                                             {MemoryIndex::UBWriteBw, "ub_write_bw"},
                                                             {MemoryIndex::L1ReadBw, "l1_read_bw"},
                                                             {MemoryIndex::L1WriteBw, "l1_write_bw"},
                                                             {MemoryIndex::MainMemReadBw, "main_mem_read_bw"},
                                                             {MemoryIndex::MainMemWriteBw, "main_mem_write_bw"},
                                                             {MemoryIndex::L2ReadBw, "l2_read_bw"},
                                                             {MemoryIndex::L2WriteBw, "l2_write_bw"}};
    return LookupHeader(table, type);
}

std::string Metric::GetMetricHeaderString(MemoryL0Index type)
{
    static const std::map<MemoryL0Index, std::string> table = {
        {MemoryL0Index::L0aReadBw, "l0a_read_bw"},          {MemoryL0Index::L0aWriteBw, "l0a_write_bw"},
        {MemoryL0Index::L0bReadBw, "l0b_read_bw"},          {MemoryL0Index::L0bWriteBw, "l0b_write_bw"},
        {MemoryL0Index::L0cReadBw, "l0c_read_bw"},          {MemoryL0Index::L0cWriteBw, "l0c_write_bw"},
        {MemoryL0Index::L0cReadBwCube, "l0c_read_bw_cube"}, {MemoryL0Index::L0cWriteBwCube, "l0c_write_bw_cube"}};
    return LookupHeader(table, type);
}

std::string Metric::GetMetricHeaderString(ResourceConflictIndex type)
{
    static const std::map<ResourceConflictIndex, std::string> table = {
        {ResourceConflictIndex::VecBankGroupCfltRatio, "vec_bankgroup_cflt_ratio"},
        {ResourceConflictIndex::VecBankCfltRatio, "vec_bank_cflt_ratio"},
        {ResourceConflictIndex::VecRescCfltRatio, "vec_resc_cflt_ratio"}};
    return LookupHeader(table, type);
}

std::string Metric::GetMetricHeaderString(MemoryUBIndex type)
{
    static const std::map<MemoryUBIndex, std::string> table = {{MemoryUBIndex::UbReadBwVector, "ub_read_bw_vector"},
                                                               {MemoryUBIndex::UbWriteBwVector, "ub_write_bw_vector"},
                                                               {MemoryUBIndex::UbReadBwScalar, "ub_read_bw_scalar"},
                                                               {MemoryUBIndex::UbWriteBwScalar, "ub_write_bw_scalar"},
                                                               {MemoryUBIndex::Fixp2UbWriteBw, "fixp2ub_write_bw"}};
    return LookupHeader(table, type);
}

std::string Metric::GetMetricHeaderString(L2CacheIndex type)
{
    static const std::map<L2CacheIndex, std::string> table = {
        // V4及之前芯片的L2Cache指标
        {L2CacheIndex::WriteCacheHit, "write_cache_hit"},
        {L2CacheIndex::WriteCacheMissAllocate, "write_cache_miss_allocate"},
        {L2CacheIndex::R0ReadCacheHit, "r0_read_cache_hit"},
        {L2CacheIndex::R0ReadCacheMissAllocate, "r0_read_cache_miss_allocate"},
        {L2CacheIndex::R1ReadCacheHit, "r1_read_cache_hit"},
        {L2CacheIndex::R1ReadCacheMissAllocate, "r1_read_cache_miss_allocate"},
        // V6芯片的L2Cache指标
        {L2CacheIndex::ReadLocalL2Hit, "read_local_l2_hit"},
        {L2CacheIndex::ReadLocalL2Miss, "read_local_l2_miss"},
        {L2CacheIndex::ReadLocalL2Victim, "read_local_l2_victim"},
        {L2CacheIndex::WriteLocalL2Hit, "write_local_l2_hit"},
        {L2CacheIndex::WriteLocalL2Miss, "write_local_l2_miss"},
        {L2CacheIndex::WriteLocalL2Victim, "write_local_l2_victim"}};
    return LookupHeader(table, type);
}

std::string Metric::GetMetricHeaderString(MemoryAccessIndex type)
{
    static const std::map<MemoryAccessIndex, std::string> table = {
        {MemoryAccessIndex::ReadMainMemoryData, "read_main_memory_datas"},
        {MemoryAccessIndex::WriteMainMemoryData, "write_main_memory_datas"},
        {MemoryAccessIndex::GmToL1Data, "GM_to_L1_datas"},
        {MemoryAccessIndex::L0CToL1Data, "L0C_to_L1_datas"},
        {MemoryAccessIndex::L0CToGmData, "L0C_to_GM_datas"},
        {MemoryAccessIndex::GmToUbData, "GM_to_UB_datas"},
        {MemoryAccessIndex::UbToGmData, "UB_to_GM_datas"}};
    return LookupHeader(table, type);
}
}  // namespace Domain
}  // namespace Analysis
