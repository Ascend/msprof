/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */
#include "analysis/csrc/infrastructure/utils/hp_float.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "analysis/csrc/infrastructure/dfx/log.h"
namespace Analysis
{
namespace Utils
{
namespace
{
const int ROUNDING_THRESHOLD = 5;
const int CARRY_THRESHOLD = 10;
const int ASCII_0 = 48;
const int ASCII_9 = 57;
const int POWER_10 = 10;  // 根据用途分别定义10，增加代码可读性
}  // namespace
// 主要用于msprof相关的大数运算
HPFloat::HPFloat()
{
    for (int32_t i = 0; i < defaultPrecision_; i++)
    {
        num_.emplace_back(0);
    }
    symbol_ = false;
    dynamicLen_ = 0;
    digit_ = 0;
}

HPFloat::HPFloat(const HPFloat &value) { *this = value; }

int32_t HPFloat::Len() const { return static_cast<int32_t>(num_.size()); }

// 清空数据，保留精度
void HPFloat::Clear()
{
    for (signed char &i : num_)
    {
        i = 0;
    }
    digit_ = 0;
    symbol_ = false;
    dynamicLen_ = 0;
}

void HPFloat::Simple()
{
    int32_t index = 0;
    auto length = Len();
    // 计算末尾的零
    while (index < length && num_[index] == 0)
    {
        index++;
    }
    // 无需化简，返回
    if (index == 0)
    {
        return;
    }
    if (index == length)
    {
        // 数量级和符号归零
        digit_ = 0;
        symbol_ = false;
        dynamicLen_ = 0;
    }
    else
    {
        // 后退去零
        MoveBackward(index);
    }
}

void HPFloat::SetPrecision(int32_t length)
{
    if (length < 0)
    {
        ERROR("Invalid argument, SetPrecision function accepts only positive integer arguments");
        return;
    }
    else if (length > maxPrecision_)
    {
        ERROR("Invalid argument, length exceeds maximum allowed precision");
        return;
    }
    int32_t oriLen = Len();
    if (oriLen == length)
    {
        return;
    }
    if (length < minPrecision_)
    {
        return;
    }
    // 判断需要扩容还是收缩
    if (length > oriLen)
    {
        for (int32_t i = 0; i < length - oriLen; i++)
        {
            num_.emplace_back(0);
        }
    }
    else
    {
        MoveForward(oriLen - dynamicLen_ - 1);
        MoveBackward(oriLen - length);
        num_.erase(num_.end() - oriLen + length, num_.end());
    }
    Simple();
}

void HPFloat::SetPrecision(unsigned long length) { SetPrecision(static_cast<int32_t>(length)); }

void HPFloat::SetPrecision(long long length) { SetPrecision(static_cast<int32_t>(length)); }

void HPFloat::SetDynamicLen()
{
    // 如果精度全部占满，设置全长
    if (num_[Len() - 1] != 0)
    {
        dynamicLen_ = Len() - 1;
        return;
    }
    auto length = Len();
    // 计算高位零的数量，这里num_.size-1使用int32_t为了避免死循环
    for (auto i = length - 1; i >= 0; i--)
    {
        if (num_[i] == 0)
        {
            continue;
        }
        else
        {
            dynamicLen_ = i;
            return;
        }
    }
}

int32_t HPFloat::MinDig() const { return digit_ - dynamicLen_; }

int32_t HPFloat::TheoreticalMinDig() const { return digit_ - Len() + 1; }

void HPFloat::BackSpace()
{
    // 记录末位
    signed char tail = num_[0];
    for (int32_t i = 1; i <= dynamicLen_; i++)
    {
        num_[i - 1] = num_[i];
        num_[i] = 0;
    }
    // 四舍五入
    if (tail >= ROUNDING_THRESHOLD)
    {
        CoorAdd(1, 0);
    }
    dynamicLen_ -= 1;
}

void HPFloat::MoveForward(int32_t step)
{
    if (!step)
    {
        return;
    }
    // 负值则后移
    if (step < 0)
    {
        MoveBackward(-step);
        return;
    }
    auto length = Len();
    for (int32_t i = dynamicLen_; i >= 0; i--)
    {
        // 处理超出精度范围情况
        if (step + i >= length)
        {
            continue;
        }
        num_[step + i] = num_[i];
        num_[i] = 0;
    }
    SetDynamicLen();
}

void HPFloat::MoveBackward(int32_t step)
{
    if (!step)
    {
        return;
    }
    if (step < 0)
    {
        MoveForward(-step);
        return;
    }
    // 移动大于精度，全清，保留数量级
    if (step > dynamicLen_ + 1)
    {
        Clear();
        return;
    }
    signed char tail = num_[step - 1];
    for (int32_t i = step; i <= dynamicLen_; i++)
    {
        num_[i - step] = num_[i];
        num_[i] = 0;
    }
    if (tail >= ROUNDING_THRESHOLD)
    {
        CoorAdd(1, 0);  // 四舍五入后进位
    }
    dynamicLen_ -= step;
}

bool HPFloat::SimplifyStr(std::string &str)
{
    bool symbol = false;
    if (str[0] == '-')
    {
        symbol = true;
        str.erase(0, 1);
    }
    int32_t index = 0;
    auto length = Len();
    while (index < length && str[index] == '0')
    {
        index++;
    }
    if (index != 0)
    {
        str.erase(0, index);  // 删掉多余的零
    }
    reverse(str.begin(), str.end());  // 倒序，和HPFloat的顺序相同
    return symbol;
}

void HPFloat::CoorAdd(signed char op, int32_t psi)
{
    // 如果在已用精度之前，直接填充
    if (psi > dynamicLen_)
    {
        digit_ += psi - dynamicLen_;
        num_[psi] = op;
        dynamicLen_ = psi;
        return;
    }
    num_[psi] += op;
    // 是否进位
    if (num_[psi] >= CARRY_THRESHOLD)
    {
        unsigned char temp = 0;
        // 如果精度已满
        if (psi == Len() - 1)
        {
            // 退格让出一位精度
            BackSpace();
            // 这里不会出现psi-1小于0的情况,因为允许最小精度是3位
            num_[psi - 1] -= CARRY_THRESHOLD;
            temp = num_[psi - 1];
            digit_++;
            num_[psi] = 1;
            // 还原因BackSpace造成的dynamicLen_变化
            dynamicLen_ = psi;
        }
        else
        {
            CoorAdd(1, psi + 1);
            num_[psi] -= CARRY_THRESHOLD;
            temp = num_[psi];
        }
        if (!temp)
        {
            Simple();
        }
    }
}

void HPFloat::DigAdd(signed char op, int32_t n)
{
    Simple();
    // 给定数量级不超过数据最大数量级情况
    if (digit_ >= n)
    {
        // 如果超出理论最小精度范围1位以上，直接返回
        if (TheoreticalMinDig() - 1 > n)
        {
            return;
        }
        // 如果超出理论最小精度范围1位，四舍五入处理末位数据
        if (TheoreticalMinDig() - 1 == n)
        {
            if (op >= ROUNDING_THRESHOLD)
            {
                MoveForward(Len() - dynamicLen_ - 1);
                CoorAdd(1, 0);
                return;
            }
            else
            {
                return;
            }
        }
        // 判断是否需要平移来对齐
        if (MinDig() <= n)
        {
            CoorAdd(op, n - digit_ + dynamicLen_);
        }
        else
        {  // 给定数量级小于原数据最小数量级，原数据MoveForward提供存放空间
            MoveForward(MinDig() - n);
            num_[0] = op;
        }
    }
    else
    {  // 给定数量级未超过数据理论最大数量级情况
        if (MinDig() + Len() - 1 >= n)
        {
            dynamicLen_ += n - digit_;
            num_[dynamicLen_] = op;  // 精度足够，直接填充
        }
        else
        {
            MoveBackward(n - (MinDig() + Len() - 1));  // 精度不足，舍去末位数据
            num_[Len() - 1] = op;
            dynamicLen_ = Len() - 1;
        }
        digit_ = n;
    }
}

void HPFloat::CoreCoorSub(signed char op, int32_t psi)
{
    num_[psi] -= op;
    if (num_[psi] < 0)
    {
        num_[psi] += CARRY_THRESHOLD;
        CoreCoorSub(1, psi + 1);  // 递归借位
    }
}

void HPFloat::CoorSub(signed char op, int32_t psi)
{
    int32_t prevLen = dynamicLen_;
    CoreCoorSub(op, psi);
    SetDynamicLen();
    digit_ -= prevLen - dynamicLen_;
    Simple();
}

// 不支持小减大，通过对外接口-,-=限制
void HPFloat::DigSub(signed char op, int32_t n)
{
    // 如果超出理论最小精度范围1位以上，直接返回
    if (TheoreticalMinDig() - 1 > n)
    {
        return;
    }
    // 如果超出理论最小精度范围1位，四舍五入处理末位数据
    if (TheoreticalMinDig() - 1 == n)
    {
        if (op >= ROUNDING_THRESHOLD)
        {
            MoveForward(Len() - dynamicLen_ - 1);
            CoorSub(1, 0);
            return;
        }
        else
        {
            return;
        }
    }
    // 是否需要平移来对齐
    if (MinDig() <= n)
    {
        CoorSub(op, n - digit_ + dynamicLen_);
    }
    else
    {  // 给定数量级小于原数据最小数量级，原数据MoveForward提供存放空间
        MoveForward(MinDig() - n);
        CoorSub(op, 0);
    }
}

std::string HPFloat::Str()
{
    std::string str;
    if (symbol_)
    {
        str += '-';
    }
    // 是否需要补零
    if (digit_ < 0)
    {
        str += "0.";
        for (long long i = -1; i > digit_; i--)
        {
            str += '0';  // 高位补零
        }
    }
    for (int32_t i = dynamicLen_; i >= 0; i--)
    {
        str += static_cast<char>(num_[i] + ASCII_0);  // 循环输出
        if (dynamicLen_ - i == digit_ && i != 0)
        {
            str += '.';  // 小数点
        }
    }
    // 低位补零
    for (int32_t i = MinDig(); i > 0; i--)
    {
        str += '0';
    }

    return str;
}

double HPFloat::Double()
{
    // 不建议输出Double格式，因为转Double过程本身会丢失精度
    double ans = 0;
    for (int32_t i = 0; i <= dynamicLen_; i++)
    {
        double temp = num_[i];
        temp *= pow(POWER_10, MinDig() + i);
        ans += temp;  // 循环加法输出
    }
    if (this->symbol_)
    {
        ans *= -1;
    }
    return ans;
}

void HPFloat::Quantize(int32_t n)
{
    if (n < 0)
    {
        ERROR("Invalid argument, Quantize function accepts only positive integer arguments");
        return;
    }
    int32_t offset = -MinDig() - n;
    if (offset <= 0)
    {
        return;
    }
    MoveBackward(offset);
}

uint64_t HPFloat::Uint64()
{
    if (digit_ < 0)
    {
        return 0;
    }
    else if (symbol_)
    {
        return UINT64_MAX;
    }
    uint64_t res = 0;
    int32_t start = dynamicLen_ - digit_;  // dynamicLen_表示vector总长度，digit_表示最大位数，前者>=后者
    for (int32_t i = dynamicLen_; i >= start; i--)
    {
        res = res * POWER_10 + (i < 0 ? 0 : num_[i]);
    }
    return res;
}

HPFloat &HPFloat::operator=(const HPFloat &op)
{
    if (this == &op)
    {
        return *this;
    }
    Clear();
    SetPrecision(op.Len());
    int32_t len = op.dynamicLen_;
    for (int32_t i = 0; i <= len; ++i)
    {
        this->num_[i] = op.num_[i];
    }
    digit_ = op.digit_;
    symbol_ = op.symbol_;
    dynamicLen_ = op.dynamicLen_;
    Simple();
    return *this;
}

HPFloat &HPFloat::operator=(unsigned long long op)
{
    Clear();
    // 为了处理num数量级超过用例最大精度溢出情况，记录原始精度同时扩大当前精度
    auto oriPrecision = Len();
    // 取正
    unsigned long long num = op;
    int32_t n = 0;
    unsigned long long tmp = 1;
    while (tmp <= num)
    {
        // 计算数量级
        n++;
        num /= POWER_10;
    }
    n = std::min(n, maxPrecision_);  // 取最小值，防止vector溢出
    SetPrecision(std::max(oriPrecision, n));
    for (int32_t i = 0; i <= n; i++)
    {
        // 逐个填入
        num_[i] = op % POWER_10;
        op = (op - op % POWER_10) / POWER_10;
    }
    digit_ = n - 1;
    dynamicLen_ = n - 1;
    symbol_ = false;
    // 还原精度，正确处理精度丢失
    SetPrecision(oriPrecision);
    Simple();
    return *this;
}

HPFloat &HPFloat::operator=(long long op)
{
    Clear();
    // 取正
    unsigned long long num = (op > 0) ? static_cast<unsigned long long>(op) : static_cast<unsigned long long>(-op);
    *this = num;
    symbol_ = op < 0;
    Simple();
    return *this;
}

HPFloat &HPFloat::operator=(const int op) { return *this = static_cast<long long>(op); }

HPFloat &HPFloat::operator=(const unsigned int op) { return *this = static_cast<unsigned long long>(op); }

HPFloat &HPFloat::operator=(const long op) { return *this = static_cast<long long>(op); }

HPFloat &HPFloat::operator=(const unsigned long op) { return *this = static_cast<unsigned long long>(op); }

HPFloat &HPFloat::operator=(const std::string &num)
{
    Clear();
    if (num.empty())
    {
        return *this;
    }
    unsigned long oriPrecision =
        num_.size();  // 为了处理num数量级超过用例最大精度溢出情况，记录原始精度同时扩大当前精度
    SetPrecision(std::max(oriPrecision, num.size()));
    std::string op = num;
    symbol_ = SimplifyStr(op);
    int8_t ifDot = 0;
    int32_t dotLoc = 0;
    int32_t count = 0;  // 用于记录有效数字位数
    for (int32_t i = 0; i < static_cast<int32_t>(op.size()) && count < maxPrecision_; i++)
    {
        if (op[i] == '.')
        {
            if (ifDot)
            {
                ERROR("Invalid character encountered");
                continue;
            }
            ifDot = 1;
            dotLoc = i;
            count += 1;
            continue;
        }
        if (op[i] < ASCII_0 || op[i] > ASCII_9)
        {  // 遇到异常数据，抛出错误,行为与库函数一致,48:'0',57:'9'
            ERROR("Invalid character encountered");
            continue;
        }
        num_[i - ifDot] = op[i] - ASCII_0;  // 录入数据， i恒定>=ifDot
        count += 1;
    }
    if (op[op.size() - 1] == '.')
    {  // 如果小数点在末尾
        op.erase(op.size() - 1, 1);
        int32_t zeroNum = 0;
        while (zeroNum <= static_cast<int32_t>(op.size()) - 1 &&
               op[static_cast<int32_t>(op.size()) - 1 - zeroNum] == '0')
        {
            zeroNum++;
        }
        digit_ += -zeroNum - 1;  // 算出数量级
    }
    else
    {
        digit_ += static_cast<int32_t>(op.size()) - 1 - dotLoc - ifDot;  // 算出数量级
    }
    SetDynamicLen();
    SetPrecision(oriPrecision);
    Simple();  // 化简返回
    return *this;
}

HPFloat &HPFloat::operator=(const char *num)
{
    std::string tmp = num;
    *this = tmp;
    return *this;
}

HPFloat abs(const HPFloat &op)
{
    HPFloat ans(op);
    ans.symbol_ = false;
    return ans;
}

bool operator==(const HPFloat &op1, const HPFloat &op2)
{  // 先判断符号、数量级、已用精度
    if (!(op1.digit_ == op2.digit_ && op1.dynamicLen_ == op2.dynamicLen_ && op1.symbol_ == op2.symbol_))
    {
        return false;
    }
    int32_t length = op1.dynamicLen_;
    for (int32_t i = 0; i <= length; i++)
    {
        if (op1.num_[i] != op2.num_[i])
        {  // 数位一一比较
            return false;
        }
    }
    return true;  // 全部通过，输出1
}

bool operator!=(const HPFloat &op1, const HPFloat &op2) { return !(op1 == op2); }

bool operator>(const HPFloat &op1, const HPFloat &op2)
{
    if (op1 == ZERO && op2 == ZERO)
    {
        return false;
    }
    if (op1 == ZERO && op2 != ZERO)
    {
        return op2.symbol_;
    }
    if (op2 == ZERO)
    {
        return !op1.symbol_;
    }
    // 符号不同情况
    if (op1.symbol_ != op2.symbol_)
    {
        return op2.symbol_;
    }
    if (op1.digit_ != op2.digit_)
    {
        return (op1.digit_ > op2.digit_) && !op1.symbol_;
    }
    int32_t len1 = op1.dynamicLen_;
    int32_t len2 = op2.dynamicLen_;
    int32_t minLen = std::min(len1, len2);
    // 从最高数量级开始比较
    for (int32_t i = 0; i <= minLen; i++)
    {
        if (op1.num_[len1 - i] != op2.num_[len2 - i])
        {
            return (op1.num_[len1 - i] > op2.num_[len2 - i]) && !op1.symbol_;
        }
    }
    // 公有数量级内两者相等，比较有效数量级
    if (len1 > len2)
    {
        return !op1.symbol_;
    }
    return op1.symbol_;
}

bool operator>=(const HPFloat &op1, const HPFloat &op2) { return (op1 > op2 || op1 == op2); }

bool operator<(const HPFloat &op1, const HPFloat &op2) { return !(op1 >= op2); }

bool operator<=(const HPFloat &op1, const HPFloat &op2) { return !(op1 > op2); }

HPFloat operator-(const HPFloat &op)
{
    HPFloat ans(op);
    ans.symbol_ = !op.symbol_;
    return ans;
}

HPFloat operator+(const HPFloat &op1, const HPFloat &op2)
{
    HPFloat ans(op1);
    ans += op2;
    return ans;
}

HPFloat operator+=(HPFloat &op1, const HPFloat &op2)
{
    // 按照加数和被加数最大精度设置结果最大精度
    op1.SetPrecision(std::max(op1.Len(), op2.Len()));
    // 如果符号不同，传递给减法
    if (op1.symbol_ != op2.symbol_)
    {
        HPFloat op3(op2);
        if (op1.symbol_)
        {
            op1.symbol_ = false;
            op3 -= op1;
            op1 = op3;
            return op1;
        }
        else
        {
            op3.symbol_ = false;
            op1 -= op3;
            return op1;
        }
    }
    // 如果被加数为零，直接返回
    if (op1 == ZERO)
    {
        op1 = op2;
        return op1;
    }
    else if (op2 == ZERO)
    {
        return op1;
    }
    int32_t minDig = op2.MinDig();
    // 一个个传入到DigAdd
    for (int32_t i = 0; i <= op2.dynamicLen_; i++)
    {
        op1.DigAdd(op2.num_[i], minDig + i);
    }
    return op1;
}

HPFloat operator-(const HPFloat &op1, const HPFloat &op2)
{
    // 符号不同就传递给加法
    if (op1.symbol_ != op2.symbol_)
    {
        if (op1.symbol_)
        {
            HPFloat ans(op1);
            ans.symbol_ = false;
            ans += op2;
            ans.symbol_ = true;
            return ans;
        }
        else
        {
            HPFloat ans(op2);
            ans.symbol_ = false;
            ans += op1;
            return ans;
        }
    }
    // 减法不判断op是否为0，因为会确保是大减小，存在0计算量很小
    bool bg = (op1 >= op2);  // 确保传递时不发生小减大
    HPFloat ans(bg ? op1 : op2);
    HPFloat op((!bg) ? op1 : op2);
    ans.symbol_ = !bg;
    int32_t minDig = op.MinDig();
    for (int32_t i = 0; i <= op.dynamicLen_; i++)
    {  // 循环传递参数
        ans.DigSub(op.num_[i], minDig + i);
    }
    return ans;
}

HPFloat operator-=(HPFloat &op1, const HPFloat &op2)
{
    HPFloat ans(op1);
    ans = op1 - op2;
    op1 = ans;
    return ans;
}

HPFloat operator<<(const HPFloat &op, const int n)
{
    if (n < 0)
    {
        return op >> -n;
    }
    HPFloat ans{op};
    ans.digit_ += n;
    return ans;
}

HPFloat operator>>(const HPFloat &op, const int n)
{
    if (n < 0)
    {
        return op << -n;
    }
    HPFloat ans{op};
    ans.digit_ -= n;
    return ans;
}

}  // namespace Utils
}  // namespace Analysis
