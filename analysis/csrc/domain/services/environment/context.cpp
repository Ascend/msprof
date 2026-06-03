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
#include "analysis/csrc/domain/services/environment/context.h"

#include <unordered_set>

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/config.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis
{
namespace Domain
{
namespace Environment
{
using namespace Analysis;
using namespace Analysis::Utils;
using namespace Viewer::Database;

namespace
{
const uint32_t ALL_EXPORT_VERSION = 0x072211;  // 2023年10月30号之后支持全导的驱动版本号 0x072211 = 467473
const uint64_t DEFAULT_DURATION_TIME_US = DEFAULT_DURATION_TIME_NS / MILLI_SECOND;
const std::string DEFAULT_HOST_UID = "0";
// 需要用到的json 和 log的文件名（前缀）
const std::string INFO_JSON = "info.json";
const std::string SAMPLE_JSON = "sample.json";
const std::string START_INFO = "start_info";
const std::string END_INFO = "end_info";
const std::string HOST_START_LOG = "host_start.log";
const std::string DEVICE_START_LOG = "dev_start.log";
// CHECK_VALUES存放一定存在的字段,字段不存在Load失败。
// 已经校验过的字段,可直接使用.at();未被校验过的字段则使用.value(),来设置默认值。确保读取正常
const std::set<std::string> CHECK_VALUES = {"platform_version",
                                            "startCollectionTimeBegin",
                                            "startClockMonotonicRaw",
                                            "pid",
                                            "CPU",
                                            "DeviceInfo",
                                            "llc_profiling",
                                            "ai_core_profiling_mode",
                                            "hostname",
                                            "memoryTotal",
                                            "netCard"};
}  // namespace

bool Context::Load(const std::set<std::string> &profPaths)
{
    for (const auto &profPath : profPaths)
    {
        std::vector<std::string> deviceDirs = File::GetOriginData(profPath, {HOST, DEVICE_PREFIX}, {});
        for (const auto &deviceDir : deviceDirs)
        {
            uint16_t deviceId = Utils::GetDeviceIdByDevicePath(deviceDir);
            if (deviceId == INVALID_DEVICE_ID)
            {
                ERROR("The prof path's deviceId is invalid.");
                return false;
            }
            if (!LoadJsonData(profPath, deviceDir, deviceId))
            {
                return false;
            }
            if (!LoadLogData(profPath, deviceDir, deviceId))
            {
                return false;
            }
            if (!CheckInfoValueIsValid(profPath, deviceId))
            {
                return false;
            }
        }
    }
    return true;
}

bool Context::LoadJsonData(const std::string &profPath, const std::string &deviceDir, uint16_t deviceId)
{
    for (const auto &fileName : {INFO_JSON, SAMPLE_JSON, START_INFO, END_INFO})
    {
        std::vector<std::string> files = File::GetOriginData(deviceDir, {fileName}, {"done"});
        if (files.size() != 1)
        {
            ERROR("The number of % in % is invalid, file num is: %.", fileName, deviceDir, files.size());
            if (fileName == END_INFO)
            {
                continue;
            }
            return false;
        }
        FileReader fd(files.back());
        nlohmann::json content;
        if (fd.ReadJson(content) != ANALYSIS_OK)
        {
            ERROR("Load json context failed: '%'.", files.back());
            return false;
        }

        if (fileName == START_INFO && content.contains("clockMonotonicRaw") && content.contains("collectionTimeBegin"))
        {
            content["startClockMonotonicRaw"] = content["clockMonotonicRaw"];
            content["startCollectionTimeBegin"] = content["collectionTimeBegin"];
        }
        else if (fileName == END_INFO && content.contains("clockMonotonicRaw") && content.contains("collectionTimeEnd"))
        {
            content["endClockMonotonicRaw"] = content["clockMonotonicRaw"];
            content["endCollectionTimeEnd"] = content["collectionTimeEnd"];
        }

        context_[profPath][deviceId].merge_patch(content);
    }
    return true;
}

bool Context::LoadLogData(const std::string &profPath, const std::string &deviceDir, uint16_t deviceId)
{
    const int expectTokenSize = 2;  // 2代表：前后的key和value
    // host就用host_start_log，device就用device_start_log。
    std::vector<std::string> fileNameList{HOST_START_LOG};
    if (deviceId != HOST_ID)
    {
        fileNameList.emplace_back(DEVICE_START_LOG);
    }
    for (const std::string &fileName : fileNameList)
    {
        std::vector<std::string> files = File::GetOriginData(deviceDir, {fileName}, {"done"});
        // host、device底下只有1份log
        if (files.size() != 1)
        {
            ERROR("The number of % in % is invalid, file num is: %.", fileName, deviceDir, files.size());
            return false;
        }
        FileReader fd(files.back());
        std::vector<std::string> text;
        if (fd.ReadText(text) != ANALYSIS_OK)
        {
            ERROR("Load log text failed: '%'.", files.back());
            return false;
        }
        for (const auto &line : text)
        {
            auto tokens = Utils::Split(line, ":");
            if (tokens.size() != expectTokenSize)
            {
                continue;
            }
            context_[profPath][deviceId][tokens[0]] = tokens[1];
        }

        if (!context_[profPath][deviceId].contains("clock_monotonic_raw") ||
            !context_[profPath][deviceId].contains("cntvct"))
        {
            return false;
        }

        if (fileName == DEVICE_START_LOG)
        {
            context_[profPath][deviceId]["devMonotonic"] = context_[profPath][deviceId]["clock_monotonic_raw"];
            context_[profPath][deviceId]["devCntvct"] = context_[profPath][deviceId]["cntvct"];
        }
        else if (fileName == HOST_START_LOG)
        {
            context_[profPath][deviceId]["hostMonotonic"] = context_[profPath][deviceId]["clock_monotonic_raw"];
            context_[profPath][deviceId]["hostCntvct"] = context_[profPath][deviceId]["cntvct"];
            if (context_[profPath][deviceId].contains("cntvct_diff"))
            {
                context_[profPath][deviceId]["hostCntvctDiff"] = context_[profPath][deviceId]["cntvct_diff"];
            }
            else
            {
                context_[profPath][deviceId]["hostCntvctDiff"] = "0";
            }
        }
    }
    return true;
}

bool Context::CheckInfoValueIsValid(const std::string &profPath, uint16_t deviceId)
{
    const auto &info = GetInfoByDeviceId(deviceId, profPath);
    for (auto &valueName : CHECK_VALUES)
    {
        if (!info.contains(valueName))
        {
            ERROR(
                "The key called %, not in the context info, "
                "the ProfPath is %, DeviceId is %.",
                valueName, profPath, deviceId);
            return false;
        }
    }
    if (deviceId == HOST_ID)
    {
        if (!info.at("CPU").is_array())
        {
            ERROR(
                "CPU's value is invalid, "
                "the ProfPath is %, DeviceId is %.",
                profPath, deviceId);
            return false;
        }
        if (!info.at("CPU").back().contains("Frequency"))
        {
            ERROR(
                "There is no Frequency in context info, "
                "the ProfPath is %, DeviceId is %.",
                profPath, deviceId);
            return false;
        }
    }
    else
    {
        if (!info.at("DeviceInfo").is_array() || (info.at("DeviceInfo").size() != 1))
        {
            ERROR(
                "DeviceInfo's value is invalid, "
                "the ProfPath is %, DeviceId is %.",
                profPath, deviceId);
            return false;
        }
        auto freqArr = info.at("DeviceInfo").back();
        if (!freqArr.contains("hwts_frequency") || freqArr.at("hwts_frequency").empty())
        {
            ERROR(
                "There is no hwts_frequency in context info, "
                "the ProfPath is %, DeviceId is %.",
                profPath, deviceId);
            return false;
        }
        if (!freqArr.contains("aic_frequency") || freqArr.at("aic_frequency").empty())
        {
            ERROR(
                "There is no aic_frequency in context info, "
                "the ProfPath is %, DeviceId is %.",
                profPath, deviceId);
            return false;
        }
    }
    return true;
}

bool Context::IsAllExport()
{
    const auto &info = GetInfoByDeviceId();
    if (info.empty())
    {
        ERROR("IsAllExport device info is empty.");
        return false;
    }
    // 低版本的驱动中不存在drvVersion字段，该字段使用时需要默认值
    auto drvVersion = info.value("drvVersion", 0u);
    if (drvVersion < ALL_EXPORT_VERSION)
    {
        WARN("DrvVersion not support all export, ALL_EXPORT_VERSION is %", ALL_EXPORT_VERSION);
        return false;
    }
    uint16_t chip;
    if (StrToU16(chip, info.at("platform_version")) != ANALYSIS_OK)
    {
        ERROR("str to uint16_t failed.");
        return false;
    }
    if (chip == static_cast<uint16_t>(Chip::CHIP_V1_1_0) || chip == static_cast<uint16_t>(Chip::CHIP_V3_1_0) ||
        chip == static_cast<uint16_t>(Chip::CHIP_V1_1_3))
    {
        WARN("Platform_version not support all export.");
        return false;
    }
    return true;
}

nlohmann::json Context::GetInfoByDeviceId(uint16_t deviceId, const std::string &profPath)
{
    nlohmann::json emptyJson;
    auto profInfo = profPath.empty() ? context_.begin() : context_.find(profPath);
    if (profInfo == context_.end())
    {
        ERROR("Can't find PROF file. input path %.", profPath);
        return emptyJson;
    }
    auto deviceInfo = profInfo->second;
    if (deviceInfo.begin() == deviceInfo.end())
    {
        ERROR("Can't find host or device file. input path %.", profPath);
        return emptyJson;
    }
    // 若不设置deviceId(使用默认id),则直接返回第一条info
    if (deviceId == DEFAULT_DEVICE_ID || deviceInfo.find(deviceId) == deviceInfo.end())
    {
        return deviceInfo.begin()->second;
    }
    return deviceInfo[deviceId];
}

uint16_t Context::GetPlatformVersion(uint16_t deviceId, const std::string &profPath)
{
    const auto &info = GetInfoByDeviceId(deviceId, profPath);
    uint16_t platformVersion = UINT16_MAX;
    if (info.empty())
    {
        ERROR("GetPlatformVersion device info is empty, input path %, deviceId %", profPath, deviceId);
        return platformVersion;
    }
    if (StrToU16(platformVersion, info.at("platform_version")) != ANALYSIS_OK)
    {
        ERROR("PlatformVersion to uint16_t failed, input path %, deviceId %", profPath, deviceId);
    }
    return platformVersion;
}

bool Context::GetProfTimeRecordInfo(Utils::ProfTimeRecord &record, const std::string &profPath, uint16_t deviceId)
{
    auto info = GetInfoByDeviceId(deviceId, profPath);
    if (info.empty())
    {
        WARN("There is no host time log in %, it will use device time log!", profPath);
        info = GetInfoByDeviceId(DEFAULT_DEVICE_ID, profPath);
        if (info.empty())
        {
            ERROR("No device time log in %, device id is %.", profPath, deviceId);
            return false;
        }
    }
    uint64_t startTimeUs = UINT64_MAX;
    if (StrToU64(startTimeUs, info.at("startCollectionTimeBegin")) != ANALYSIS_OK)
    {
        ERROR("StartTime to uint64_t failed. Prof path is %, device id is %.", profPath, deviceId);
        return false;
    }
    uint64_t endTimeUs = DEFAULT_DURATION_TIME_US + startTimeUs;
    if (StrToU64(endTimeUs, info.value("endCollectionTimeEnd", std::to_string(endTimeUs))) != ANALYSIS_OK)
    {
        ERROR("EndTime to uint64_t failed. Prof path is %, device id is %.", profPath, deviceId);
        return false;
    }
    uint64_t baseTimeNs = UINT64_MAX;
    if (StrToU64(baseTimeNs, info.at("startClockMonotonicRaw")) != ANALYSIS_OK)
    {
        ERROR("BaseTime to uint64_t failed. Prof path is %, device id is %.", profPath, deviceId);
        return false;
    }
    // 先判断时间之间的大小关系，确保后续计算时整数不回绕
    if ((startTimeUs * MILLI_SECOND < baseTimeNs))
    {
        ERROR("The value of startTimeUs and baseTimeNs is invalid. Path is %, device id is %.", profPath, deviceId);
        return false;
    }
    // startInfo endInfo 里的 collectionTime的单位是us，需要转换成ns
    record.startTimeNs = startTimeUs * MILLI_SECOND;
    record.endTimeNs = endTimeUs * MILLI_SECOND;
    record.baseTimeNs = baseTimeNs;
    return true;
}

uint32_t Context::GetPidFromInfoJson(uint16_t deviceId, const std::string &profPath)
{
    const auto &info = GetInfoByDeviceId(deviceId, profPath);
    uint32_t pid = 0;
    if (info.empty())
    {
        ERROR("GetPidFromInfoJson device info is empty, input path %, deviceId %", profPath, deviceId);
        return pid;
    }
    const std::string strPid = info.at("pid");
    if (strPid == "NA")
    {
        WARN("pid is NA and will be set 0.");
        return pid;
    }
    if (StrToU32(pid, strPid) != ANALYSIS_OK)
    {
        ERROR("Pid to uint32_t failed, input path %, deviceId %", profPath, deviceId);
    }
    return pid;
}

std::string Context::GetPidNameFromInfoJson(uint16_t deviceId, const std::string &profPath)
{
    const auto &info = GetInfoByDeviceId(deviceId, profPath);
    if (info.empty())
    {
        ERROR("GetPidNameFromInfoJson device info is empty.");
        return "";
    }
    return info.value("pid_name", "");
}

int64_t Context::GetMsBinPid(const std::string &profPath)
{
    const auto &info = GetInfoByDeviceId(DEFAULT_DEVICE_ID, profPath);
    if (info.empty())
    {
        PRINT_ERROR("Samplejson is empty, path %.", profPath);
        return analysis::dvvp::common::config::MSVP_MMPROCESS;
    }

    return info.value("msprofBinPid", analysis::dvvp::common::config::MSVP_MMPROCESS);
}

std::string Context::GetLLCProfiling(uint16_t deviceId, const std::string &profPath)
{
    const auto &info = GetInfoByDeviceId(deviceId, profPath);
    if (info.empty())
    {
        ERROR("Samplejson is empty, path %.", profPath);
        return "";
    }
    return info.value("llc_profiling", "");
}

bool Context::GetSyscntConversionParams(Utils::SyscntConversionParams &params, uint16_t deviceId,
                                        const std::string &profPath)
{
    auto info = GetInfoByDeviceId(deviceId, profPath);
    // host freq可用作host cnt计算，也可用于host diff计算
    if (info.empty())
    {
        ERROR("GetSyscntConversionParams device info is empty, input path %, deviceId %", profPath, deviceId);
        return false;
    }
    std::string hostFreqStr = info.at("CPU").back().at("Frequency");
    double hostFreq = DEFAULT_FREQ;
    if (hostFreqStr.empty())
    {
        INFO("HostFreq is empty, it will be set 1000.0 .");
    }
    else if (StrToDouble(hostFreq, hostFreqStr) != ANALYSIS_OK)
    {
        ERROR("HostFreq to double failed, input path %, deviceId %", profPath, deviceId);
        return false;
    }
    if (deviceId == HOST_ID)
    {
        params.freq = hostFreq;
    }
    else
    {
        // freq 来自info.json
        if (StrToDouble(params.freq, info.at("DeviceInfo").back().at("hwts_frequency")) != ANALYSIS_OK)
        {
            ERROR("DeviceFreq to double failed, input path %, deviceId %", profPath, deviceId);
            return false;
        }
    }
    if (IsDoubleEqual(params.freq, 0) || IsDoubleEqual(hostFreq, 0))
    {
        ERROR("Freq is 0, can't be used to calculate, input path %, deviceId %", profPath, deviceId);
        return false;
    }
    params.hostFreq = hostFreq;
    // host取host的cnt， device取device的cnt
    std::string cntName = (deviceId == HOST_ID) ? "hostCntvct" : "devCntvct";
    if (StrToU64(params.sysCnt, info.value(cntName, "0")) != ANALYSIS_OK)
    {
        ERROR("SysCnt to uint64_t failed, input path %, deviceId %", profPath, deviceId);
        return false;
    }
    // hostCnt fetch from the host_start_log file
    if (StrToU64(params.hostCnt, info.value("hostCntvct", "0")) != ANALYSIS_OK)
    {
        ERROR("hostCnt to uint64_t failed, input path %, deviceId %", profPath, deviceId);
        return false;
    }
    // hostMonotonic 来自 host_start_log 的 clock_monotonic_raw
    if (StrToU64(params.hostMonotonic, info.value("hostMonotonic", "0")) != ANALYSIS_OK)
    {
        ERROR("HostMonotonic to uint64_t failed, input path %, deviceId %", profPath, deviceId);
        return false;
    }
    uint64_t diff = 0;
    if (StrToU64(diff, info.value("hostCntvctDiff", "0")) != ANALYSIS_OK)
    {
        WARN("HostCntvctDiff to uint64_t failed, input path %, deviceId %", profPath, deviceId);
        // diff 异常不影响数据解析，部分时间存在误差
        return true;
    }
    // 保留整数位即可, monotonic(ns), diff(次), freq(MHz). 次 * 1000 / MHz = ns
    params.hostMonotonic += (diff * MILLI_SECOND / hostFreq);
    return true;
}

bool Context::GetPmuFreq(double &freq, uint16_t deviceId, const std::string &profPath)
{
    if (deviceId == HOST_ID)
    {
        ERROR("Host do not have aic or aiv frequency!");
        return false;
    }
    auto info = GetInfoByDeviceId(deviceId, profPath);
    if (info.empty())
    {
        ERROR("GetPmuFreq device info is empty, input path %, deviceId %", profPath, deviceId);
        return false;
    }
    // freq 来自info.json
    if (StrToDouble(freq, info.at("DeviceInfo").back().at("aic_frequency")) != ANALYSIS_OK)
    {
        ERROR("DeviceFreq to double failed, input path %, deviceId %", profPath, deviceId);
        return false;
    }
    return true;
}

bool Context::GetMetricMode(std::string &metricMode, const std::string &profPath)
{
    auto info = GetInfoByDeviceId(DEFAULT_DEVICE_ID, profPath);
    if (info.empty())
    {
        ERROR("GetMetricMode device info is empty.");
        return false;
    }
    metricMode = info.at("ai_core_profiling_mode");
    return true;
}

bool Context::GetClockMonotonicRaw(uint64_t &monotonicRaw, bool isHost, uint16_t deviceId, const std::string &profPath)
{
    if (!isHost && (deviceId == HOST_ID))
    {
        ERROR("GetClockMonotonicRaw host do not have device monotonic!");
        return false;
    }
    auto info = GetInfoByDeviceId(deviceId, profPath);
    if (info.empty())
    {
        ERROR("GetClockMonotonicRaw device info is empty.");
        return false;
    }
    // host取host monotonic， device取device的monotonic
    std::string monotonic = isHost ? "hostMonotonic" : "devMonotonic";
    if (StrToU64(monotonicRaw, info.value(monotonic, "0")) != ANALYSIS_OK)
    {
        ERROR("Monotonic to uint64_t failed, input path %, deviceId %", profPath, deviceId);
        return false;
    }
    if (!isHost)
    {
        return true;
    }
    std::string hostFreqStr = info.at("CPU").back().at("Frequency");
    double hostFreq = DEFAULT_FREQ;
    if (hostFreqStr.empty())
    {
        INFO("HostFreq is empty, it will be set 1000.0 .");
    }
    else if (StrToDouble(hostFreq, hostFreqStr) != ANALYSIS_OK)
    {
        ERROR("HostFreq to double failed, input path %, deviceId %", profPath, deviceId);
        return false;
    }
    if (IsDoubleEqual(hostFreq, 0))
    {
        ERROR("Freq is 0, can't be used to calculate.");
        return false;
    }
    uint64_t diff = 0;
    if (StrToU64(diff, info.value("hostCntvctDiff", "0")) != ANALYSIS_OK)
    {
        WARN("HostCntvctDiff to uint64_t failed, input path %, deviceId %", profPath, deviceId);
        // diff 异常不影响数据解析，部分时间存在误差
        return true;
    }
    // 保留整数位即可, monotonic(ns), diff(次), freq(MHz). 次 * 1000 / MHz = ns
    monotonicRaw += (diff * MILLI_SECOND / hostFreq);
    return true;
}

std::string Context::GetHostUid(uint16_t deviceId, const std::string &profPath)
{
    const auto &info = GetInfoByDeviceId(deviceId, profPath);
    if (info.empty())
    {
        ERROR("GetHostUid InfoJson info is empty, input path %, deviceId %", profPath, deviceId);
        return DEFAULT_HOST_UID;
    }
    return info.value("hostUid", DEFAULT_HOST_UID);
}

std::string Context::GetHostName(uint16_t deviceId, const std::string &profPath)
{
    const auto &info = GetInfoByDeviceId(deviceId, profPath);
    if (info.empty())
    {
        ERROR("GetHostName InfoJson info is empty.");
        return "";
    }
    return info.at("hostname");
}

std::vector<std::string> Context::GetQosEvents(uint16_t deviceId, const std::string &profPath)
{
    if (deviceId == HOST_ID)
    {
        ERROR("Host do not have qosEvents!");
        return {};
    }
    auto info = GetInfoByDeviceId(deviceId, profPath);
    if (info.empty())
    {
        ERROR("GetQosEvents device info is empty.");
        return {};
    }
    std::string qosEvents = info.value("qosEvents", "");
    if (qosEvents.empty())
    {
        INFO("Check qosProfiling is on or off, if it is on, maybe some mistakes have happened");
        return {};
    }
    else
    {
        return Split(qosEvents, ",");
    }
}

void Context::Clear() { context_.clear(); }

bool Context::IsStarsChip(uint16_t platformVersion)
{
    return IsChipV1(platformVersion) || IsChipV4(platformVersion) || IsChipV6(platformVersion);
}

bool Context::IsChipV1(uint16_t platformVersion)
{
    auto chipV1_1_1 = static_cast<uint16_t>(Chip::CHIP_V1_1_1);
    auto chipV1_1_2 = static_cast<uint16_t>(Chip::CHIP_V1_1_2);
    auto chipV1_1_3 = static_cast<uint16_t>(Chip::CHIP_V1_1_3);
    std::unordered_set<uint16_t> checkList{chipV1_1_1, chipV1_1_2, chipV1_1_3};
    return static_cast<bool>(checkList.count(platformVersion));
}

bool Context::IsChipV4(uint16_t platformVersion) { return platformVersion == static_cast<int>(Chip::CHIP_V4_1_0); }

bool Context::IsChipV6(uint16_t platformVersion)
{
    auto chipV6_1_0 = static_cast<uint16_t>(Chip::CHIP_V6_1_0);
    auto chipV6_2_0 = static_cast<uint16_t>(Chip::CHIP_V6_2_0);
    std::unordered_set<uint16_t> checkList{chipV6_1_0, chipV6_2_0};
    return static_cast<bool>(checkList.count(platformVersion));
}

bool Context::IsFirstChipV1(uint16_t platformVersion) { return platformVersion == static_cast<int>(Chip::CHIP_V1_1_0); }

uint16_t Context::GetAiCoreNum(uint16_t deviceId, const std::string &profPath)
{
    if (deviceId == HOST_ID)
    {
        ERROR("Host do not have ai core num!");
        return 0;
    }
    auto info = GetInfoByDeviceId(deviceId, profPath);
    if (info.empty())
    {
        ERROR("GetAiCoreNum device info is empty, input path %, deviceId %", profPath, deviceId);
        return 0;
    }
    return info.at("DeviceInfo").back().value("ai_core_num", 0);
}

uint64_t Context::GetTotalMem(uint16_t deviceId, const std::string &profPath)
{
    auto info = GetInfoByDeviceId(deviceId, profPath);
    // 这里如果没有host目录 会去取到device的数据 本质上依赖外层路径校验
    if (info.empty())
    {
        ERROR("info is empty, input path %, deviceId %", profPath, deviceId);
        return 0;
    }
    return info.at("memoryTotal");
}

uint64_t Context::GetNetCardTotalSpeed(uint16_t deviceId, const std::string &profPath)
{
    auto info = GetInfoByDeviceId(deviceId, profPath);
    // 这里如果没有host目录 会去取到device的数据 本质上依赖外层路径校验
    if (info.empty())
    {
        ERROR("info is empty, input path %, deviceId %", profPath, deviceId);
        return 0;
    }
    uint64_t totalSpeed = 0;
    // 负数不计数
    for (const auto netCard : info.at("netCard"))
    {
        auto speed = netCard.value("speed", 0);
        totalSpeed += (speed < 0) ? 0 : static_cast<uint64_t>(speed);
    }
    return totalSpeed;
}

bool Context::IsLevel0(const std::string &profPath)
{
    auto info = GetInfoByDeviceId(DEFAULT_DEVICE_ID, profPath);
    // 这里如果没有host目录 会去取到device的数据 本质上依赖外层路径校验
    if (info.empty())
    {
        ERROR("info is empty, input path %", profPath);
        return true;
    }
    std::set<std::string> level0Set = {"l0", "level0"};
    // profLevel(产品): l0, l1       prof_level(hisi): level0, level1
    std::string profLevel = info.contains("profLevel") ? info.value("profLevel", "") : info.value("prof_level", "");
    return (level0Set.find(profLevel) != level0Set.end());
}

std::vector<uint16_t> Context::GetCannVersion(uint16_t deviceId, const std::string &profPath)
{
    std::vector<uint16_t> emptyVersion;
    std::vector<uint16_t> retVersion;
    constexpr size_t CANN_VERSION_CNT = 2;  // 2: major version and minor version
    auto info = GetInfoByDeviceId(deviceId, profPath);
    if (info.empty())
    {
        ERROR("GetCannVersion device info is empty, input path %, deviceId %", profPath, deviceId);
        return emptyVersion;
    }
    if (info.contains("cannVersion") && info.at("cannVersion").is_string())
    {
        // cann version format: major.minor.patch(-beta.x), like 9.1.0 or 9.1.0-beta.0 or 9.1.T100
        std::string verStr = info.at("cannVersion").get<std::string>();
        uint16_t verVal = 0;
        for (size_t i = 0; i < CANN_VERSION_CNT; ++i)
        {
            size_t pos = verStr.find(".");
            std::string seg;
            if (pos == std::string::npos)
            {
                seg = verStr;
            }
            else
            {
                seg = verStr.substr(0, pos);
                verStr = verStr.substr(pos + 1);
            }
            if (seg.empty())
            {
                return emptyVersion;
            }
            for (char c : seg)
            {
                if (!std::isdigit(c))
                {
                    return emptyVersion;
                }
            }
            if (StrToU16(verVal, seg) != ANALYSIS_OK)
            {
                ERROR("CannVersion segment to uint16_t failed, input path %, deviceId %", profPath, deviceId);
                return emptyVersion;
            }
            retVersion.push_back(verVal);

            if (verStr.empty())
            {
                break;
            }
        }
        return retVersion.size() == CANN_VERSION_CNT ? retVersion : emptyVersion;
    }
    return emptyVersion;
}

}  // namespace Environment
}  // namespace Domain
}  // namespace Analysis
