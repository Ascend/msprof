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

#include "analysis/csrc/application/summary/aicpu_assembler.h"

#include <string>
#include <vector>

#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Utils;
using namespace Analysis::Domain;

AicpuAssembler::AicpuAssembler(const std::string &name, const std::string &profPath) : SummaryAssembler(name, profPath)
{
}

uint8_t AicpuAssembler::AssembleData(DataInventory &dataInventory)
{
    auto aicpuData = dataInventory.GetPtr<std::vector<AicpuSummaryData>>();
    auto dpData = dataInventory.GetPtr<std::vector<AicpuDpData>>();
    auto miData = dataInventory.GetPtr<std::vector<AicpuMiData>>();
    bool written = false;
    if (aicpuData != nullptr && WriteAicpuCsv(*aicpuData))
    {
        written = true;
    }
    if (dpData != nullptr && WriteDpCsv(*dpData))
    {
        written = true;
    }
    if (miData != nullptr && WriteMiCsv(*miData))
    {
        written = true;
    }
    if (!written)
    {
        WARN("No data to export aicpu/dp/aicpu_mi summary");
        return DATA_NOT_EXIST;
    }
    return ASSEMBLE_SUCCESS;
}

bool AicpuAssembler::WriteAicpuCsv(const std::vector<AicpuSummaryData> &data)
{
    if (data.empty())
    {
        return false;
    }
    headers_ = {"Timestamp(us)",     "Node",           "Compute_time(us)", "Memcpy_time(us)", "Task_time(us)",
                "Dispatch_time(us)", "Total_time(us)", "Stream ID",        "Task ID"};
    res_.clear();
    for (const auto &item : data)
    {
        res_.emplace_back(std::vector<std::string>{
            DivideByPowersOfTenWithPrecision(item.timestampNs), item.nodeName, DoubleToStr(item.computeTimeUs),
            DoubleToStr(item.memcpyTimeUs), DoubleToStr(item.taskTimeUs), DoubleToStr(item.dispatchTimeUs),
            DoubleToStr(item.totalTimeUs), std::to_string(item.streamId), std::to_string(item.taskId)});
    }
    WriteToFile(File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, AICPU_NAME}), {});
    return true;
}

bool AicpuAssembler::WriteDpCsv(const std::vector<AicpuDpData> &data)
{
    if (data.empty())
    {
        return false;
    }
    headers_ = {"Timestamp(us)", "Action", "Source", "Cached Buffer Size"};
    res_.clear();
    for (const auto &item : data)
    {
        res_.emplace_back(std::vector<std::string>{DivideByPowersOfTenWithPrecision(item.timestamp), item.action,
                                                   item.source, std::to_string(item.bufferSize)});
    }
    WriteToFile(File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, AICPU_DP_NAME}), {});
    return true;
}

bool AicpuAssembler::WriteMiCsv(const std::vector<AicpuMiData> &data)
{
    if (data.empty())
    {
        return false;
    }
    headers_ = {"Node Name", "Start Time(us)", "End Time(us)", "Queue Size"};
    res_.clear();
    for (const auto &item : data)
    {
        res_.emplace_back(std::vector<std::string>{item.nodeName, std::to_string(item.startTime),
                                                   std::to_string(item.endTime), std::to_string(item.queueSize)});
    }
    WriteToFile(File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, AICPU_MI_NAME}), {});
    return true;
}
}  // namespace Application
}  // namespace Analysis
