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

#include "analysis/csrc/application/summary/ub_summary_assembler.h"

#include <algorithm>
#include <string>
#include <vector>

#include "analysis/csrc/domain/entities/viewer_data/system/include/ub_data.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain;
using namespace Analysis::Utils;

UbSummaryAssembler::UbSummaryAssembler(const std::string &name, const std::string &profPath)
    : SummaryAssembler(name, profPath)
{
    headers_ = {"Device_id", "PortId", "TimeStamp", "UBRxPortBandWidth(MB/s)", "UBTxPortBandWidth(MB/s)"};
}

uint8_t UbSummaryAssembler::AssembleData(Infra::DataInventory &dataInventory)
{
    const auto data = dataInventory.GetPtr<std::vector<UbData>>();
    if (data == nullptr)
    {
        WARN("No data to export UB summary.");
        return DATA_NOT_EXIST;
    }
    if (data->empty())
    {
        ERROR("No UB data was generated for summary.");
        return ASSEMBLE_FAILED;
    }

    const std::string fileName = File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, UB_SUMMARY_NAME});
    auto &consumer = DeliverableConsumer::Instance();
    consumer.StartCsv(fileName, headers_, {}, "", data->size());
    std::vector<std::vector<std::string>> batch;
    batch.reserve(std::min(CSV_ROW_BATCH, data->size()));
    for (const auto &item : *data)
    {
        batch.emplace_back(std::vector<std::string>{std::to_string(item.deviceId), std::to_string(item.portId),
                                                    DivideByPowersOfTenWithPrecision(item.timestamp, true),
                                                    std::to_string(item.udmaRxBind), std::to_string(item.udmaTxBind)});
        if (batch.size() >= CSV_ROW_BATCH)
        {
            consumer.AppendCsv(fileName, batch);
            batch.clear();
        }
    }
    if (!batch.empty())
    {
        consumer.AppendCsv(fileName, batch);
    }
    consumer.FinishCsv(fileName);
    INFO("Prepared % UB summary rows for export, profPath is %.", data->size(), profPath_);
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
