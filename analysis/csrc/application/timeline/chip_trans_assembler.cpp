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

#include "analysis/csrc/application/timeline/chip_trans_assembler.h"

#include "analysis/csrc/application/timeline/chip_trans_v6_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/chip_trans_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Application;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
namespace
{
const std::vector<std::string> PA_LINK_COUNTERS{"PA Link Rx", "PA Link Tx"};
const std::vector<std::string> PCIE_COUNTERS{"PCIE Read Bandwidth", "PCIE Write Bandwidth"};
const std::string PA_ID = "PA Link ID";
const std::string PCIE_ID = "PCIE ID";
const int STARS_PA = 111111;  // 这两个值按照二进制的func_type对数据类型做区分，Python使用的是字符串，实测类型无影响
const int STARS_PCIE = 100010;
}  // namespace

ChipTransAssembler::ChipTransAssembler()
    : JsonAssembler(PROCESS_STARS_CHIP_TRANS, {{MSPROF_JSON_FILE, FileCategory::MSPROF}})
{
}

void GeneratePaLinkInfoTrace(std::vector<PaLinkInfoData> &paLinkInfoData,
                             const std::unordered_map<uint16_t, uint32_t> &pidMap,
                             std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    uint64_t value;
    uint32_t pid;
    for (const auto &data : paLinkInfoData)
    {
        time = DivideByPowersOfTenWithPrecision(data.timestamp);
        pid = pidMap.at(data.deviceId);
        std::vector<std::string> level{data.paLinkTrafficMonitRx, data.paLinkTrafficMonitTx};
        for (size_t i = 0; i < level.size(); ++i)
        {
            if (StrToU64(value, level[i]) != ANALYSIS_OK)
            {
                ERROR("paLinkInfoData is invalid");
                continue;
            }
            MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, STARS_PA, time, PA_LINK_COUNTERS[i]);
            event->SetSeriesIValue(PA_ID, data.paLinkId);
            event->SetSeriesIValue(PA_LINK_COUNTERS[i], value);
            res.push_back(event);
        }
    }
}

void GeneratePcieInfoTrace(std::vector<PcieInfoData> &pcieInfoData,
                           const std::unordered_map<uint16_t, uint32_t> &pidMap,
                           std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    uint32_t pid;
    for (const auto &data : pcieInfoData)
    {
        time = DivideByPowersOfTenWithPrecision(data.timestamp);
        pid = pidMap.at(data.deviceId);
        std::vector<uint64_t> level{data.pcieReadBandwidth, data.pcieWriteBandwidth};
        for (size_t i = 0; i < level.size(); i++)
        {
            MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, STARS_PCIE, time, PCIE_COUNTERS[i]);
            event->SetSeriesIValue(PCIE_ID, data.pcieId);
            event->SetSeriesIValue(PCIE_COUNTERS[i], level[i]);
            res.push_back(event);
        }
    }
}

uint8_t ChipTransAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto version = Context::GetInstance().GetPlatformVersion(DEFAULT_DEVICE_ID, profPath);
    if (Context::GetInstance().IsChipV6(version))
    {
        ChipTransV6Assembler chipTransV6Assembler;
        return chipTransV6Assembler.AssembleData(dataInventory, ostream, profPath);
    }
    auto paLinkInfoData = dataInventory.GetPtr<std::vector<PaLinkInfoData>>();
    if (paLinkInfoData == nullptr)
    {
        WARN("Can't get paLinkInfoData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_STARS_CHIP_TRANS);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    auto pcieInfoData = dataInventory.GetPtr<std::vector<PcieInfoData>>();
    if (pcieInfoData == nullptr)
    {
        WARN("Can't get pcieInfoData from dataInventory");  // 很多时候这个表并不存在
    }
    else
    {
        GeneratePcieInfoTrace(*pcieInfoData, pidMap, res_);
    }
    GeneratePaLinkInfoTrace(*paLinkInfoData, pidMap, res_);
    if (res_.empty())
    {
        ERROR("Can't Generate any Stars Chip Trans data");
        return ASSEMBLE_FAILED;
    }
    for (const auto &node : res_)
    {
        node->DumpJson(ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
