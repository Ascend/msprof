/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
#ifndef ANALYSIS_APPLICATION_PIPELINE_FLAGS_H
#define ANALYSIS_APPLICATION_PIPELINE_FLAGS_H

namespace Analysis
{
namespace Application
{
// Numeric values form the Python/native pipeline interface contract.
constexpr int PIPELINE_CANN_TRACE = 0x01;
constexpr int PIPELINE_DEVICE_DATA = 0x02;
constexpr int PIPELINE_DB = 0x04;
constexpr int PIPELINE_TIMELINE = 0x08;
constexpr int PIPELINE_SUMMARY = 0x10;
constexpr int PIPELINE_ALL =
    PIPELINE_CANN_TRACE | PIPELINE_DEVICE_DATA | PIPELINE_DB | PIPELINE_TIMELINE | PIPELINE_SUMMARY;

constexpr bool IsValidPipelineFlags(int flags) { return flags != 0 && (flags & ~PIPELINE_ALL) == 0; }
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_PIPELINE_FLAGS_H
