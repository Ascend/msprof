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

#include "analysis/csrc/application/summary/hbm_summary_assembler.h"

#include <limits>
#include <string>
#include <vector>

#include "analysis/csrc/domain/entities/viewer_data/system/include/hbm_data.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain;
using namespace Analysis::Utils;

HbmSummaryAssembler::HbmSummaryAssembler(const std::string &name, const std::string &profPath)
    : SummaryAssembler(name, profPath)
{
    headers_ = {"Device_id", "Metric", "Read(MB/s)", "Write(MB/s)"};
}

uint8_t HbmSummaryAssembler::AssembleData(Infra::DataInventory &dataInventory)
{
    const auto data = dataInventory.GetPtr<std::vector<HbmSummaryData>>();
    if (data == nullptr)
    {
        WARN("No data to export HBM summary.");
        return DATA_NOT_EXIST;
    }
    for (const auto &item : *data)
    {
        const std::string metric =
            item.hbmId == std::numeric_limits<uint8_t>::max() ? "Average" : std::to_string(item.hbmId);
        res_.emplace_back(std::vector<std::string>{std::to_string(item.deviceId), metric,
                                                   DoubleToStr(item.readBandwidth), DoubleToStr(item.writeBandwidth)});
    }
    if (res_.empty())
    {
        ERROR("No HBM summary data was generated.");
        return ASSEMBLE_FAILED;
    }
    WriteToFile(File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, HBM_SUMMARY_NAME}), {});
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
