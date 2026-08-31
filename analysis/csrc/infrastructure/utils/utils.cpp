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

#include "utils.h"

#include <iomanip>
#include <unordered_set>

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
namespace Utils
{
using namespace Analysis::Domain::Environment;

namespace
{
// 归一 StrToU16/U32/U64/Int64/Double 的公共路径：空串/异常/残留字符三段校验，
// 各转换函数仅需提供对应的 std 转换 lambda，避免重复相同的错误处理代码
template <typename T, typename Convert>
int StrToNumImpl(const std::string &funcName, T &dest, const std::string &numStr, Convert convert)
{
    if (numStr.empty())
    {
        ERROR("% failed, the input string is empty.", funcName.c_str());
        return ANALYSIS_ERROR;
    }
    size_t pos = 0;
    try
    {
        dest = convert(numStr, &pos);
    }
    catch (...)
    {
        ERROR("% failed, the input string is '%'.", funcName.c_str(), numStr.c_str());
        return ANALYSIS_ERROR;
    }
    if (pos != numStr.size())
    {
        ERROR("% failed, the input string is '%'.", funcName.c_str(), numStr.c_str());
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}
}  // namespace

std::string Join(const std::vector<std::string> &str, const std::string &delimiter)
{
    std::stringstream ss;
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (i > 0)
        {
            ss << delimiter;
        }
        ss << str.at(i);
    }
    return ss.str();
}

std::vector<std::string> Split(const std::string &str, const std::string &delimiter, const int &splitPosition)
{
    std::vector<std::string> res;
    if (delimiter.empty())
    {
        res.emplace_back(str);
        return res;
    }
    size_t start = 0;
    size_t end;
    if (splitPosition < 0)
    {
        while ((end = str.find(delimiter, start)) != std::string::npos)
        {
            res.emplace_back(str.substr(start, end - start));
            start = end + delimiter.length();
        }
        res.emplace_back(str.substr(start));
    }
    else
    {
        int count = 0;
        while (count < splitPosition && (end = str.find(delimiter, start)) != std::string::npos)
        {
            start = end + delimiter.length();
            count++;
        }
        if (count < splitPosition)
        {
            return res;
        }
        res.emplace_back(str.substr(0, start - 1));
        res.emplace_back(str.substr(start));
    }
    return res;
}

std::string Rstrip(const std::string &str1, const std::string &str2)
{
    size_t end = str1.find_last_not_of(str2);
    return (end == std::string::npos) ? "" : str1.substr(0, end + 1);
}

int StrToU16(uint16_t &dest, const std::string &numStr)
{
    return StrToNumImpl(__func__, dest, numStr, [](const std::string &s, size_t *pos) { return std::stoi(s, pos); });
}

int StrToU32(uint32_t &dest, const std::string &numStr)
{
    return StrToNumImpl(__func__, dest, numStr, [](const std::string &s, size_t *pos) { return std::stoul(s, pos); });
}

int StrToU64(uint64_t &dest, const std::string &numStr)
{
    return StrToNumImpl(__func__, dest, numStr, [](const std::string &s, size_t *pos) { return std::stoull(s, pos); });
}

int StrToInt64(int64_t &dest, const std::string &numStr)
{
    return StrToNumImpl(__func__, dest, numStr, [](const std::string &s, size_t *pos) { return std::stoll(s, pos); });
}

int StrToDouble(double &dest, const std::string &numStr)
{
    return StrToNumImpl(__func__, dest, numStr, [](const std::string &s, size_t *pos) { return std::stod(s, pos); });
}

uint16_t GetDeviceIdByDevicePath(const std::string &filePath)
{
    auto tempStr = filePath;
    while (tempStr.back() == '/')
    {
        tempStr.pop_back();
    }
    // 默认host的deviceId为64
    uint16_t deviceId = INVALID_DEVICE_ID;
    auto dirName = Split(tempStr, "/").back();
    if (dirName == "host")
    {
        return HOST_ID;
    }
    if (StrToU16(deviceId, Split(dirName, "_").back()) != ANALYSIS_OK)
    {
        ERROR("DeviceId to uint16_t failed.");
        return deviceId;
    }
    return deviceId;
}

bool IsNumber(const std::string &s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it))
    {
        ++it;
    }
    return !s.empty() && it == s.end();
}

uint64_t Contact(uint32_t high, uint32_t low)
{
    const uint8_t shift = 32;
    return (static_cast<uint64_t>(high) << shift) | low;
}

bool IsDoubleEqual(double checkDouble, double standard)
{
    const double eps = 1e-9;
    return (std::abs(checkDouble - standard) < eps);
}

std::string AddQuotation(std::string str) { return Join({"\"", str, "\""}, ""); }

std::string DivideByPowersOfTenWithPrecision(uint64_t value, int accuracy, int scale)
{
    // scale代表除以10的多少次幂，比如3就是除以10^3，accuracy代表保留位数
    std::string numStr = std::to_string(value);
    // 在位置前插入小数点
    if (static_cast<int>(numStr.size()) <= scale)
    {
        numStr.insert(0, scale - numStr.size() + 1, '0');
    }
    numStr.insert(numStr.size() - scale, ".");
    if (scale == accuracy)
    {  // 精度与移位数相等，直接返回即可
        return numStr;
    }
    else if (accuracy > scale)
    {  // 精度比移位数大，需要末尾补0
        numStr.insert(numStr.end(), accuracy - scale, '0');
        return numStr;
    }
    else
    {  // 精度比移位数小，需要截取numStr.size() - (scale - accuracy)长个字符串
        return numStr.substr(0, numStr.size() + accuracy - scale);
    }
}

bool EndsWith(const std::string &str, const std::string &suffix)
{
    if (suffix.size() > str.size())
    {
        return false;
    }
    return str.substr(str.size() - suffix.size()) == suffix;
}

std::string DoubleToStr(const double &value, const uint16_t &scale)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(scale) << value;
    std::string result = oss.str();

    // 去掉末尾的零
    size_t pos = result.find('.');
    if (pos != std::string::npos)
    {
        // 找到最后一个非零的位置
        size_t lastNonZero = result.find_last_not_of('0');
        if (lastNonZero != std::string::npos)
        {
            result = result.substr(0, lastNonZero + 1);
            if (result.back() == '.')
            {
                result.pop_back();
            }
        }
        else
        {
            result = result.substr(0, pos);
        }
    }
    return result;
}

double RoundToDecimalPlaces(const double num, int decimalPlaces)
{
    double multiplier = std::pow(10.0, decimalPlaces);
    return std::round(num * multiplier) / multiplier;
}

/**
 * @brief 生成「固定位数补零数字」的字符串
 * @param number 要格式化的数字（支持任意整数类型：int/uint16_t/uint32_t 等）
 * @param fixedLength 数字部分的固定长度，不足补 0，超过则按实际长度输出
 * @return 格式化后的完整字符串（如 number=1, fixed_length=3 → "001"）
 */
std::string FormatWithFixedLengthNumber(int number, int fixedLength)
{
    if (fixedLength < 1)
    {
        fixedLength = 1;
    }

    if (number < 0)
    {
        return std::to_string(number);
    }

    std::ostringstream oss;
    oss << std::setw(fixedLength) << std::setfill('0') << std::right << number;

    return oss.str();
}

}  // namespace Utils
}  // namespace Analysis
