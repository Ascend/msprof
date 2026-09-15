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

#include "analysis/csrc/domain/services/parser/llc/include/llc_parser.h"

#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>

#include "analysis/csrc/domain/entities/hal/include/hal_llc_pcie.h"
#include "analysis/csrc/domain/services/parser/llc/include/llc_pcie_cpp_enable.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "securec.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const uint32_t LLC_RECORD_SIZE = 24;
const double UINT64_UPPER_BOUND = 18446744073709551616.0;
const std::string LLC_VERSION_ONE = "1.0";
const std::string LLC_VERSION_TWO = "2.0";

struct LlcV1Wire
{
    uint64_t timestamp;
    uint64_t count;
    uint32_t event;
    uint32_t l3tid;
};

struct LlcV2Wire
{
    uint32_t reserved;
    uint32_t count;
    uint32_t event;
    uint32_t l3tid;
    uint64_t timestamp;
};

static_assert(sizeof(LlcV1Wire) == LLC_RECORD_SIZE, "LLC V1 wire layout must match QQII");
static_assert(sizeof(LlcV2Wire) == LLC_RECORD_SIZE, "LLC V2 wire layout must match IIIIQ");
static_assert(offsetof(LlcV1Wire, timestamp) == 0 && offsetof(LlcV1Wire, count) == 8 &&
                  offsetof(LlcV1Wire, event) == 16 && offsetof(LlcV1Wire, l3tid) == 20,
              "LLC V1 wire field offsets must match QQII");
static_assert(offsetof(LlcV2Wire, reserved) == 0 && offsetof(LlcV2Wire, count) == 4 &&
                  offsetof(LlcV2Wire, event) == 8 && offsetof(LlcV2Wire, l3tid) == 12 &&
                  offsetof(LlcV2Wire, timestamp) == 16,
              "LLC V2 wire field offsets must match IIIIQ");
static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "LLC wire parsing requires a little-endian target");
static_assert(std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits == 53 &&
                  std::numeric_limits<double>::max_exponent == 1024,
              "LLC V1 timestamp conversion requires IEEE-754 binary64");
#ifdef __FAST_MATH__
#error "LLC V1 timestamp conversion must not be compiled with fast-math"
#endif

bool AddWithoutOverflow(uint64_t lhs, uint64_t rhs, uint64_t &result)
{
    if (lhs > std::numeric_limits<uint64_t>::max() - rhs)
    {
        return false;
    }
    result = lhs + rhs;
    return true;
}

bool ValidateV1TimeContext(const HostStartLog &hostStart, const DeviceStartLog &deviceStart,
                           const DeviceStartInfo &startInfo)
{
    if (hostStart.clockMonotonicRaw == 0 || deviceStart.clockMonotonicRaw == 0 || startInfo.collectionTimeBegin == 0 ||
        startInfo.clockMonotonicRaw == 0)
    {
        ERROR("LLC V1 time context is incomplete.");
        return false;
    }
    if (hostStart.clockMonotonicRaw < deviceStart.clockMonotonicRaw)
    {
        ERROR("LLC V1 host monotonic timestamp precedes device start timestamp.");
        return false;
    }
    if (startInfo.collectionTimeBegin > std::numeric_limits<uint64_t>::max() / NS_TO_US)
    {
        ERROR("LLC V1 collection epoch is out of range.");
        return false;
    }
    const uint64_t collectionTimeNs = startInfo.collectionTimeBegin * NS_TO_US;
    if (collectionTimeNs < startInfo.clockMonotonicRaw)
    {
        ERROR("LLC V1 collection epoch precedes its monotonic timestamp.");
        return false;
    }
    return true;
}

bool ConvertV1Timestamp(uint64_t rawTimestamp, const HostStartLog &hostStart, const DeviceStartLog &deviceStart,
                        const DeviceStartInfo &startInfo, uint64_t &timestamp)
{
    if (!ValidateV1TimeContext(hostStart, deviceStart, startInfo))
    {
        return false;
    }
    if (rawTimestamp > std::numeric_limits<uint64_t>::max() / NS_TO_US)
    {
        return false;
    }
    const uint64_t rawNs = rawTimestamp * NS_TO_US;
    uint64_t samplingTimestamp;
    if (!AddWithoutOverflow(rawNs, deviceStart.clockMonotonicRaw, samplingTimestamp))
    {
        return false;
    }

    const double hostMonotonicSeconds = static_cast<double>(hostStart.clockMonotonicRaw) / NANO_SECOND;
    const double deviceStartSeconds = static_cast<double>(deviceStart.clockMonotonicRaw) / NANO_SECOND;
    const double deltaSeconds = hostMonotonicSeconds - deviceStartSeconds;
    const double deltaNs = deltaSeconds * NANO_SECOND;
    const double monotonicTimestamp = static_cast<double>(samplingTimestamp) + deltaNs;
    const double collectionTimeUs = static_cast<double>(startInfo.collectionTimeBegin);
    const double collectionRawUs = static_cast<double>(startInfo.clockMonotonicRaw) / NS_TO_US;
    const double localOffsetUs = collectionTimeUs - collectionRawUs;
    const double localOffsetNs = localOffsetUs * NS_TO_US;
    const double absoluteTimestamp = monotonicTimestamp + localOffsetNs;
    if (!std::isfinite(absoluteTimestamp) || absoluteTimestamp < 0.0 || absoluteTimestamp >= UINT64_UPPER_BOUND)
    {
        return false;
    }
    timestamp = static_cast<uint64_t>(absoluteTimestamp);
    return true;
}

