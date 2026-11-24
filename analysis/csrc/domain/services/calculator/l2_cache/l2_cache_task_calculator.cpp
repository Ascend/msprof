/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 * PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/services/calculator/l2_cache/l2_cache_task_calculator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_l2_cache.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/l2_cache/l2_cache_sample_events.h"
#include "analysis/csrc/domain/services/parser/l2_cache/include/l2_cache_parser.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
struct EventIndex
{
    size_t index;
    int coefficient;
};
using MetricIndexes = std::array<std::vector<EventIndex>, 3>;

// Aligns with analysis/msconfig/l2_cache_config.py DATA (request / hit / victim
// token lists).
const std::unordered_map<uint32_t, std::array<const char *, 3>> &L2CacheMetricTokenTable()
{
    static const std::unordered_map<uint32_t, std::array<const char *, 3>> kTable = {
        {CHIP_V2_1_0, {{"0x59", "0x5b", "0x5c"}}},
        {CHIP_V3_1_0, {{"0x78,0x79", "0x6a", "0x71"}}},
        {CHIP_V3_2_0, {{"0x78,0x79", "0x6a", "0x71"}}},
        {CHIP_V3_3_0, {{"0x78,0x79", "0x6a", "0x71"}}},
        {CHIP_V4_1_0, {{"0xfb,0xfc", "0x90,0x91", "0x9c"}}},
        {CHIP_V1_1_1, {{"0xfb,0xfc", "0x90,0x91", "0x9c"}}},
        {CHIP_V1_1_2, {{"0xfb,0xfc", "0x90,0x91", "0x9c"}}},
        {CHIP_V1_1_3, {{"0xfb,0xfc", "0x90,0x91", "0x9c"}}},
        {CHIP_V6_1_0, {{"0x00", "0x00,-0x81,-0x82,-0x83,-0x74,-0x75", "0x74,0x75"}}},
        {CHIP_V6_2_0, {{"0x00", "0x00,-0x81,-0x82,-0x83,-0x74,-0x75", "0x74,0x75"}}},
        {CHIP_V6_1_1, {{"0x00", "0x00,-0x81,-0x82,-0x83,-0x74,-0x75", "0x74,0x75"}}},
    };
    return kTable;
}

bool BuildMetricIndexes(uint32_t chipId, const std::vector<std::string> &events, MetricIndexes &indexes)
{
    const auto &table = L2CacheMetricTokenTable();
    const auto found = table.find(chipId);
    if (found == table.end())
    {
        WARN("invalid platform % for l2 cache profiling", chipId);
        return false;
    }
    for (size_t metric = 0; metric < found->second.size(); ++metric)
    {
        for (const auto &token : Utils::Split(found->second[metric], ","))
        {
            if (token.empty())
            {
                continue;
            }
            const bool negative = token.front() == '-';
            const auto name = negative ? token.substr(1) : token;
            const auto it = std::find(events.begin(), events.end(), name);
            if (it == events.end())
            {
                WARN("invalid l2 cache events, in platform: %", chipId);
                return false;
            }
            indexes[metric].push_back({static_cast<size_t>(std::distance(events.begin(), it)), negative ? -1 : 1});
        }
    }
    return true;
}

bool SumEvents(const uint64_t counts[L2_CACHE_EVENT_SLOT_COUNT], size_t countSize,
               const std::vector<EventIndex> &indexes, int64_t &result)
{
    __int128 total = 0;
    for (const auto &item : indexes)
    {
        if (item.index >= countSize)
        {
            return false;
        }
        total += static_cast<__int128>(counts[item.index]) * item.coefficient;
    }
    if (total < std::numeric_limits<int64_t>::min() || total > std::numeric_limits<int64_t>::max())
    {
        return false;
    }
    result = static_cast<int64_t>(total);
    return true;
}

