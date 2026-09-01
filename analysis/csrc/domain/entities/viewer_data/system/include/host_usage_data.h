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

#ifndef ANALYSIS_DOMAIN_HOST_USAGE_DATA_H
#define ANALYSIS_DOMAIN_HOST_USAGE_DATA_H

#include <string>

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis
{
namespace Domain
{
struct CpuUsageData : public BasicData
{
    double usage = 0.0;
    std::string cpuNo;
};

struct CpuFreqData : public BasicData
{
    double freq = 0.0;
    std::string cpuNo;
};

struct MemUsageData : public BasicData
{
    double usage = 0.0;
};

struct DiskUsageData : public BasicData
{
    double usage = 0.0;
    double readRate = 0.0;
    double writeRate = 0.0;
};

struct NetWorkUsageData : public BasicData
{
    double usage = 0.0;
    double speed = 0.0;
};

struct OSRuntimeApiData : public BasicData
{
    std::string name;
    uint64_t pid = 0;
    uint64_t tid = 0;
    uint64_t endTime = 0;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_HOST_USAGE_DATA_H
