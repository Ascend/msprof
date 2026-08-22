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

#ifndef ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_TEST_SPY_H
#define ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_TEST_SPY_H
#include <cstdint>
#include <iostream>
#include <map>
#include <string>

namespace Analysis
{

class ProcessSpy final
{
   public:
    explicit ProcessSpy(const std::string& processName) : processName_(processName)
    {
        // 可以通过打印定位问题，例如：std::cout << processName_ << " Enter!" << std::endl
    }
    ~ProcessSpy()
    {
        // 可以通过打印定位问题，例如：std::cout << processName_ << " Out!" << std::endl
    }
    static uint32_t GetResult(const std::string& processName);
    static void SetResult(const std::string& processName, uint32_t result);
    static void ClearResult();

   private:
    static std::map<std::string, uint32_t>& GetResultMap();

   private:
    std::string processName_;
};

#define PROCESS_TRACE(processName) ProcessSpy processTrace(#processName)

}  // namespace Analysis
#endif
