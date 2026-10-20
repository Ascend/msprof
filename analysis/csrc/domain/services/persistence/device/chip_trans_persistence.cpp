/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include "analysis/csrc/domain/services/persistence/device/chip_trans_persistence.h"

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/services/parser/log/include/stars_soc_profile_parser.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Application;
using namespace Infra;
using namespace Utils;

namespace
{
constexpr uint64_t INVALID_BW = 0xFFFFFFFFFFFFFFFFULL;

// PcieInfoV6: die_id, sys_time(REAL), pcie_write_bandwidth, pcie_read_bandwidth
using V6Row = std::tuple<int32_t, double, uint64_t, uint64_t>;
// PaLinkInfo: pa_link_id, pa_link_traffic_monit_rx(TEXT), pa_link_traffic_monit_tx(TEXT), sys_time(REAL)
using PaRow = std::tuple<int32_t, std::string, std::string, double>;
// PcieInfo: pcie_id, pcie_write_bandwidth, pcie_read_bandwidth, sys_time(REAL)
using PcieRow = std::tuple<int32_t, uint64_t, uint64_t, double>;

using V6Data = std::vector<V6Row>;
using PaData = std::vector<PaRow>;
using PcieData = std::vector<PcieRow>;

bool GenerateChipTransData(const std::vector<HalLogData>& source, const DeviceContext& context, V6Data& v6Data,
                           PaData& paData, PcieData& pcieData, uint64_t& invalidBandwidthCount,
                           uint64_t& invalidTransTypeCount)
{
    if (!Reserve(v6Data, source.size()) || !Reserve(paData, source.size()) || !Reserve(pcieData, source.size()))
    {
        ERROR("Reserve for ChipTrans persistence data failed");
        return false;
    }
    const auto params = context.GetSyscntConversionParams();
    for (const auto& item : source)
    {
        if (item.type != SOC_PROFILE_CHIP_TRANS)
        {
            continue;
        }
        const auto& chipTrans = item.chipTrans;
        const double sysTime = GetTimeFromSyscnt(chipTrans.sysCnt, params).Double();
        switch (chipTrans.transType)
        {
            case CHIP_TRANS_V6:
                // 目前发现 Pcie 结构可能上报 0xFFFFFFFFFFFFFFFF 的异常值，故进行过滤
                if (chipTrans.value1 == INVALID_BW || chipTrans.value2 == INVALID_BW)
                {
                    ++invalidBandwidthCount;
                    continue;
                }
                v6Data.emplace_back(chipTrans.id, sysTime, chipTrans.value1, chipTrans.value2);
                break;
            case CHIP_TRANS_PA:
                paData.emplace_back(chipTrans.id, std::to_string(chipTrans.value1), std::to_string(chipTrans.value2),
                                    sysTime);
                break;
            case CHIP_TRANS_PCIE:
                pcieData.emplace_back(chipTrans.id, chipTrans.value1, chipTrans.value2, sysTime);
                break;
            default:
                ++invalidTransTypeCount;
                break;
        }
    }
    return true;
}

template <typename RowType>
bool SaveChipTransTable(const std::vector<RowType>& data, DBInfo& dbInfo, std::string& dbPath,
                        const std::string& tableName)
{
    dbInfo.tableName = tableName;
    if (!data.empty())
    {
        return SaveData(data, dbInfo, dbPath);
    }
    if (!dbInfo.dbRunner->CreateTable(tableName, dbInfo.database->GetTableCols(tableName)))
    {
        ERROR("Create ChipTrans table failed: %, table: %", dbPath, tableName);
        return false;
    }
    return true;
}

}  // namespace

uint32_t ChipTransPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const auto& deviceContext = static_cast<const DeviceContext&>(context);
    auto socProfileData = dataInventory.GetPtr<HalSocProfileData>();
    if (socProfileData == nullptr)
    {
        ERROR("There is no ChipTrans data to persist");
        return ANALYSIS_ERROR;
    }
    V6Data v6Data;
    PaData paData;
    PcieData pcieData;
    uint64_t invalidBandwidthCount = 0;
    uint64_t invalidTransTypeCount = 0;
    if (!GenerateChipTransData(socProfileData->items, deviceContext, v6Data, paData, pcieData, invalidBandwidthCount,
                               invalidTransTypeCount))
    {
        return ANALYSIS_ERROR;
    }

    if (invalidBandwidthCount != 0 || invalidTransTypeCount != 0)
    {
        WARN("Discarded ChipTrans records: invalid bandwidth: %, invalid transType: %, total: %", invalidBandwidthCount,
             invalidTransTypeCount, invalidBandwidthCount + invalidTransTypeCount);
    }

    DBInfo chipTransDB("chip_trans.db", "");
    MAKE_SHARED0_RETURN_VALUE(chipTransDB.database, ChipTransDB, ANALYSIS_ERROR);
    std::string dbPath = File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, chipTransDB.dbName});
    MAKE_SHARED_RETURN_VALUE(chipTransDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);

    // V4 芯片落 PaLinkInfo/PcieInfo，V6 芯片落 PcieInfoV6，二者不共存，避免下游严格校验因缺表误报失败。
    if (deviceContext.isChipV6())
    {
        if (!SaveChipTransTable(v6Data, chipTransDB, dbPath, "PcieInfoV6"))
        {
            return ANALYSIS_ERROR;
        }
    }
    else if (!SaveChipTransTable(paData, chipTransDB, dbPath, "PaLinkInfo") ||
             !SaveChipTransTable(pcieData, chipTransDB, dbPath, "PcieInfo"))
    {
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

REGISTER_PROCESS_SEQUENCE(ChipTransPersistence, false, StarsSocProfileParser);
REGISTER_PROCESS_DEPENDENT_DATA(ChipTransPersistence, HalSocProfileData);
REGISTER_PROCESS_SUPPORT_CHIP(ChipTransPersistence, CHIP_V4_1_0, CHIP_V1_1_1, CHIP_V6_1_0);

}  // namespace Domain
}  // namespace Analysis
