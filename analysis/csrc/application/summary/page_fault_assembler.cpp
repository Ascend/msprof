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

#include "analysis/csrc/application/summary/page_fault_assembler.h"

#include "analysis/csrc/domain/entities/viewer_data/system/include/page_fault_data.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Infra;

PageFaultAssembler::PageFaultAssembler(const std::string& name, const std::string& profPath)
    : SummaryAssembler(name, profPath)
{
    headers_ = {"Device Id", "Stream Id", "Task Id", "Page Fault Num", "Op Name"};
}

uint8_t PageFaultAssembler::AssembleData(DataInventory& dataInventory)
{
    auto pageFaultData = dataInventory.GetPtr<std::vector<PageFaultData>>();
    if (pageFaultData == nullptr)
    {
        WARN("No data to export page fault summary");
        return DATA_NOT_EXIST;
    }
    for (const auto& item : *pageFaultData)
    {
        res_.emplace_back(std::vector<std::string>{
            std::to_string(item.deviceId),
            std::to_string(item.streamId),
            std::to_string(item.taskId),
            std::to_string(item.pageFaultNum),
            item.opName,
        });
    }
    if (res_.empty())
    {
        ERROR("Can't match any page fault data, failed to generate page_fault_*.csv");
        return ASSEMBLE_FAILED;
    }
    WriteToFile(File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, PAGE_FAULT_NAME}), {});
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
