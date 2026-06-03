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

#ifndef ANALYSIS_VIEWER_DATABASE_DRAFTS_CANN_DB_DUMPER_H
#define ANALYSIS_VIEWER_DATABASE_DRAFTS_CANN_DB_DUMPER_H

#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/entities/tree/include/tree.h"
#include "analysis/csrc/domain/services/association/cann/include/tree_analyzer.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{

// 供HostTraceWorker调用， 传入Analyzer对象，将所有数据落盘
class CANNTraceDBDumper
{
    using TreeAnalyzer = Analysis::Domain::Cann::TreeAnalyzer;
    using HostTask = Analysis::Domain::HostTask;
    using OpDesc = Analysis::Domain::OpDesc;
    using HostTasks = std::vector<std::shared_ptr<HostTask>>;
    using TaskInfoData = std::vector<
        std::tuple<uint32_t, std::string, uint32_t, uint32_t, uint32_t, uint32_t, std::string, std::string, std::string,
                   int32_t, uint32_t, uint64_t, uint32_t, uint32_t, std::string, std::string, std::string, std::string,
                   std::string, std::string, uint32_t, uint32_t, std::string, std::string, std::string, std::string>>;
    using HCCLBigOpDescs = Analysis::Domain::Cann::HCCLBigOpDescs;
    using GeFusionOpInfos = Analysis::Domain::Cann::GeFusionOpInfos;

   public:
    // 创建时传入host路径
    explicit CANNTraceDBDumper(std::string hostFilePath);

    // 提供DumpData方法，调用后执行落盘操作，成功返回True，失败返回False。
    bool DumpData(TreeAnalyzer analyzer);

   private:
    // 落盘HCCLOP
    void DumpHcclOps(const HCCLBigOpDescs &hcclOps);

    // 落盘HostTask
    void DumpHostTasks(const HostTasks &hostTasks);

    // 落盘TaskInfo
    void DumpOpDesc(const HostTasks &computeTasks);

    // 落盘HCCLTask
    void DumpHcclTasks(const HostTasks &hcclTasks);

    // 落盘GeFusionOpInfo
    void DumpGeFusionOps(const GeFusionOpInfos &geFusionOps);

    void AddTaskInfo(const std::shared_ptr<HostTask> &task, TaskInfoData &data, bool isLevel0);
    void AddTaskInfoForOnlyTaskTrack(const std::shared_ptr<HostTask> &task, TaskInfoData &data, bool isLevel0);

    void AddTensorShapeInfo(const std::shared_ptr<ConcatTensorInfo> &tensorDesc, MsprofNodeBasicInfo nodeBasicInfo,
                            TaskInfoData &data, const std::shared_ptr<HostTask> &task);
    static void ProcessRuntimeTrackInfo(const std::shared_ptr<MsprofCompactInfo> &runtimeTrack, uint32_t &blockNum,
                                        uint32_t &mixBlockNum, std::string &gridDimStr, std::string &blockDimStr);
    static std::string GetFormat(uint32_t oriFormat);
    const uint32_t poolSize_ = 4;
    std::string hostFilePath_;
    std::atomic<bool> result_;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_VIEWER_DATABASE_DRAFTS_CANN_DB_DUMPER_H
