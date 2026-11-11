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
 * MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef ANALYSIS_DOMAIN_FTRACE_GENERIC_EVENT_DATA_H
#define ANALYSIS_DOMAIN_FTRACE_GENERIC_EVENT_DATA_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Analysis
{
namespace Domain
{
// Value column of GENERIC_EVENT_DATA, mirrors the four typed columns plus the local strId.
enum class GenericFieldKind : uint8_t
{
    INT_VALUE = 0,
    UINT_VALUE = 1,
    FLOAT_VALUE = 2,
    DOUBLE_VALUE = 3,
    STRING_VALUE = 4,
};

struct GenericEventFieldValue
{
    uint32_t fieldIdx = 0;  // GENERIC_EVENT_TYPE_FIELDS.fieldIdx
    GenericFieldKind kind = GenericFieldKind::INT_VALUE;
    int64_t intVal = 0;    // intVal column
    uint64_t uintVal = 0;  // uintVal column
    double realVal = 0.0;  // floatVal / doubleVal column
    std::string strVal;    // interned into StringIds by the saver
};

// One row of GENERIC_EVENTS plus its GENERIC_EVENT_DATA rows.
struct GenericEventData
{
    uint64_t rawTimestamp = 0;  // CLOCK_MONOTONIC_RAW ns, as recorded
    uint64_t timestamp = 0;     // epoch ns
    uint32_t cpuId = 0;
    uint32_t typeId = 0;
    uint64_t globalTid = 0;
    // bit0: this row came from an incomplete cpu file
    // bit1: at least one field failed the layout/bounds check, so its value columns are
    //       placeholders (all zero) rather than decoded data; a real 0 (for example the idle
    //       tid) is only trustworthy on rows without this bit
    // common_flags is exported as an ordinary event field.
    uint32_t flags = 0;
    std::vector<GenericEventFieldValue> values;
};

// GENERIC_EVENT_TYPES + GENERIC_EVENT_TYPE_FIELDS
struct GenericEventTypeField
{
    std::string name;
    int32_t offset = 0;
    int32_t size = 0;
    GenericFieldKind kind = GenericFieldKind::INT_VALUE;
    // __data_loc T[] keeps a u32 locator in the fixed area (low 16 bits = offset from the event
    // payload, high 16 bits = length); __rel_loc measures that offset from the end of the locator.
    bool isDataLoc = false;
    bool isRelLoc = false;
};

struct GenericEventTypeDef
{
    uint32_t typeId = 0;
    std::string system;
    std::string name;
    int32_t kernelId = -1;
    std::string formatHash;
    std::string formatText;  // tracefs format 原文，写入 GENERIC_EVENT_TYPES.ftraceFormat
    std::vector<GenericEventTypeField> fields;
};

struct GenericEventMeta
{
    size_t pageSize = 4096;      // one tracefs sub-buffer
    size_t pageDataOffset = 16;  // data[] starts right after time_stamp + commit
    // Ring buffer pseudo event lengths are kernel dependent. The collector records the values
    // it counted with; these defaults match kernel 6.8 and are the fallback for meta files
    // written by older collectors.
    uint32_t timeExtendLen = 8;
    uint32_t timeStampLen = 8;
    std::vector<GenericEventTypeDef> types;
    std::unordered_map<uint16_t, size_t> typeIndex;  // kernel event id -> types index
};

// Everything the saver needs: type definitions plus decoded events.
struct GenericEventBundle
{
    GenericEventMeta meta;
    std::vector<GenericEventData> events;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_FTRACE_GENERIC_EVENT_DATA_H