bool ParseV1Record(const uint8_t *raw, uint64_t index, const HostStartLog &hostStart, const DeviceStartLog &deviceStart,
                   const DeviceStartInfo &startInfo, HalLlcData &output)
{
    LlcV1Wire wire{};
    if (memcpy_s(&wire, sizeof(wire), raw, sizeof(wire)) != EOK)
    {
        ERROR("Copy LLC V1 record failed at record %.", index);
        return false;
    }
    if (!ConvertV1Timestamp(wire.timestamp, hostStart, deviceStart, startInfo, output.timestamp))
    {
        ERROR("Convert LLC V1 timestamp failed at record %.", index);
        return false;
    }
    output.count = wire.count;
    output.event = wire.event;
    output.l3tid = wire.l3tid;
    return true;
}

bool ParseV2Record(const uint8_t *raw, uint64_t index, HalLlcData &output)
{
    LlcV2Wire wire{};
    if (memcpy_s(&wire, sizeof(wire), raw, sizeof(wire)) != EOK)
    {
        ERROR("Copy LLC V2 record failed at record %.", index);
        return false;
    }
    output.timestamp = wire.timestamp;
    output.count = wire.count;
    output.event = wire.event;
    output.l3tid = wire.l3tid;
    return true;
}
}  // namespace

std::vector<std::string> LlcParser::GetFilePattern() { return filePrefix_; }

uint32_t LlcParser::GetTrunkSize() { return LLC_RECORD_SIZE; }

uint32_t LlcParser::ParseData(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    const auto *deviceContext = dynamic_cast<const DeviceContext *>(&context);
    if (deviceContext == nullptr)
    {
        ERROR("LLC parser requires DeviceContext.");
        return ANALYSIS_ERROR;
    }
    DeviceInfo deviceInfo{};
    HostStartLog hostStart{};
    DeviceStartLog deviceStart{};
    DeviceStartInfo startInfo{};
    deviceContext->Getter(deviceInfo);
    deviceContext->Getter(hostStart);
    deviceContext->Getter(deviceStart);
    deviceContext->Getter(startInfo);
    if (binaryDataSize > 0 && binaryData == nullptr)
    {
        ERROR("LLC binary data is null, data size: %.", binaryDataSize);
        return ANALYSIS_ERROR;
    }
    if (binaryDataSize % LLC_RECORD_SIZE != 0)
    {
        ERROR("Invalid LLC data size: %, record size: %.", binaryDataSize, LLC_RECORD_SIZE);
        return ANALYSIS_ERROR;
    }

    const uint64_t recordCount = binaryDataSize / LLC_RECORD_SIZE;
    std::vector<HalLlcData> records;
    if (!Utils::Resize(records, recordCount))
    {
        ERROR("Resize for LLC data failed.");
        return ANALYSIS_ERROR;
    }

    const bool useV2 = deviceInfo.collectionVersion == LLC_VERSION_TWO;
    if (!useV2 && deviceInfo.collectionVersion != LLC_VERSION_ONE && deviceInfo.collectionVersion != NA)
    {
        WARN("LLC collection version % is not found, use 1.0 parser.", deviceInfo.collectionVersion);
    }
    for (uint64_t index = 0; index < recordCount; ++index)
    {
        const uint8_t *raw = binaryData.get() + index * LLC_RECORD_SIZE;
        auto &output = records[index];
        output.deviceId = deviceInfo.deviceId;
        const bool parsed = useV2 ? ParseV2Record(raw, index, output)
                                  : ParseV1Record(raw, index, hostStart, deviceStart, startInfo, output);
        if (!parsed)
        {
            return ANALYSIS_ERROR;
        }
    }

    std::shared_ptr<std::vector<HalLlcData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalLlcData>, ANALYSIS_ERROR, std::move(records));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

// REGISTER_PROCESS_SEQUENCE(LlcParser, true);
// REGISTER_PROCESS_SUPPORT_CHIP(LlcParser, CHIP_V2_1_0, CHIP_V3_1_0, CHIP_V3_2_0, CHIP_V3_3_0, CHIP_V4_1_0,
//                               CHIP_V1_1_1, CHIP_V1_1_2, CHIP_V1_1_3, CHIP_V6_1_0, CHIP_V6_2_0);
}  // namespace Domain
}  // namespace Analysis
