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

#ifndef ANALYSIS_PERSISTENCE_HOST_MC2_COMM_INFO_DUMPER_H
#define ANALYSIS_PERSISTENCE_HOST_MC2_COMM_INFO_DUMPER_H

#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/parser/host/cann/addition_info_parser.h"
#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/capture_stream_info_dumper.h"

namespace Analysis
{
namespace Domain
{

using Mc2CommInfoData = std::vector<std::tuple<std::string, uint32_t, uint32_t, uint32_t, uint32_t, std::string>>;

struct Mc2CommInfoInput
{
    Mc2CommInfoInput(const std::vector<std::shared_ptr<ParserAdditionalInfo>> &mc2Data,
                     const CaptureStreamInfoData &captureData)
        : mc2Data(mc2Data), captureData(captureData)
    {
    }

    bool empty() const { return mc2Data.empty(); }

    const std::vector<std::shared_ptr<ParserAdditionalInfo>> &mc2Data;
    const CaptureStreamInfoData &captureData;
};

class Mc2CommInfoDumper : public BaseDumper<Mc2CommInfoDumper>
{
   public:
    explicit Mc2CommInfoDumper(const std::string &hostPath);
    Mc2CommInfoData GenerateData(const Mc2CommInfoInput &input);
};

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_PERSISTENCE_HOST_MC2_COMM_INFO_DUMPER_H
