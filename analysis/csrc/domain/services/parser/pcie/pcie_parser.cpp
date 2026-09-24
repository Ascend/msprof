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

#include "analysis/csrc/domain/services/parser/pcie/include/pcie_parser.h"

#include <memory>
#include <utility>

#include "analysis/csrc/domain/entities/hal/include/hal_llc_pcie.h"
#include "analysis/csrc/domain/services/parser/llc/include/llc_pcie_cpp_enable.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "securec.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const uint32_t PCIE_RECORD_SIZE = 96;

struct PcieWire
{
    uint64_t timestamp;
    uint32_t deviceId;
    uint32_t txPBandwidthMin;
    uint32_t txPBandwidthMax;
    uint32_t txPBandwidthAvg;
    uint32_t txNpBandwidthMin;
    uint32_t txNpBandwidthMax;
    uint32_t txNpBandwidthAvg;
    uint32_t txCplBandwidthMin;
    uint32_t txCplBandwidthMax;
    uint32_t txCplBandwidthAvg;
    uint32_t txNpLatencyMin;
    uint32_t txNpLatencyMax;
    uint32_t txNpLatencyAvg;
    uint32_t rxPBandwidthMin;
    uint32_t rxPBandwidthMax;
    uint32_t rxPBandwidthAvg;
    uint32_t rxNpBandwidthMin;
    uint32_t rxNpBandwidthMax;
    uint32_t rxNpBandwidthAvg;
    uint32_t rxCplBandwidthMin;
    uint32_t rxCplBandwidthMax;
    uint32_t rxCplBandwidthAvg;
};

static_assert(sizeof(PcieWire) == PCIE_RECORD_SIZE, "PCIe wire layout must match Q22I");

void CopyMetrics(const PcieWire &wire, HalPcieData &output)
{
    output.txPBandwidthMin = wire.txPBandwidthMin;
    output.txPBandwidthMax = wire.txPBandwidthMax;
    output.txPBandwidthAvg = wire.txPBandwidthAvg;
    output.txNpBandwidthMin = wire.txNpBandwidthMin;
    output.txNpBandwidthMax = wire.txNpBandwidthMax;
    output.txNpBandwidthAvg = wire.txNpBandwidthAvg;
    output.txCplBandwidthMin = wire.txCplBandwidthMin;
    output.txCplBandwidthMax = wire.txCplBandwidthMax;
    output.txCplBandwidthAvg = wire.txCplBandwidthAvg;
    output.txNpLatencyMin = wire.txNpLatencyMin;
    output.txNpLatencyMax = wire.txNpLatencyMax;
    output.txNpLatencyAvg = wire.txNpLatencyAvg;
    output.rxPBandwidthMin = wire.rxPBandwidthMin;
    output.rxPBandwidthMax = wire.rxPBandwidthMax;
    output.rxPBandwidthAvg = wire.rxPBandwidthAvg;
    output.rxNpBandwidthMin = wire.rxNpBandwidthMin;
    output.rxNpBandwidthMax = wire.rxNpBandwidthMax;
    output.rxNpBandwidthAvg = wire.rxNpBandwidthAvg;
    output.rxCplBandwidthMin = wire.rxCplBandwidthMin;
    output.rxCplBandwidthMax = wire.rxCplBandwidthMax;
    output.rxCplBandwidthAvg = wire.rxCplBandwidthAvg;
}
}  // namespace

std::vector<std::string> PcieParser::GetFilePattern() { return filePrefix_; }

uint32_t PcieParser::GetTrunkSize() { return PCIE_RECORD_SIZE; }

uint32_t PcieParser::ParseData(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    const auto *deviceContext = dynamic_cast<const DeviceContext *>(&context);
    if (deviceContext == nullptr)
    {
        ERROR("PCIe parser requires DeviceContext.");
        return ANALYSIS_ERROR;
    }
    DeviceInfo deviceInfo;
    deviceContext->Getter(deviceInfo);
    if (binaryDataSize > 0 && binaryData == nullptr)
    {
        ERROR("PCIe binary data is null, data size: %.", binaryDataSize);
        return ANALYSIS_ERROR;
    }
    if (binaryDataSize % PCIE_RECORD_SIZE != 0)
    {
        ERROR("Invalid PCIe data size: %, record size: %.", binaryDataSize, PCIE_RECORD_SIZE);
        return ANALYSIS_ERROR;
    }

    const uint64_t recordCount = binaryDataSize / PCIE_RECORD_SIZE;
    std::vector<HalPcieData> records;
    if (!Utils::Resize(records, recordCount))
    {
        ERROR("Resize for PCIe data failed.");
        return ANALYSIS_ERROR;
    }

    for (uint64_t index = 0; index < recordCount; ++index)
    {
        PcieWire wire{};
        if (memcpy_s(&wire, sizeof(wire), binaryData.get() + index * PCIE_RECORD_SIZE, sizeof(wire)) != EOK)
        {
            ERROR("Copy PCIe record failed at record %.", index);
            return ANALYSIS_ERROR;
        }
        auto &output = records[index];
        output.timestamp = wire.timestamp;
        output.deviceId = deviceInfo.deviceId;
        CopyMetrics(wire, output);
    }

    std::shared_ptr<std::vector<HalPcieData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalPcieData>, ANALYSIS_ERROR, std::move(records));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

// REGISTER_PROCESS_SEQUENCE(PcieParser, true);
// REGISTER_PROCESS_SUPPORT_CHIP(PcieParser, CHIP_V1_1_0, CHIP_V2_1_0, CHIP_V3_1_0, CHIP_V3_2_0, CHIP_V3_3_0,
//                               CHIP_V4_1_0, CHIP_V6_1_0, CHIP_V6_1_1, CHIP_V6_2_0);
}  // namespace Domain
}  // namespace Analysis
