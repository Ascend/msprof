/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of the Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_CSRC_DOMAIN_SERVICES_PERSISTENCE_HOST_DPU_HCCL_TRACK_DUMPER_H
#define ANALYSIS_CSRC_DOMAIN_SERVICES_PERSISTENCE_HOST_DPU_HCCL_TRACK_DUMPER_H

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"

namespace Analysis
{
namespace Domain
{
using DpuHcclTrackRow =
    std::tuple<uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, std::string, std::string, std::string, uint32_t,
               uint32_t, uint32_t, double, std::string, std::string, uint64_t, uint32_t, uint32_t, uint32_t, uint32_t,
               std::string, std::string, std::string, std::string, std::string, std::string, std::string, std::string,
               std::string, std::string>;
using DpuHcclTrackRows = std::vector<DpuHcclTrackRow>;

class DpuHcclTrackDumper : public BaseDumper<DpuHcclTrackDumper>
{
   public:
    explicit DpuHcclTrackDumper(const std::string &hostPath);
    DpuHcclTrackRows GenerateData(const std::vector<std::shared_ptr<ParserAdditionalInfo>> &records);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_CSRC_DOMAIN_SERVICES_PERSISTENCE_HOST_DPU_HCCL_TRACK_DUMPER_H
