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

#include "analysis/csrc/application/summary/comm_statistic_assembler.h"

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Utils;
using Context = Analysis::Domain::Environment::Context;

CommStatisticAssembler::CommStatisticAssembler(const std::string& name, const std::string& profPath)
    : SummaryAssembler(name, profPath)
{
    headers_ = {"Device_id",    "OP Type",      "Count",        "Total Time(us)",
                "Min Time(us)", "Avg Time(us)", "Max Time(us)", "Ratio(%)"};
}

void CommStatisticAssembler::GenerateCommStatistic(std::vector<HcclStatisticData>& commStatisticData)
{
    // 按totalTime降序排序
    std::sort(commStatisticData.begin(), commStatisticData.end(),
              [](const HcclStatisticData& a, const HcclStatisticData& b)
              {
                  return a.totalTime > b.totalTime;  // 降序排序
              });
    for (const auto& data : commStatisticData)
    {
        std::vector<std::string> row = {std::to_string(data.deviceId),
                                        data.opType,
                                        data.count,
                                        DoubleToStr(data.totalTime),
                                        DoubleToStr(data.min),
                                        DoubleToStr(data.avg),
                                        DoubleToStr(data.max),
                                        DoubleToStr(data.ratio)};
        res_.emplace_back(row);
    }
}

uint8_t CommStatisticAssembler::AssembleData(DataInventory& dataInventory)
{
    auto commStatisticData = dataInventory.GetPtr<std::vector<HcclStatisticData>>();
    auto isLevel0 = Context::GetInstance().IsLevel0(profPath_);
    if (commStatisticData == nullptr || isLevel0)
    {
        WARN("Communication Statistic data not exist.");
        return DATA_NOT_EXIST;
    }
    GenerateCommStatistic(*commStatisticData);
    if (res_.empty())
    {
        ERROR("Can't Generate any communication statistic process data");
        return ASSEMBLE_FAILED;
    }
    WriteToFile(File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, COMM_STATISTIC_NAME}), {});
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
