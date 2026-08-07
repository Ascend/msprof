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
#ifndef ANALYSIS_CSRC_VIEWER_DATABASE_MEMCPY_INFO_DUMPER_H
#define ANALYSIS_CSRC_VIEWER_DATABASE_MEMCPY_INFO_DUMPER_H

#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"
#include "analysis/csrc/infrastructure/utils/prof_common.h"

namespace Analysis
{
namespace Domain
{
using MemcpyInfoData = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint16_t, int64_t, uint16_t>>;
using MemcpyInfos = std::vector<std::shared_ptr<MsprofCompactInfo>>;
struct MemcpyAsyncTask
{
    uint32_t threadId;
    uint64_t timestamp;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t batchId;
    uint16_t deviceId;
    MemcpyAsyncTask()
        : threadId(UINT32_MAX),
          timestamp(0),
          streamId(UINT32_MAX),
          taskId(UINT32_MAX),
          batchId(UINT32_MAX),
          deviceId(UINT16_MAX)
    {
    }
};

class MemcpyInfoDumper : public BaseDumper<MemcpyInfoDumper>
{
   public:
    explicit MemcpyInfoDumper(const std::string &hostPath);
    MemcpyInfoData GenerateData(const MemcpyInfos &memcpyInfos);

   private:
    std::vector<MemcpyAsyncTask> GetMemcpyAsyncTasks();
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_CSRC_VIEWER_DATABASE_MEMCPY_INFO_DUMPER_H