// Round a binary64 ratio to 6 decimal places with half-to-even, matching
// Python round(x, 6) and the classic-locale ostringstream path it replaced.
// std::round / nearbyint(ratio * 1e6) are not used: std::round is
// half-away-from-zero and nearbyint follows the current rounding mode, both
// of which diverge from Python on some midpoints.
//
// 10^6 = 15625 * 2^6. For |ratio| >= 2^33, a 6-decimal rounding converted
// back to double is still the original value. Otherwise frexp yields a
// fraction in [0.5, 1) and an exponent; exponent <= -21 means
// |ratio| < 2^{-21} < 0.5e-6, which rounds to a signed zero. In the middle
// interval the 53-bit mantissa is an exact integer, mantissa * 15625 is
// computed in unsigned __int128, and the product is shifted right by
// s = 47 - exponent (s is in 14..67). The remainder is compared with
// 2^(s-1): above half rounds up, an exact midpoint rounds up only when the
// quotient is odd. The quotient is less than 2^53, so
// double(quot) / 1000000.0 is exact. copysign restores the original sign,
// including negative zero.
double RoundRatioToSixDecimals(double ratio)
{
    const double absolute = std::fabs(ratio);
    // 2^33 = 8589934592. Below this bound the 6-decimal integer lies in
    // [0, 2^53); at or above it the original double is already the result.
    constexpr double kTwoPow33 = 8589934592.0;
    if (absolute >= kTwoPow33)
    {
        return ratio;
    }

    int exponent = 0;
    const double fraction = std::frexp(absolute, &exponent);
    if (exponent <= -21)
    {
        return std::copysign(0.0, ratio);
    }

    // fraction * 2^53 is an exact integer in [2^52, 2^53) for this interval
    // (and 0 when ratio is zero, which also falls through this path).
    const uint64_t mantissa = static_cast<uint64_t>(std::ldexp(fraction, 53));
    const int shift = 47 - exponent;
    const unsigned __int128 product = static_cast<unsigned __int128>(mantissa) * 15625u;
    const unsigned __int128 quotientWide = product >> shift;
    const unsigned __int128 remainder = product - (quotientWide << shift);
    const unsigned __int128 half = static_cast<unsigned __int128>(1) << (shift - 1);
    uint64_t quotient = static_cast<uint64_t>(quotientWide);
    if (remainder > half || (remainder == half && (quotient & 1ULL) != 0))
    {
        ++quotient;
    }
    return std::copysign(static_cast<double>(quotient) / 1000000.0, ratio);
}

bool Rate(int64_t value, int64_t request, double &out)
{
    if (request == 0)
    {
        out = 0.0;
        return true;
    }
    // Keep the existing conversion: each operand is narrowed to binary64
    // before the division, matching the previous Rate ratio expression.
    const double ratio = static_cast<double>(value) / static_cast<double>(request);
    out = RoundRatioToSixDecimals(ratio);
    return true;
}

bool CalculateRow(const HalL2CacheRaw &row, const MetricIndexes &indexes, L2CacheSummaryRow &summary)
{
    std::array<int64_t, 3> sums{};
    for (size_t metric = 0; metric < sums.size(); ++metric)
    {
        if (!SumEvents(row.events, L2_CACHE_EVENT_SLOT_COUNT, indexes[metric], sums[metric]))
        {
            return false;
        }
    }
    summary.taskType = row.taskType;
    summary.streamId = row.streamId;
    summary.taskId = row.taskId;
    if (!Rate(sums[1], sums[0], summary.hitRate) || !Rate(sums[2], sums[0], summary.victimRate))
    {
        return false;
    }
    return true;
}
}  // namespace

uint32_t L2CacheTaskCalculator::ProcessEntry(Infra::DataInventory &inventory, const Infra::Context &context)
{
    const auto &device = static_cast<const DeviceContext &>(context);
    SampleInfo sampleInfo;
    device.Getter(sampleInfo);
    const auto events = ParseL2CacheEventsFromSample(sampleInfo);
    if (events.empty())
    {
        INFO("l2 cache events are empty, skip");
        return ANALYSIS_OK;
    }

    MetricIndexes indexes;
    const auto chipId = device.GetChipID();
    if (!BuildMetricIndexes(chipId, events, indexes))
    {
        return ANALYSIS_OK;
    }

    const auto parsed = inventory.GetPtr<std::vector<HalL2CacheRaw>>();
    if (!parsed || parsed->empty())
    {
        WARN("l2 cache parser data is empty");
        return ANALYSIS_OK;
    }

    std::vector<L2CacheSummaryRow> summary;
    summary.reserve(parsed->size());
    for (const auto &row : *parsed)
    {
        L2CacheSummaryRow result;
        if (!CalculateRow(row, indexes, result))
        {
            ERROR("invalid or overflowing l2 cache event counts, in platform: %", chipId);
            return ANALYSIS_ERROR;
        }
        summary.emplace_back(result);
    }
    std::shared_ptr<std::vector<L2CacheSummaryRow>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<L2CacheSummaryRow>, ANALYSIS_ERROR, std::move(summary));
    return inventory.Inject(data) ? ANALYSIS_OK : ANALYSIS_ERROR;
}

// REGISTER_PROCESS_SEQUENCE(L2CacheTaskCalculator, true, L2CacheParser);
// REGISTER_PROCESS_DEPENDENT_DATA(L2CacheTaskCalculator, std::vector<HalL2CacheRaw>);
// REGISTER_PROCESS_SUPPORT_CHIP(L2CacheTaskCalculator, CHIP_V2_1_0, CHIP_V3_1_0, CHIP_V3_2_0, CHIP_V3_3_0, CHIP_V4_1_0,
//                               CHIP_V1_1_1, CHIP_V1_1_2, CHIP_V1_1_3, CHIP_V6_1_0, CHIP_V6_2_0, CHIP_V6_1_1);
}  // namespace Domain
}  // namespace Analysis
