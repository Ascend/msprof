/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_DOMAIN_HOST_PLATFORM_DATA_H
#define ANALYSIS_DOMAIN_HOST_PLATFORM_DATA_H

#include <stdint.h>

#include <string>

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis
{
namespace Domain
{
struct NumaLevelsHierarchyData : public BasicData
{
    uint64_t id;
    uint64_t title0_id;
    uint64_t title1_id;
    uint64_t title2_id;
};

struct NumaMetricsData : public BasicData
{
    uint64_t id;
    uint64_t ts;
    double value;
    uint64_t levels_id;
};

struct NumaScalingValuesData : public BasicData
{
    uint64_t id;
    uint64_t level_id;
    double max_value;
};

struct NumaTitlesNamesData : public BasicData
{
    uint64_t id;
    std::string name;
    std::string description;
    uint64_t summary_flag;
    std::string measurement_unit;
    uint64_t unique_id;
};

struct HostCoreThreadData : public BasicData
{
    int64_t id = 0;
    int64_t tid = 0;
    std::string name;
    int64_t processId = 0;
    int64_t parentId = 0;
    int64_t startTs = 0;
    int64_t endTs = 0;
};

struct HostCoreProcessData : public BasicData
{
    int64_t id = 0;
    int64_t pid = 0;
    std::string name;
    int64_t startTs = 0;
    int64_t endTs = 0;
};

struct HostCoreMetricDescData : public BasicData
{
    int64_t id = 0;
    std::string name;
    std::string description;
    std::string measurementUnit;
};

struct HostCoreMetricData : public BasicData
{
    int64_t id = 0;
    int64_t timestamp = 0;
    double value = 0.0;
    int64_t descId = 0;
    int64_t tidId = 0;
    int64_t cpuId = 0;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_HOST_PLATFORM_DATA_H
