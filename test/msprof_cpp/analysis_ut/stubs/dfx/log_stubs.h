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
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef MSPROF_TEST_ANALYSIS_UT_STUBS_DFX_LOG_STUBS_H
#define MSPROF_TEST_ANALYSIS_UT_STUBS_DFX_LOG_STUBS_H

#include <cstdint>
#include <string>
#include <vector>

namespace Analysis
{
struct TestLogMessage
{
    std::string message;
    std::string level;
    std::string fileName;
    uint32_t line;
};

void StartTestLogCapture();
std::vector<TestLogMessage> StopTestLogCapture();
}  // namespace Analysis

#endif  // MSPROF_TEST_ANALYSIS_UT_STUBS_DFX_LOG_STUBS_H
