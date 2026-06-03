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

#ifndef ANALYSIS_UTILS_TIME_UTILS_H
#define ANALYSIS_UTILS_TIME_UTILS_H

#include <cstdint>
#include <string>
#include "analysis/csrc/infrastructure/utils/hp_float.h"

namespace Analysis {
namespace Utils {
const double DEFAULT_FREQ = 1000.0;
// log中数据单位为US,换算成NS时需要*1000,此处为30分钟
const uint64_t DEFAULT_DURATION_TIME_NS = 30ULL * 60 * 1000 * 1000 * 1000; // 30min

// SyscntConversionParams 用于记录 syscnt 转为 timestamp 所需要的三个数据
// 分为host侧和device的syscnt两种情况，不同情况下，所需要的数据来源不同
// freq: 计算使用的频率, host使用 CPU的Frequency; device使用DeviceInfo的hwts_frequency
// hostFreq: host中CPU的Frequency;
// sysCnt: 基于开机时间的syscnt, host使用 host_start_log; device 使用 dev_start_log 中的cntvct
// hostCnt: 无论是host还是device默认都从host_start_log中取
// hostMonotonic: 基于开机时间的monotonic, 不论host还是device，固定使用host侧下的host_start_log 中的 clock_monotonic_raw
struct SyscntConversionParams {
    double freq = DEFAULT_FREQ;
    double hostFreq = DEFAULT_FREQ;
    uint64_t sysCnt = UINT64_MAX;
    uint64_t hostCnt = UINT64_MAX;
    uint64_t hostMonotonic = UINT64_MAX;
    SyscntConversionParams() = default;
    SyscntConversionParams(double freq, uint64_t sysCnt, uint64_t hostMonotonic)
        : freq(freq), sysCnt(sysCnt), hostMonotonic(hostMonotonic){};
};

struct ProfTimeRecord {
    uint64_t startTimeNs = UINT64_MAX;                                  // host start info 基于世界时间的任务开始时间
    uint64_t endTimeNs = 0;                                             // host end info 基于世界时间的任务结束时间
    uint64_t baseTimeNs = UINT64_MAX;                                   // host start info 基于开机时间的任务开始时间
    ProfTimeRecord() = default;
    ProfTimeRecord(uint64_t startTimeNs, uint64_t endTimeNs, uint64_t baseTimeNs)
        : startTimeNs(startTimeNs), endTimeNs(endTimeNs), baseTimeNs(baseTimeNs){};
};

// 转换成本地时间所需要的上下文参数
struct LocaltimeContext {
    uint16_t deviceId = UINT16_MAX;
    uint64_t hostMonotonic = UINT64_MAX;
    uint64_t deviceMonotonic = UINT64_MAX;
    Utils::ProfTimeRecord timeRecord;
};
// 时间转换
HPFloat GetTimeFromCnt(uint64_t sysCnt, uint64_t hostMonotonic, uint64_t referenceCnt, double frequency);
std::string GetFormatLocalTime();
// 将syscnt转为timestamp 返回结果为ns级
HPFloat GetTimeFromSyscnt(uint64_t syscnt, const SyscntConversionParams &params);
// 将syscnt转为timestamp 返回结果为ns级,从host侧获取
HPFloat GetTimeFromHostCnt(uint64_t syscnt, const SyscntConversionParams &params);
// 将以syscnt计算的duration转换成timestamp 返回结果为ns级
HPFloat GetDurTimeFromSyscnt(uint64_t syscnt, const SyscntConversionParams &params);
// 将采样时间戳转为host侧时间戳
HPFloat GetTimeBySamplingTimestamp(double timestamp, const uint64_t hostMonotonic, const uint64_t deviceMonotonic);
// 计算加上本地的偏移时间, ns级
HPFloat GetLocalTime(HPFloat &timestamp, const ProfTimeRecord &record);

}  // namespace Utils
}  // namespace Analysis

#endif // ANALYSIS_UTILS_TIME_UTILS_H
