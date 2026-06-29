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

#ifndef ANALYSIS_DOMAIN_STEP_TRACE_PROCESSOR_H
#define ANALYSIS_DOMAIN_STEP_TRACE_PROCESSOR_H

#include <functional>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/step_trace_data.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;
using namespace Analysis::Domain::Environment;

// model_id, index_id, FP_start, BP_end, iter_end, iter_time, fp_bp_time, grad_refresh_bound, data_aug_bound
using OriTrace =
    std::vector<std::tuple<uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
// model_id, index_id, start_time, end_time
using OriGetNext = std::vector<std::tuple<uint32_t, uint32_t, uint64_t, uint64_t>>;

// model_id, index_id, iter_end, start, end
using OriAllReduce = std::vector<std::tuple<uint32_t, uint32_t, uint64_t, uint64_t, uint64_t>>;

class StepTraceProcessor : public DataProcessor
{
   public:
    StepTraceProcessor() = default;
    explicit StepTraceProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
    bool SaveStepTraceData(std::vector<TrainTraceData> &trace, std::vector<GetNextData> &next,
                           std::vector<AllReduceData> &reduce, DataInventory &dataInventory);

    template <typename T>
    using HandleFunc = std::function<std::vector<T>(const DBInfo &dbInfo, LocaltimeContext &allParams, bool &errFlag,
                                                    SyscntConversionParams &params)>;
    template <typename T>
    bool ProcessData(DBInfo &info, LocaltimeContext &allParams, std::vector<T> &res, HandleFunc<T> func,
                     const std::string &devPath)
    {
        SyscntConversionParams params;
        if (!Context::GetInstance().GetSyscntConversionParams(params, allParams.deviceId, profPath_))
        {
            ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
            return false;
        }
        std::string dbPath = Utils::File::PathJoin({devPath, SQLITE, info.dbName});
        if (!info.ConstructDBRunner(dbPath) || info.dbRunner == nullptr)
        {
            return false;
        }
        auto status = CheckPathAndTable(dbPath, info);
        if (status == CHECK_FAILED)
        {
            return false;
        }
        else if (status == NOT_EXIST)
        {
            return true;
        }
        bool errFlag = false;
        auto formatData = func(info, allParams, errFlag, params);
        if (errFlag)
        {
            ERROR("% data format failed!", info.tableName);
            return false;
        }
        if (formatData.empty())
        {
            WARN("%, has no data", info.tableName);
            return true;
        }
        res.insert(res.end(), formatData.begin(), formatData.end());
        return true;
    }
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_STEP_TRACE_PROCESSOR_H
