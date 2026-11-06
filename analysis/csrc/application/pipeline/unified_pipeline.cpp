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
#include "analysis/csrc/application/pipeline/unified_pipeline.h"

#include <algorithm>
#include <sstream>

#include "analysis/csrc/application/deliverable_consumer.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_control.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis
{
namespace Application
{
using namespace Infra;
using namespace Utils;
nlohmann::json DescribeTopoGraph(const TopoGraphBuilder& builder, const RuntimeProcessCollection& processes)
{
    nlohmann::json nodes = nlohmann::json::array();
    for (const auto& entry : builder.GetRuntimeNodeKeys())
    {
        const auto& node = processes.at(entry.second);
        nodes.push_back({{"key", entry.second},
                         {"scope", entry.first.scope},
                         {"name", entry.first.name},
                         {"stage", static_cast<uint8_t>(entry.first.stage)},
                         {"dependencies", node.processDependence},
                         {"completion_dependencies", node.completionDependence},
                         {"run_on_dependency_failure", node.runOnDependencyFailure},
                         {"resources", node.resources},
                         {"mandatory", node.mandatory}});
    }
    std::sort(nodes.begin(), nodes.end(), [](const nlohmann::json& left, const nlohmann::json& right)
              { return left.at("key").get<RuntimeProcessKey>() < right.at("key").get<RuntimeProcessKey>(); });
    return {{"version", 1}, {"nodes", std::move(nodes)}};
}

namespace
{
// Both native and legacy export entries execute here. Drain file writes and close DB writers
// before returning, including when execution or diagnostics throw.
class DeliverableScope final
{
   public:
    DeliverableScope() { DeliverableConsumer::Instance().ResetOk(); }
    ~DeliverableScope() { Finish(); }
    void Finish()
    {
        DeliverableConsumer::Instance().FinishTimeline();
        DeliverableConsumer::Instance().CloseAllDb();
    }
    DeliverableScope(const DeliverableScope&) = delete;
    DeliverableScope& operator=(const DeliverableScope&) = delete;
};

void WriteGraphReport(nlohmann::json report, const ProcessControl& executor, const std::string& profPath)
{
    try
    {
        const auto results = executor.GetRuntimeResults();
        const char* states[] = {"PENDING", "RUNNING", "SUCCEEDED", "FAILED", "BLOCKED", "CANCELLED"};
        std::ostringstream dot;
        dot << "digraph UnifiedPipeline {\n";
        for (auto& node : report["nodes"])
        {
            const auto key = node.at("key").get<RuntimeProcessKey>();
            const auto result = results.find(key);
            node["state"] = result == results.end() ? "PENDING" : states[static_cast<size_t>(result->second.state)];
            if (result != results.end())
            {
                const auto& stat = result->second.statistics;
                node["chip_id"] = stat.chipId;
                node["return_code"] = stat.returnCode;
                node["start_us"] = stat.startTime;
                node["duration_us"] = stat.duration;
                node["failed_dependencies"] = result->second.failedDependencies;
            }
            const std::string label = node.at("scope").get<std::string>() + "/" + node.at("name").get<std::string>() +
                                      "\n" + node.at("state").get<std::string>();
            dot << "  n" << key << " [label=" << nlohmann::json(label).dump() << "];\n";
            for (const auto& dependency : node.at("dependencies"))
            {
                dot << "  n" << dependency.get<RuntimeProcessKey>() << " -> n" << key
                    << (node.value("run_on_dependency_failure", false) ? " [style=dashed]" : "") << ";\n";
            }
            for (const auto& dependency : node.at("completion_dependencies"))
            {
                dot << "  n" << dependency.get<RuntimeProcessKey>() << " -> n" << key << " [style=dashed];\n";
            }
        }
        dot << "}\n";
        const auto directory = File::PathJoin({profPath, "mindstudio_profiler_log"});
        if (!File::CreateDir(directory))
        {
            return;
        }
        FileWriter json(File::PathJoin({directory, "unified_pipeline_graph.json"}));
        json.WriteText(report.dump(2));
        FileWriter graph(File::PathJoin({directory, "unified_pipeline_graph.dot"}));
        graph.WriteText(dot.str());
        if (!json.IsGood() || !graph.IsGood())
        {
            WARN("Failed to write unified pipeline graph diagnostics.");
        }
    }
    catch (const std::exception& error)
    {
        WARN("Failed to describe unified pipeline graph: %.", error.what());
    }
}
}  // namespace

// Return true and rebuild to enable JSON/DOT graph dumps.
bool IsTopoGraphDumpEnabled() { return false; }

void RecordTopoGraphDiagnostics(const nlohmann::json& report, const ProcessControl& executor,
                                const std::string& profPath)
{
    try
    {
        if (!report.is_null())
        {
            WriteGraphReport(report, executor, profPath);
        }
        std::string statistics;
        RecordProcessStat(executor.GetExecuteStat(), profPath, statistics);
        INFO("Unified pipeline statistics: %", statistics);
    }
    catch (const std::exception& error)
    {
        WARN("Failed to record unified pipeline diagnostics: %.", error.what());
    }
}

bool ExecuteTopoGraph(const TopoGraphBuilder& builder, RuntimeProcessCollection& processes, const std::string& profPath)
{
    DeliverableScope deliverableScope;
    try
    {
        const auto report = IsTopoGraphDumpEnabled() ? DescribeTopoGraph(builder, processes) : nlohmann::json();
        ProcessControlOptions options;
        options.maxActiveDevices = 2;
        options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
        ProcessControl executor(processes, options);
        const bool success = executor.ExecuteProcess();
        deliverableScope.Finish();
        RecordTopoGraphDiagnostics(report, executor, profPath);
        return success && DeliverableConsumer::Instance().Ok();
    }
    catch (const std::exception& error)
    {
        ERROR("Execute topology failed, profile path: %, reason: %.", profPath, error.what());
        return false;
    }
}

int RunUnifiedPipeline(const PipelineRequest& request)
{
    RuntimeProcessCollection processes;
    TopoGraphBuilder builder;
    if (!builder.Build(request, processes))
    {
        return ANALYSIS_ERROR;
    }
    return ExecuteTopoGraph(builder, processes, request.profPath) ? ANALYSIS_OK : ANALYSIS_ERROR;
}
}  // namespace Application
}  // namespace Analysis
