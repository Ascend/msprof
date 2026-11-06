/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */
#ifndef ANALYSIS_APPLICATION_UNIFIED_PIPELINE_H
#define ANALYSIS_APPLICATION_UNIFIED_PIPELINE_H

#include "analysis/csrc/application/pipeline/pipeline_flags.h"
#include "analysis/csrc/infrastructure/process/include/topo_graph.h"
#include "nlohmann/json.hpp"

namespace Analysis
{
namespace Infra
{
class ProcessControl;
}

namespace Application
{
struct PipelineRequest
{
    std::string profPath;
    std::string hostPath;
    std::string devicePath;
    std::string reportsJson;
    int flags = 0;
};

// Planning is read-only. Runtime bindings own the objects until all work has drained.
bool PlanUnifiedPipeline(const PipelineRequest& request, TopoBuildContext& context, TopoNodeCollection& definitions,
                         std::vector<TopoNodeId>& roots);
int RunUnifiedPipeline(const PipelineRequest& request);
// Code-level decision for automatic JSON/DOT graph output; defaults to false.
bool IsTopoGraphDumpEnabled();
// Capture graph metadata before ProcessControl takes ownership of the runtime processes.
nlohmann::json DescribeTopoGraph(const TopoGraphBuilder& builder, const Infra::RuntimeProcessCollection& processes);
// A null report skips graph files while retaining process statistics.
// Records an already executed graph without changing its execution result.
void RecordTopoGraphDiagnostics(const nlohmann::json& report, const Infra::ProcessControl& executor,
                                const std::string& profPath);
bool ExecuteTopoGraph(const TopoGraphBuilder& builder, Infra::RuntimeProcessCollection& processes,
                      const std::string& profPath);
}  // namespace Application
}  // namespace Analysis
#endif
