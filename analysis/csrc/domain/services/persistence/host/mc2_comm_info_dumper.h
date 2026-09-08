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

#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/capture_stream_info_dumper.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"

namespace Analysis
{
namespace Domain
{

using Mc2CommInfoData = std::vector<std::tuple<std::string, uint32_t, uint32_t, uint32_t, uint32_t, std::string>>;
using Mc2RawData = std::vector<std::shared_ptr<ParserAdditionalInfo>>;

class Mc2CommInfoDumper : public BaseDumper<Mc2CommInfoDumper>
{
   public:
    explicit Mc2CommInfoDumper(const std::string &hostPath, CaptureStreamInfoData captureData = {});
    Mc2CommInfoData GenerateData(const Mc2RawData &mc2Data);

   private:
    bool NeedMapInvalidStreamId() const;

    CaptureStreamInfoData captureData_;
    std::string hostPath_;
};

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_PERSISTENCE_HOST_MC2_COMM_INFO_DUMPER_H
