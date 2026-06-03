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

#ifndef ANALYSIS_DOMAIN_TIME_DURATION_H
#define ANALYSIS_DOMAIN_TIME_DURATION_H

#include <stdint.h>

namespace Analysis {
namespace Domain {
class TimeDuration {
public:
    TimeDuration(uint64_t start, uint64_t end) : start(start), end(end)
    {}

    bool operator<(const TimeDuration &other) const
    {
        return start < other.start || (start == other.start && end < other.end);
    }
public:
    uint64_t start{};
    uint64_t end{};
};
}
}
#endif // ANALYSIS_DOMAIN_TIME_DURATION_H
