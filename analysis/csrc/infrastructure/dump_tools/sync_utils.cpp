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

#include "analysis/csrc/infrastructure/dump_tools/include/sync_utils.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Analysis
{
namespace Infra
{
const std::string& GetTimeStampStr()
{
    const static std::string TIMESTAMP_STR = []() -> std::string
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto tm = std::localtime(&time);
        if (!tm)
        {
            return "99999999999999";  // When localtime is abnormal, the default value is '99999999999999'
        }
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y%m%d%H%M%S");
        return oss.str();
    }();
    return TIMESTAMP_STR;
}
}  // namespace Infra
}  // namespace Analysis
