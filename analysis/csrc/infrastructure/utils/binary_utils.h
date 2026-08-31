/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_INFRASTRUCTURE_UTILS_BINARY_UTILS_H
#define ANALYSIS_INFRASTRUCTURE_UTILS_BINARY_UTILS_H

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace Analysis
{
namespace Utils
{
/**
 * @brief Read an integral value from a little-endian byte sequence.
 * @note Reads byte by byte, so the input
 * address does not need to be aligned.
 */
template <typename ValueType>
ValueType ReadLittleEndian(const uint8_t *data)
{
    static_assert(std::is_unsigned<ValueType>::value, "ValueType must be an unsigned integer type");
    ValueType value = 0;
    for (size_t index = 0; index < sizeof(ValueType); ++index)
    {
        value |= static_cast<ValueType>(data[index]) << (index * 8U);
    }
    return value;
}
}  // namespace Utils
}  // namespace Analysis

#endif  // ANALYSIS_INFRASTRUCTURE_UTILS_BINARY_UTILS_H
