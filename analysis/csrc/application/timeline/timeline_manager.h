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

#ifndef ANALYSIS_APPLICATION_TIMELINE_MANAGER_H
#define ANALYSIS_APPLICATION_TIMELINE_MANAGER_H

#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "analysis/csrc/application/timeline/json_process_enum.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/dump_tools/include/dump_tool.h"
#include "analysis/csrc/infrastructure/process/include/topo_graph.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Infra;
class TimelineManager
{
   public:
    explicit TimelineManager(const std::string &profPath, const std::string &outputPath)
        : profPath_(profPath), outputPath_(outputPath) {};
    static bool GetTopologyRoots(const std::vector<JsonProcess> &jsonProcesses, std::vector<TopoNodeId> &roots);
    static TopoNodeCreatorFactory CreateTimelineAssembler(const std::string &name);
    static TopoNodeCreatorFactory CreateTimelinePreDump();
    static TopoNodeCreatorFactory CreateTimelinePostDump();
    static std::vector<TopoNodeId> ResolveSelectedTimelineNodes(const TopoBuildContext &context,
                                                                const std::vector<TopoNodeId> &roots);
    static std::vector<TopoNodeId> ResolveTimelinePreDumpDependencies(const TopoBuildContext &context,
                                                                      const std::vector<TopoNodeId> &roots);
    bool PreDumpJson(const std::vector<JsonProcess> &jsonProcess, DataInventory &dataInventory);
    void PostDumpJson();

   private:
    void WriteFile(const std::string &filePrefix, FileCategory category);
    static std::vector<std::string> GetAssemblerList(const std::vector<JsonProcess> &jsonProcess);

   private:
    std::string profPath_;
    std::string outputPath_;
    std::unordered_map<FileCategory, std::string> fileType_;
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_TIMELINE_MANAGER_H
