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

#ifndef ANALYSIS_VIEWER_DATABASE_DPU_TASK_TRACK_DB_DUMPER_H
#define ANALYSIS_VIEWER_DATABASE_DPU_TASK_TRACK_DB_DUMPER_H

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/utils/prof_common.h"

namespace Analysis
{
namespace Domain
{

using DpuTaskTrackData =
    std::vector<std::tuple<uint32_t, uint32_t, uint64_t, uint64_t, std::string, uint32_t, uint32_t, std::string>>;

class DpuTaskTrackDBDumper final : public BaseDumper<DpuTaskTrackDBDumper>
{
    using DpuTrackInfos = std::vector<std::shared_ptr<MsprofCompactInfo>>;

   public:
    explicit DpuTaskTrackDBDumper(const std::string &hostFilePath);
    DpuTaskTrackData GenerateData(const DpuTrackInfos &dpuTrackInfos);
    void SetKernelNameMap(const std::unordered_map<uint64_t, uint64_t> &map) { kernelNameMap_ = map; }
    std::unordered_map<uint64_t, uint64_t> kernelNameMap_;
};

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_VIEWER_DATABASE_DPU_TASK_TRACK_DB_DUMPER_H
