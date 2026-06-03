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

#ifndef ANALYSIS_DOMAIN_HCCL_STATISTICS_DATA_H
#define ANALYSIS_DOMAIN_HCCL_STATISTICS_DATA_H

#include <limits>
#include <string>
#include <stdint.h>

namespace Analysis {
namespace Domain {
struct HcclStatisticData {
    std::string opType;
    std::string count;
    uint16_t deviceId;
    double totalTime = 0.0;
    double min = std::numeric_limits<double>::infinity();
    double max = 0.0;
    double avg = 0.0;
    double ratio = 0.0;
};
}
}

#endif // ANALYSIS_DOMAIN_HCCL_STATISTICS_DATA_H
