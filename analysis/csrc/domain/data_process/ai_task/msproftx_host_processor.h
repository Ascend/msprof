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

#ifndef ANALYSIS_DOMAIN_MSPROFTX_HOST_PROCESSOR_H
#define ANALYSIS_DOMAIN_MSPROFTX_HOST_PROCESSOR_H

#include <functional>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"

namespace Analysis
{
namespace Domain
{
// pid, tid, category, payload_type, message_type, payload_value, start, end, event_type, message
using OriMsprofTxHostData = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, uint64_t,
                                                   uint64_t, std::string, std::string>>;
// pid, tid, mark_id, start, end, event_type, domain, message
using OriMsprofTxExHostData =
    std::vector<std::tuple<uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, std::string, std::string, std::string>>;
class MsprofTxHostProcessor : public DataProcessor
{
   public:
    MsprofTxHostProcessor() = default;
    explicit MsprofTxHostProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    void FormatTxData(const OriMsprofTxHostData &oriTxData, std::vector<MsprofTxHostData> &processedData,
                      Utils::SyscntConversionParams &params, Utils::ProfTimeRecord &record);
    void FormatTxExData(const OriMsprofTxExHostData &oriTxExData, std::vector<MsprofTxHostData> &processedData,
                        Utils::SyscntConversionParams &params, Utils::ProfTimeRecord &record);

    template <typename T>
    bool ProcessData(T &oriData, DBInfo &info, bool &notExistFlag,
                     std::function<T(const DBInfo &dbInfo, const std::string &dbPath)> func)
    {
        std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, info.dbName});
        if (!info.ConstructDBRunner(dbPath))
        {
            return false;
        }
        auto status = CheckPathAndTable(dbPath, info, false);
        if (status == CHECK_FAILED)
        {
            return false;
        }
        else if (status == NOT_EXIST)
        {
            notExistFlag = true;
            return true;
        }
        oriData = func(info, dbPath);
        if (oriData.empty())
        {
            WARN("original data is empty. DBPath is %", dbPath);
        }
        return true;
    }
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_MSPROFTX_HOST_PROCESSOR_H
