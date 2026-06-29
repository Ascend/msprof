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
 * -------------------------------------------------------------------------
 */

#include "analysis/csrc/domain/services/device_context/load_stream_expand_spec_data.h"

#include <string>

#include "analysis/csrc/domain/entities/hal/include/stream_expand_spec.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/persistence/host/number_mapping.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

using namespace Analysis;
using namespace Analysis::Application;
using namespace Analysis::Infra;
using namespace Utils;

namespace Analysis
{
namespace Domain
{

using StreamExpandSpecFormat = std::vector<std::tuple<uint16_t>>;
const std::string STREAM_EXPAND_INFO_TABLE = "StreamExpandSpec";

namespace
{
bool CheckPathAndTableExists(const std::string& path, DBRunner& dbRunner, const std::string& tableName)
{
    if (!File::Exist(path))
    {
        WARN("There is no %", path);
        return false;
    }
    if (!dbRunner.CheckTableExists(tableName))
    {
        WARN("There is no %", tableName);
        return false;
    }
    return true;
}

uint32_t ReadHostStreamExpandInfo(DataInventory& dataInventory, const DeviceContext& deviceContext)
{
    StreamExpandSpecDB streamExpandSpecDB;
    DeviceInfo deviceInfo{};
    deviceContext.Getter(deviceInfo);
    auto hostPath = deviceContext.GetDeviceFilePath();
    std::string streamExpandInfoDbDirectory =
        Utils::File::PathJoin({hostPath, "../", "/host", "/sqlite", streamExpandSpecDB.GetDBName()});
    DBRunner streamExpandInfoDBRunner(streamExpandInfoDbDirectory);
    if (!CheckPathAndTableExists(streamExpandInfoDbDirectory, streamExpandInfoDBRunner, STREAM_EXPAND_INFO_TABLE))
    {
        return ANALYSIS_OK;
    }
    std::string sql{"SELECT expand_status from StreamExpandSpec LIMIT 1"};
    StreamExpandSpecFormat result;
    bool rc = streamExpandInfoDBRunner.QueryData(sql, result);
    uint16_t expandStatus = 0;
    if (!rc)
    {
        ERROR("Failed to obtain data from the % table.", streamExpandSpecDB.GetDBName());
        return ANALYSIS_ERROR;
    }
    else if (result.empty())
    {
        WARN("No data found in StreamExpandSpec table, using default value 0.");
    }
    else
    {
        auto& firstStreamExpandSpecInfo = *result.begin();
        expandStatus = std::get<0>(firstStreamExpandSpecInfo);
    }
    StreamExpandSpec streamExpandSpec{expandStatus};
    std::shared_ptr<StreamExpandSpec> data;
    MAKE_SHARED_RETURN_VALUE(data, StreamExpandSpec, ANALYSIS_ERROR,
                             std::move(StreamExpandSpec{std::move(streamExpandSpec)}));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}
}  // namespace

uint32_t LoadStreamExpandSpec::ProcessEntry(DataInventory& dataInventory, const Infra::Context& context)
{
    const auto& deviceContext = dynamic_cast<const DeviceContext&>(context);
    return ReadHostStreamExpandInfo(dataInventory, deviceContext);
}

REGISTER_PROCESS_SEQUENCE(LoadStreamExpandSpec, true);
REGISTER_PROCESS_SUPPORT_CHIP(LoadStreamExpandSpec, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
