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

#include "analysis/csrc/domain/services/persistence/device/freq_persistence.h"

#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Application;
using namespace Utils;
using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint32_t>>;

ProcessedDataFormat FreqPersistence::GenerateFreqData(std::vector<HalFreqLpmData>& freqData)
{
    ProcessedDataFormat processedData;
    for (auto& data : freqData)
    {
        processedData.emplace_back(data.sysCnt, data.freq);
    }
    return processedData;
}

uint32_t FreqPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);
    auto freqData = dataInventory.GetPtr<std::vector<HalFreqLpmData>>();
    if (freqData == nullptr)
    {
        ERROR("There is no freq data, don't need to persistence");
        return ANALYSIS_ERROR;
    }
    DBInfo freqDB("freq.db", "FreqParse");
    MAKE_SHARED0_RETURN_VALUE(freqDB.database, FreqDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, freqDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(freqDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    auto data = GenerateFreqData(*freqData);
    if (data.empty())
    {
        INFO("The freq is default data no need to persistence!");
        return ANALYSIS_OK;
    }
    auto res = SaveData(data, freqDB, dbPath);
    if (res)
    {
        INFO("Process % done!", freqDB.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed: %", dbPath);
    return ANALYSIS_ERROR;
}
REGISTER_PROCESS_SEQUENCE(FreqPersistence, true, FreqParser);
REGISTER_PROCESS_DEPENDENT_DATA(FreqPersistence, std::vector<HalFreqLpmData>);
REGISTER_PROCESS_SUPPORT_CHIP(FreqPersistence, CHIP_V4_1_0, CHIP_V1_1_1);
}  // namespace Domain
}  // namespace Analysis
