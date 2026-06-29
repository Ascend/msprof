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

#include "analysis/csrc/infrastructure/utils/time_utils.h"

#include <ctime>
#include <iomanip>

#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
namespace Analysis
{
namespace Utils
{

const int YEAR_DISPLAY_WIDTH = 4;  // 年的固定宽度是4
const int TIME_DISPLAY_WIDTH = 2;  // 日期的固定宽度是2，不足的前面补零
const int NS_US = 3;               // NS到US的时间转换的数量级，3表示10^3的指数

/**
 * 通过host_start.log或者device_start.log参数计算时间
 * @param sysCnt
 * @param hostMonotonic
 * @param referenceCnt
 * @param frequency
 * @return
 */
HPFloat GetTimeFromCnt(uint64_t sysCnt, uint64_t hostMonotonic, uint64_t referenceCnt, double frequency)
{
    uint64_t timeDiff = (sysCnt >= referenceCnt) ? (sysCnt - referenceCnt) : (referenceCnt - sysCnt);
    HPFloat res = static_cast<double>(timeDiff) / frequency;
    res = res << NS_US;
    if (sysCnt >= referenceCnt)
    {
        res = HPFloat(hostMonotonic) + res;
    }
    else
    {
        res = HPFloat(hostMonotonic) - res;
    }
    return res;
}
std::string GetFormatLocalTime()
{
    time_t t = time(nullptr);  // 获取1970年1月1日0点0分0秒到现在经过的秒数
    struct tm localTm;
    struct tm *tm = localtime_r(&t, &localTm);

    std::stringstream sstr;

    // 将秒数转换为本地时间,年从1900算起,需要+1900,月为0-11,所以要+1
    sstr << std::setw(YEAR_DISPLAY_WIDTH) << std::setfill('0') << (tm->tm_year + 1900) << std::setw(TIME_DISPLAY_WIDTH)
         << std::setfill('0') << (tm->tm_mon + 1) << std::setw(TIME_DISPLAY_WIDTH) << std::setfill('0') << tm->tm_mday
         << std::setw(TIME_DISPLAY_WIDTH) << std::setfill('0') << tm->tm_hour << std::setw(TIME_DISPLAY_WIDTH)
         << std::setfill('0') << tm->tm_min << std::setw(TIME_DISPLAY_WIDTH) << std::setfill('0') << tm->tm_sec;

    // 格式：20231129134509  年月日时分秒的拼接字符串
    return sstr.str();
}
/**
 * host侧 sysCnt取host/host_start.log, freq取host/info.json->cpu->Frequency
 * device 侧 sysCnt取device/device_start.log, freq取host/info.json->hwts_frequency
 * @param syscnt
 * @param params
 * @return
 */
HPFloat GetTimeFromSyscnt(uint64_t syscnt, const SyscntConversionParams &params)
{
    return GetTimeFromCnt(syscnt, params.hostMonotonic, params.sysCnt, params.freq);
}

/**
 * 无论host侧还是device侧 hostCnt取host_start.log,hostFreq取info.json->cpu->Frequency
 * @param syscnt
 * @param params
 * @return
 */
HPFloat GetTimeFromHostCnt(uint64_t syscnt, const SyscntConversionParams &params)
{
    if (IsDoubleEqual(params.hostFreq, DEFAULT_FREQ))
    {
        // 当freq为默认值时,默认数据为 monotonic,直接返回即可
        return {syscnt};
    }
    return GetTimeFromCnt(syscnt, params.hostMonotonic, params.hostCnt, params.hostFreq);
}

HPFloat GetDurTimeFromSyscnt(uint64_t syscnt, const SyscntConversionParams &params)
{
    HPFloat res = syscnt / params.freq;
    res = res << NS_US;
    return res;
}

HPFloat GetTimeBySamplingTimestamp(double timestamp, const uint64_t hostMonotonic, const uint64_t deviceMonotonic)
{
    // 时间单位均为ns
    return HPFloat(timestamp) + HPFloat(hostMonotonic) - HPFloat(deviceMonotonic);
}

HPFloat GetLocalTime(HPFloat &timestamp, const ProfTimeRecord &record)
{
    HPFloat baseTime = record.startTimeNs - record.baseTimeNs;
    HPFloat res = timestamp + baseTime;
    return res;
}

}  // namespace Utils
}  // namespace Analysis
