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

#ifndef ANALYSIS_PARSER_ENVIRONMENT_CONTEXT_H
#define ANALYSIS_PARSER_ENVIRONMENT_CONTEXT_H

#include <map>
#include <set>
#include <string>
#include <unordered_map>

#include "analysis/csrc/infrastructure/utils/singleton.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"
#include "opensource/json/include/nlohmann/json.hpp"

namespace Analysis
{
namespace Domain
{
namespace Environment
{
const uint16_t HOST_ID = 64;
// UINT16_MAX为非法device id, HOST_ID + 1 为默认device id
const uint16_t INVALID_DEVICE_ID = UINT16_MAX;
const uint16_t DEFAULT_DEVICE_ID = HOST_ID + 1;
enum class Chip : uint16_t
{
    CHIP_V1_1_0 = 0,
    CHIP_V2_1_0 = 1,
    CHIP_V3_1_0 = 2,
    CHIP_V3_2_0 = 3,
    CHIP_V3_3_0 = 4,
    CHIP_V4_1_0 = 5,
    CHIP_V1_1_1 = 7,
    CHIP_V1_1_2 = 8,
    CHIP_V5_1_0 = 9,
    CHIP_V1_1_3 = 11,
    CHIP_V6_1_0 = 15,
    CHIP_V6_2_0 = 16
};
// 该类是Context信息单例类，读取device(host)路径下json/log文件及环境信息
// 通过 std::unordered_map<std::string, std::unordered_map<uint16_t, nlohmann::json>> 结构的成员变量context_
// 以prof, deviceId两层进行数据路径分割，将该device目录下的对应json和log进行key值合并，统一整合为一份json对象
// 数据查询以prof（无prof则默认为begin()）和deviceId（必选）进行查找
class Context : public Utils::Singleton<Context>
{
   public:
    bool Load(const std::set<std::string> &profPaths);
    bool IsAllExport();
    void Clear();

   public:
    // 获取start_info end_info中的时间
    bool GetProfTimeRecordInfo(Utils::ProfTimeRecord &record, const std::string &profPath = "",
                               uint16_t deviceId = HOST_ID);
    // 返回info.json 中的pid
    uint32_t GetPidFromInfoJson(uint16_t deviceId = DEFAULT_DEVICE_ID, const std::string &profPath = "");
    // 返回info.json中的pid_name
    std::string GetPidNameFromInfoJson(uint16_t deviceId = DEFAULT_DEVICE_ID, const std::string &profPath = "");
    // 返回samplejson.json 中的msprofBinPid
    int64_t GetMsBinPid(const std::string &profPath);
    std::string GetLLCProfiling(uint16_t deviceId, const std::string &profPath);
    // 获取start_log中的相关时间
    bool GetSyscntConversionParams(Utils::SyscntConversionParams &params, uint16_t deviceId = DEFAULT_DEVICE_ID,
                                   const std::string &profPath = "");
    // 获取start_log中的clock_monotonic_raw
    bool GetClockMonotonicRaw(uint64_t &monotonicRaw, bool isHost, uint16_t deviceId = DEFAULT_DEVICE_ID,
                              const std::string &profPath = "");
    // aic和aiv的freq的值在采集时使用同一个值,即两者频率始终保持一致,这里只取aic_frequency
    // 该频率只在device侧能取到
    bool GetPmuFreq(double &freq, uint16_t deviceId = DEFAULT_DEVICE_ID, const std::string &profPath = "");
    // 采集开关中不支持单独设置aiv-mode,因此ai_core_profiling_mode和aiv_profiling_mode应保持一致,这里使用ai_core_profiling_mode
    bool GetMetricMode(std::string &metricMode, const std::string &profPath = "");
    // 获取info.json中的hostUid
    std::string GetHostUid(uint16_t deviceId = DEFAULT_DEVICE_ID, const std::string &profPath = "");
    // 获取info.json中的hostname
    std::string GetHostName(uint16_t deviceId = DEFAULT_DEVICE_ID, const std::string &profPath = "");
    // 获取sample.json中的qosEvents
    std::vector<std::string> GetQosEvents(uint16_t deviceId = DEFAULT_DEVICE_ID, const std::string &profPath = "");
    // 获取ai_core_num
    uint16_t GetAiCoreNum(uint16_t deviceId, const std::string &profPath);
    uint64_t GetTotalMem(uint16_t deviceId, const std::string &profPath);
    uint64_t GetNetCardTotalSpeed(uint16_t deviceId, const std::string &profPath);
    bool IsLevel0(const std::string &profPath);
    // 获取cann version的major version和minor version
    std::vector<uint16_t> GetCannVersion(uint16_t deviceId, const std::string &profPath);

   public:
    // 获取对应device的芯片型号
    uint16_t GetPlatformVersion(uint16_t deviceId = DEFAULT_DEVICE_ID, const std::string &profPath = "");
    // 判断芯片类型
    static bool IsStarsChip(uint16_t platformVersion);
    // 校验是否为CHIP_V1_1_x系列(不包含CHIP_V1_1_0)
    static bool IsChipV1(uint16_t platformVersion);
    static bool IsChipV4(uint16_t platformVersion);
    static bool IsChipV6(uint16_t platformVersion);
    // 校验是否为CHIP_V1_1_0
    static bool IsFirstChipV1(uint16_t platformVersion);

   private:
    nlohmann::json GetInfoByDeviceId(uint16_t deviceId = DEFAULT_DEVICE_ID, const std::string &profPath = "");
    bool LoadJsonData(const std::string &profPath, const std::string &deviceDir, uint16_t deviceId);
    bool LoadLogData(const std::string &profPath, const std::string &deviceDir, uint16_t deviceId);
    bool CheckInfoValueIsValid(const std::string &profPath, uint16_t deviceId);
    std::unordered_map<std::string, std::map<uint16_t, nlohmann::json>> context_;
};  // class Context
}  // namespace Environment
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_PARSER_ENVIRONMENT_CONTEXT_H
