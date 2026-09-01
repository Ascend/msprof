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

#include "analysis/csrc/domain/services/persistence/device/lpm_info_persistence.h"

#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/parser/freq/include/lpm_info_parser.h"
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
using ProcessedData = std::vector<std::tuple<uint64_t, uint32_t>>;

bool GenerateData(const std::vector<HalLpmValue>& source, ProcessedData& result)
{
    if (!Reserve(result, source.size()))
    {
        ERROR("Reserve for LpmInfo persistence data failed");
        return false;
    }
    for (const auto& item : source)
    {
        result.emplace_back(item.sysCnt, item.value);
    }
    return true;
}

bool SaveSingleTable(const ProcessedData& data, const std::string& dbName, const std::string& tableName,
                     const std::shared_ptr<Database>& database, const std::shared_ptr<DBRunner>& dbRunner,
                     std::string& dbPath)
{
    if (data.empty())
    {
        return true;
    }
    DBInfo dbInfo(dbName, tableName);
    dbInfo.database = database;
    dbInfo.dbRunner = dbRunner;
    return SaveData(data, dbInfo, dbPath);
}
}  // namespace

uint32_t LpmInfoPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const auto& deviceContext = static_cast<const DeviceContext&>(context);
    auto lpmInfoData = dataInventory.GetPtr<HalLpmInfoData>();
    if (lpmInfoData == nullptr)
    {
        ERROR("There is no LpmInfo data to persistence");
        return ANALYSIS_ERROR;
    }

    ProcessedData freqData;
    ProcessedData aicVoltageData;
    ProcessedData busVoltageData;
    if (!GenerateData(lpmInfoData->freqData, freqData) || !GenerateData(lpmInfoData->aicVoltageData, aicVoltageData) ||
        !GenerateData(lpmInfoData->busVoltageData, busVoltageData))
    {
        return ANALYSIS_ERROR;
    }
    if (freqData.empty() && aicVoltageData.empty() && busVoltageData.empty())
    {
        INFO("LpmInfo data is empty, don't need to persistence");
        return ANALYSIS_OK;
    }

    if (!freqData.empty())
    {
        std::shared_ptr<Database> freqDatabase;
        MAKE_SHARED0_RETURN_VALUE(freqDatabase, FreqDB, ANALYSIS_ERROR);
        std::string freqPath = File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, "freq.db"});
        std::shared_ptr<DBRunner> freqRunner;
        MAKE_SHARED_RETURN_VALUE(freqRunner, DBRunner, ANALYSIS_ERROR, freqPath);
        if (!SaveSingleTable(freqData, "freq.db", "FreqParse", freqDatabase, freqRunner, freqPath))
        {
            ERROR("Save FreqParse data failed: %", freqPath);
            return ANALYSIS_ERROR;
        }
    }

    if (!aicVoltageData.empty() || !busVoltageData.empty())
    {
        std::shared_ptr<Database> voltageDatabase;
        MAKE_SHARED0_RETURN_VALUE(voltageDatabase, VoltageDB, ANALYSIS_ERROR);
        std::string voltagePath = File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, "voltage.db"});
        std::shared_ptr<DBRunner> voltageRunner;
        MAKE_SHARED_RETURN_VALUE(voltageRunner, DBRunner, ANALYSIS_ERROR, voltagePath);
        if (!SaveSingleTable(aicVoltageData, "voltage.db", "AicVoltage", voltageDatabase, voltageRunner, voltagePath) ||
            !SaveSingleTable(busVoltageData, "voltage.db", "BusVoltage", voltageDatabase, voltageRunner, voltagePath))
        {
            ERROR("Save voltage data failed: %", voltagePath);
            return ANALYSIS_ERROR;
        }
    }
    return ANALYSIS_OK;
}

}  // namespace Domain
}  // namespace Analysis
