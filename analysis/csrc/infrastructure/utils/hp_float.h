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

#ifndef ANALYSIS_UTILS_HP_FLOAT_H
#define ANALYSIS_UTILS_HP_FLOAT_H

#include <string>
#include <vector>
#include <stdint.h>
namespace Analysis {
namespace Utils {
/*
==============HPFloat:高精度浮点类型===================
1.类设计
基于数量级（多项式）运算
支持运算:+,-,<<（乘以10的倍数）>>（乘以10的倍数）,+=,-=,-(负号),=
构造函数支持:string,HPFloat,整型与浮点型
除HPFloat构造时与参数精度相同，其他类型参数构造均为30位精度，SetPrecision方法可修改精度
====================================================
2.使用方法：
2.1定义&构造：
HPFloat a;
std::string str = "2.33";
double num = 2.33;
long long num2 = 233;
HPFloat b(a);
HPFloat c(str);
HPFloat d(num);
HPFloat e(num2);
2.2 运算：
支持运算:+,-,<<（乘以10的倍数）>>（乘以10的倍数）,+=,-=,-(负号),=
使用方法和c++基本类一致
a += b;
auto m = c - d;
===================================================
3.特殊情况
3.1特殊构造&赋值：
char a = 123;
unsigned char b = 123;
std::string c = "123";
HPFloat m(a);
HPFloat n(b);
HPFloat o(c);
m,n和o存储的实际数值都是123，实际上char(unsigned char)可以视为一个数值而不是字符，例如INT8就是signed char的别名
3.2 精度继承：
初HPFloat操作赋值，其他赋值操作不改变类原有精度，赋值过大数会发生精度溢出
HPFloat赋值操作由于等价于复制构造，因此继承右侧精
HPFloat a;
a.SetPrecision(5);
a = 123.233  这里实际存储数值123.23(四舍五入舍去，小数点末尾0不存储）
a = 123.235  这里实际存储数值123.24(四舍五入舍去，小数点末尾0不存储）
a = 123233   这里实际存储数值123230(与c++ double类型丢失非小数行为一致，四舍五入精度前一位，之后数字归零
a = 123235   这里实际存储数值123240(与c++ double类型丢失非小数行为一致，四舍五入精度前一位，之后数字归零
*/
class HPFloat {
public:
    HPFloat();
    template<typename T>
    HPFloat(T value);
    HPFloat(const HPFloat &value);
    HPFloat &operator=(const HPFloat &num);
    HPFloat &operator=(long long num);
    HPFloat &operator=(unsigned long long num);
    HPFloat &operator=(long num);
    HPFloat &operator=(unsigned long num);
    HPFloat &operator=(int num);
    HPFloat &operator=(unsigned int num);
    HPFloat &operator=(const std::string &num);
    HPFloat &operator=(const char *num);
    template<typename T>
    HPFloat &operator=(const T &op);
    // 由于重载运算符方法内部需要访问参数的私有变量，例如num_，所以需要使用友元函数
    // 绝对值运算使用小写字母是因为要重载abs函数
    friend HPFloat abs(const HPFloat &op1);
    friend HPFloat operator+(const HPFloat &op1, const HPFloat &op2);
    friend HPFloat operator-(const HPFloat &op1, const HPFloat &op2);
    // 临时方案，十进制左移位，相当于乘以10的n次幂
    friend HPFloat operator<<(const HPFloat &op, int n);
    // 临时方案，十进制右移位，相当于除以10的n次幂
    friend HPFloat operator>>(const HPFloat &op, int n);
    // 负号
    friend HPFloat operator-(const HPFloat &op);
    friend HPFloat operator+=(HPFloat &op1, const HPFloat &op2);
    friend HPFloat operator-=(HPFloat &op1, const HPFloat &op2);
    friend bool operator==(const HPFloat &op1, const HPFloat &op2);
    friend bool operator!=(const HPFloat &op1, const HPFloat &op2);
    friend bool operator>(const HPFloat &op1, const HPFloat &op2);
    friend bool operator>=(const HPFloat &op1, const HPFloat &op2);
    friend bool operator<(const HPFloat &op1, const HPFloat &op2);
    friend bool operator<=(const HPFloat &op1, const HPFloat &op2);
    // 定义精度, 只接受正整数参数
    void SetPrecision(int32_t length);
    void SetPrecision(unsigned long length);
    void SetPrecision(long long length);
    void SetPrecision(double length) = delete;
    void SetPrecision(float length) = delete;
    template<typename T>
    void SetPrecision(T length);
    // 数据位数(有效位数+未占用位，不包含小数点）
    int32_t Len() const;
    // 输出字符串格式
    std::string Str();
    // 输出double格式，不建议使用
    double Double();
    // 输出uint64格式
    uint64_t Uint64();
    // 量化，指定保留n位小数，不足n位不处理, 默认保留3位小数
    void Quantize(int32_t n = 3);
private:
    // 清空数据，保留精度
    void Clear();
    // 设置动态长度变量，即num_中实际使用位数
    void SetDynamicLen();
    void Simple();
    // 整体退位,用于将最小数量级位数按照四舍五入舍弃，处理精度溢出使用
    void BackSpace();
    // 移位操作，用于位数对齐
    void MoveForward(int32_t step);
    void MoveBackward(int32_t step);
    // 返回最小数量级
    int32_t MinDig() const;
    // 返回理论最小数量级
    int32_t TheoreticalMinDig() const;
    // 坐标加法,基于num_坐标进行运算
    void CoorAdd(signed char op, int32_t psi);
    // 数量级加法,基于多项式数量级进行运算
    void DigAdd(signed char op, int32_t n);
    // 坐标减法核心
    void CoreCoorSub(signed char op, int32_t psi);
    // 坐标减法
    void CoorSub(signed char op, int32_t psi);
    // 数量级减法,基于多项式数量级进行运算
    void DigSub(signed char op, int32_t d);
    // 化简字符串用于构造函数
    bool SimplifyStr(std::string &str);
private:
    // 默认精度
    int32_t defaultPrecision_{30};
    // 允许最小精度
    int32_t minPrecision_{3};
    // 允许最大精度
    int32_t maxPrecision_{100};
    // 最大数量级,传入数据按数量级（多项式）表示，例如a1*10^2+a2*10^1+a3*10^0+a4*10^-1，本例中digit_=2
    int32_t digit_{0};
    // 数位,用于倒序存储数据
    std::vector<signed char> num_;
    // 符号, 正数为false，负数为true
    bool symbol_{false};
    // 动态长度，表示num_有效位数长度（计算过程中不恒等于占用位数）
    int32_t dynamicLen_{0};
};

template<typename T>
HPFloat::HPFloat(T value)
{
    for (int32_t i = 0; i < defaultPrecision_; i++) {
        num_.emplace_back(0);
    }
    *this = value;
}

template<typename T>
HPFloat &HPFloat::operator=(const T &op)
{
    *this = std::to_string(op);
    return *this;
}

template<typename T>
void Utils::HPFloat::SetPrecision(T length)
{
    SetPrecision(static_cast<int32_t>(length));
}

const HPFloat ZERO(0);

}
}

#endif // ANALYSIS_UTILS_HP_FLOAT_H