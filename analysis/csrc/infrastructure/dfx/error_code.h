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

#ifndef ANALYSIS_DFX_ERROR_CODE_H
#define ANALYSIS_DFX_ERROR_CODE_H

#include <cstdint>
#include <string>

namespace Analysis
{

const int ANALYSIS_OK = 0;
const int ANALYSIS_ERROR = 1;

const int SERVICE_OFFSET = 24;
const int SEQUENCE_OFFSET = 16;
// define Service id
constexpr uint32_t SERVICE_ID_CONTEXT = 0x30;      // Context Service
constexpr uint32_t SERVICE_ID_PARSER = 0x31;       // Parser Service
constexpr uint32_t SERVICE_ID_MODELING = 0x32;     // Modeling Service
constexpr uint32_t SERVICE_ID_ASSOCIATION = 0x33;  // association Service
constexpr uint32_t SERVICE_ID_PERSISTENCE = 0x34;  // persistence Service

// 第1个8bit为服务/组件ID,第2个8bit为服务/组件内部细分序号，后16个bit为错误码
constexpr uint32_t ERROR_NO(const uint32_t serviceId, const uint32_t seq, const uint32_t error_code)
{
    return ((serviceId << SERVICE_OFFSET) | (seq << SEQUENCE_OFFSET) | (error_code));
}

const uint8_t ASSEMBLE_FAILED = 0;
const uint8_t ASSEMBLE_SUCCESS = 1;
const uint8_t DATA_NOT_EXIST = 2;

}  // namespace Analysis
#endif  // ANALYSIS_DFX_ERROR_CODE_H
