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

#ifndef ANALYSIS_UTILS_UTILS_H
#define ANALYSIS_UTILS_UTILS_H

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"

namespace Analysis
{
using namespace Common;
namespace Utils
{
using CHAR_PTR = char *;
constexpr int ACCURACY_THREE = 3;
constexpr double NS_TO_S = 1000.0 * 1000.0 * 1000.0;  // 1s = 10^9ns
std::string Join(const std::vector<std::string> &str, const std::string &delimiter);
std::vector<std::string> Split(const std::string &str, const std::string &delimiter, const int &splitPosition = -1);
std::string Rstrip(const std::string &str1, const std::string &str2);
int StrToU16(uint16_t &dest, const std::string &numStr);
int StrToU32(uint32_t &dest, const std::string &numStr);
int StrToU64(uint64_t &dest, const std::string &numStr);
int StrToDouble(double &dest, const std::string &numStr);
// 根据所给的device路径获取对应的deviceId {local_path}/PROF_xxx/device_{id} 返回id对应的数值
// 对于传入的 {local_path}/PROF_xxx/host, 也返回host对应的id（64）
uint16_t GetDeviceIdByDevicePath(const std::string &filePath);
bool IsNumber(const std::string &s);
// 将两个uint32_t 拼成uint64_t
uint64_t Contact(uint32_t high, uint32_t low);
bool IsDoubleEqual(double checkDouble, double standard);
std::string AddQuotation(std::string str);
std::string DivideByPowersOfTenWithPrecision(uint64_t value, int scale = ACCURACY_THREE, int accuracy = ACCURACY_THREE);
bool EndsWith(const std::string &str, const std::string &suffix);
std::string DoubleToStr(const double &value, const uint16_t &scale = ACCURACY_THREE);
double RoundToDecimalPlaces(const double num, int decimalPlaces = ACCURACY_THREE);
std::string FormatWithFixedLengthNumber(int number, int fixedLength);

// make_shared参数个数为0，异常操作为return void
#define MAKE_SHARED0_RETURN_VOID(instance, type) \
    do                                           \
    {                                            \
        try                                      \
        {                                        \
            instance = std::make_shared<type>(); \
        }                                        \
        catch (...)                              \
        {                                        \
            ERROR("Make shared ptr failed");     \
            instance = nullptr;                  \
            return;                              \
        }                                        \
    } while (0)

// make_shared参数个数为0，异常操作为return value
#define MAKE_SHARED0_RETURN_VALUE(instance, type, value) \
    do                                                   \
    {                                                    \
        try                                              \
        {                                                \
            instance = std::make_shared<type>();         \
        }                                                \
        catch (...)                                      \
        {                                                \
            ERROR("Make shared ptr failed");             \
            instance = nullptr;                          \
            return value;                                \
        }                                                \
    } while (0)

// make_shared参数个数不定，异常操作为return void
#define MAKE_SHARED_RETURN_VOID(instance, type, ...)        \
    do                                                      \
    {                                                       \
        try                                                 \
        {                                                   \
            instance = std::make_shared<type>(__VA_ARGS__); \
        }                                                   \
        catch (...)                                         \
        {                                                   \
            ERROR("Make shared ptr failed");                \
            instance = nullptr;                             \
            return;                                         \
        }                                                   \
    } while (0)

// make_shared参数个数不定，异常操作为return value
#define MAKE_SHARED_RETURN_VALUE(instance, type, value, ...) \
    do                                                       \
    {                                                        \
        try                                                  \
        {                                                    \
            instance = std::make_shared<type>(__VA_ARGS__);  \
        }                                                    \
        catch (...)                                          \
        {                                                    \
            ERROR("Make shared ptr failed");                 \
            instance = nullptr;                              \
            return value;                                    \
        }                                                    \
    } while (0)

// make_shared参数个数不定，异常操作为break
#define MAKE_SHARED_BREAK(instance, type, ...)              \
    do                                                      \
    {                                                       \
        try                                                 \
        {                                                   \
            instance = std::make_shared<type>(__VA_ARGS__); \
        }                                                   \
        catch (...)                                         \
        {                                                   \
            ERROR("Make shared ptr failed");                \
            instance = nullptr;                             \
            break;                                          \
        }                                                   \
    } while (0)

// make_shared参数个数0，异常操作无，常用于构造函数
#define MAKE_SHARED0_NO_OPERATION(instance, type, ...)      \
    do                                                      \
    {                                                       \
        try                                                 \
        {                                                   \
            instance = std::make_shared<type>(__VA_ARGS__); \
        }                                                   \
        catch (...)                                         \
        {                                                   \
            ERROR("Make shared ptr failed");                \
            instance = nullptr;                             \
        }                                                   \
    } while (0)

// make_shared参数个数不定，异常操作无，常用于构造函数
#define MAKE_SHARED_NO_OPERATION(instance, type, ...)       \
    do                                                      \
    {                                                       \
        try                                                 \
        {                                                   \
            instance = std::make_shared<type>(__VA_ARGS__); \
        }                                                   \
        catch (...)                                         \
        {                                                   \
            ERROR("Make shared ptr failed");                \
            instance = nullptr;                             \
        }                                                   \
    } while (0)

// 把变量名转为字符串
#define NAME_STR(name) (#name)

template <class T>
bool Reserve(std::vector<T> &vec, size_t s)
{
    try
    {
        vec.reserve(s);
    }
    catch (...)
    {
        ERROR("Reserve vector failed");
        return false;
    }
    return true;
}

template <class T>
bool Resize(std::vector<T> &vec, size_t s)
{
    try
    {
        vec.resize(s);
    }
    catch (...)
    {
        ERROR("Resize vector failed");
        return false;
    }
    return true;
}

template <typename T, typename U, U T::*element>
void Sort(std::vector<std::shared_ptr<T>> &items)
{
    std::stable_sort(items.begin(), items.end(), [&](const std::shared_ptr<T> &lhs, const std::shared_ptr<T> &rhs)
                     { return lhs && rhs && (*lhs).*element < (*rhs).*element; });
}

template <typename T, typename V>
T ReinterpretConvert(V ptr)
{
    return reinterpret_cast<T>(ptr);
}

template <typename K, typename... Args>
std::unique_ptr<K> MAKE_UNIQUE_PTR(Args &&...args)
{
    std::unique_ptr<K> ptr{new (std::nothrow) K(std::forward<Args>(args)...)};
    return ptr;
}

template <typename T, typename U>
inline std::shared_ptr<T> ReinterpretPointerCast(const std::shared_ptr<U> &r) noexcept
{
    return std::shared_ptr<T>(r, reinterpret_cast<typename std::shared_ptr<T>::element_type *>(r.get()));
}

// 元素数量等于1时，模板特例
template <typename T>
void Join(std::ostringstream &oss, T t)
{
    oss << t;
}

// 元素数量大于1时，递归展开可变参数模板
template <typename T, typename... Args>
void Join(std::ostringstream &oss, const std::string &delimiter, T t, Args... args)
{
    oss << t << delimiter;
    Join(oss, delimiter, args...);
}

// 调用可变参数模板，返回字符串
template <typename... Args>
std::string Join(const std::string &delimiter, Args... args)
{
    const int repeat = static_cast<int>(2 * delimiter.size());
    constexpr int count = static_cast<int>(sizeof...(args));
    // args的长度应该至少为1
    if (!count)
    {
        WARN("The number of inputs to the function should be greater than 1");
        return "";
    }

    std::ostringstream oss;
    Join(oss, delimiter, args...);
    auto res = oss.str();

    // 去除字符串结尾两次重复的delimiter
    return res.substr(0, res.size() - repeat);
}
}  // namespace Utils
}  // namespace Analysis

#endif  // ANALYSIS_UTILS_UTILS_H
