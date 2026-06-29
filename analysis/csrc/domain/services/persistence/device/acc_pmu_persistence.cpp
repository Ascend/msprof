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

#include "analysis/csrc/domain/services/persistence/device/acc_pmu_persistence.h"

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/services/parser/log/include/stars_soc_parser.h"
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
const int READ = 0;
const int WRITE = 1;
// acc_id, read_bandwidth, write_bandwidth, read_ost, write_ost, timestamp
using ProcessedDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t, uint64_t, double>>;

ProcessedDataFormat GenerateAccPmuData(std::vector<HalLogData>& logData, const DeviceContext& context)
{
    auto params = GenerateSyscntConversionParams(context);
    ProcessedDataFormat processedData;
    for (auto& data : logData)
    {
        if (data.type != ACC_PMU)
        {
            continue;
        }
        auto hfFloatTime = GetTimeFromSyscnt(data.accPmu.timestamp, params);
        processedData.emplace_back(data.accPmu.accId, data.accPmu.bandwidth[READ], data.accPmu.bandwidth[WRITE],
                                   data.accPmu.ost[READ], data.accPmu.ost[WRITE], hfFloatTime.Double());
    }
    return processedData;
}

uint32_t AccPmuPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);
    auto accPmu = dataInventory.GetPtr<std::vector<HalLogData>>();
    if (!accPmu)
    {
        ERROR("acc pmu data is null.");
        return ANALYSIS_ERROR;
    }
    DBInfo accPmuDB("acc_pmu.db", "AccPmu");
    MAKE_SHARED0_RETURN_VALUE(accPmuDB.database, AccPmuDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, accPmuDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(accPmuDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    auto data = GenerateAccPmuData(*accPmu, deviceContext);
    if (data.empty())
    {
        INFO("There is no acc pmu data don't need to persistence!");
        return ANALYSIS_OK;
    }
    auto res = SaveData(data, accPmuDB, dbPath);
    if (res)
    {
        INFO("Process % done!", accPmuDB.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed: %", dbPath);
    return ANALYSIS_ERROR;
}
REGISTER_PROCESS_SEQUENCE(AccPmuPersistence, true, StarsSocParser);
REGISTER_PROCESS_DEPENDENT_DATA(AccPmuPersistence, std::vector<HalLogData>);
REGISTER_PROCESS_SUPPORT_CHIP(AccPmuPersistence, CHIP_V4_1_0);
}  // namespace Domain
}  // namespace Analysis
