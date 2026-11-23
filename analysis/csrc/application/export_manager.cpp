/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
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

#include "analysis/csrc/application/include/export_manager.h"

#include <algorithm>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "analysis/csrc/application/database/db_assembler.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/include/report_map.h"
#include "analysis/csrc/application/pipeline/unified_pipeline.h"
#include "analysis/csrc/application/summary/summary_manager.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/application/timeline/timeline_manager.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain;

namespace
{
enum class ReportSectionState
{
    ABSENT,  // The section is not configured, no deliverable of this kind is exported.
    PARSED,  // The section is valid and every enabled deliverable has been applied.
    INVALID  // The section is malformed, the caller decides whether to abort or to keep the defaults.
};

// Parses one section of the report config and hands every enabled deliverable to the caller.
// The apply callback returns false when the deliverable name is invalid.
template <typename ApplyFunc>
ReportSectionState ParseReportSection(const nlohmann::json& section, const char* sectionKey, const char* displayName,
                                      bool strict, ApplyFunc apply)
{
    if (section.is_null() || (section.is_object() && section.empty()))
    {
        INFO("The % is not exist.", sectionKey);
        return ReportSectionState::ABSENT;
    }
    if (!section.is_object())
    {
        if (!strict)
        {
            INFO("The % is not exist.", sectionKey);
            return ReportSectionState::ABSENT;
        }
        ERROR("% process must be a JSON object.", displayName);
        PRINT_ERROR("% process must be a JSON object.", displayName);
        return ReportSectionState::INVALID;
    }
    for (nlohmann::json::const_iterator it = section.begin(); it != section.end(); ++it)
    {
        if (!it.value().is_boolean())
        {
            ERROR("% process value must be bool.", displayName);
            PRINT_ERROR("% process value must be bool.", displayName);
            return ReportSectionState::INVALID;
        }
        if (it.value() && !apply(it.key()))
        {
            ERROR("% process contains invalid key.", displayName);
            PRINT_ERROR("% process contains invalid key.", displayName);
            return ReportSectionState::INVALID;
        }
    }
    return ReportSectionState::PARSED;
}

// Expands the whole deliverable selection, it is used when the report config is not given or unreadable.
void ApplyDefaultSelection(bool exportTimeline, bool exportSummary, bool exportDB, ExportSelection& selection)
{
    if (exportTimeline)
    {
        selection.timelineProcesses = GetAllTimelineJsonKeys();
    }
    if (exportSummary)
    {
        selection.summaryProcesses = GetAllSummaryJsonKeys();
    }
    if (exportDB)
    {
        selection.dbProcesses = GetAllDBJsonKeys();
    }
}

void RemoveDuplicateRoots(std::vector<TopoNodeId>& roots)
{
    std::unordered_set<TopoNodeId, TopoNodeIdHash> seen;
    std::vector<TopoNodeId> uniqueRoots;
    uniqueRoots.reserve(roots.size());
    for (const auto& root : roots)
    {
        if (seen.insert(root).second)
        {
            uniqueRoots.emplace_back(root);
        }
    }
    roots.swap(uniqueRoots);
}
}  // namespace

bool ExportManager::HasExportedMsprofDB(const std::string& profPath)
{
    const std::string dbSuffix = ".db";
    const std::vector<std::string> dbFiles = File::GetFilesWithPrefix(profPath, DB_NAME_MSPROF_DB + "_");
    return std::any_of(dbFiles.begin(), dbFiles.end(),
                       [&dbSuffix](const std::string& dbPath)
                       {
                           return dbPath.size() >= dbSuffix.size() &&
                                  dbPath.compare(dbPath.size() - dbSuffix.size(), dbSuffix.size(), dbSuffix) == 0;
                       });
}

bool ExportManager::CheckProfDirsValid()
{
    if (profPath_.find("PROF") == std::string::npos)
    {
        ERROR("The path: % is not contains PROF, please check", profPath_);
        PRINT_ERROR(
            "There are no directories whose names contain 'PROF' under this path: %. Please verify the file "
            "correctness.",
            profPath_);
        return false;
    }
    return true;
}

bool ExportManager::Init()
{
    if (!CheckProfDirsValid())
    {
        ERROR("Check TimelineManager output path failed, path is %.", profPath_);
        PRINT_ERROR(
            "Check TimelineManager output path failed, path is %. "
            "Please check msprof_analysis_log in outputPath for more info.",
            profPath_);
        return false;
    }
    if (!Analysis::Domain::Environment::Context::GetInstance().Load({profPath_}))
    {
        ERROR("JSON parameter loading failed. Please check if the JSON data is complete.");
        PRINT_ERROR(
            "JSON parameter loading failed. Please check if the JSON data is complete. "
            "Please check msprof_analysis_log in outputPath for more info.");
        return false;
    }
    return true;
}

bool ExportManager::Run(const std::set<ExportMode>& exportModeSet)
{
    INFO("Start to load data!");
    PRINT_INFO("Start to load data!");
    if (!Init())
    {
        return false;
    }

    TopoBuildContext buildContext;
    buildContext.environmentLoaded = true;
    buildContext.strictSelection = !jsonPath_.empty() && exportModeSet.count(ExportMode::SUMMARY) != 0;
    std::vector<TopoNodeId> roots;
    if (!Plan(exportModeSet, buildContext, roots))
    {
        return false;
    }
    if (roots.empty())
    {
        INFO("No export deliverable is selected, skip export.");
        return true;
    }

    Infra::RuntimeProcessCollection processes;
    TopoGraphBuilder graphBuilder;
    if (!graphBuilder.BuildExport(buildContext, roots, processes))
    {
        ERROR("Build export topology failed.");
        return false;
    }

    const bool success = ExecuteTopoGraph(graphBuilder, processes, profPath_);
    if (!success)
    {
        ERROR("The % export topology failed to be executed.", profPath_);
        PRINT_ERROR(
            "The % for export failed to be executed. "
            "Please check msprof_analysis_log in outputPath for more info.",
            profPath_);
        return false;
    }
    return true;
}

bool ExportManager::Plan(const std::set<ExportMode>& exportModeSet, TopoBuildContext& buildContext,
                         std::vector<TopoNodeId>& roots)
{
    roots.clear();
    buildContext.selectedExportRoots.clear();
    if (exportModeSet.empty() || !File::CheckDir(profPath_))
    {
        ERROR("Invalid export modes or profile path.");
        return false;
    }
    ExportSelection selection;
    if (!GetExportSelection(exportModeSet, selection, buildContext.strictSelection))
    {
        return false;
    }

    buildContext.profPath = profPath_;
    for (const auto& exportMode : exportModeSet)
    {
        switch (exportMode)
        {
            case ExportMode::DB:
            {
                if (selection.dbProcesses.empty() || HasExportedMsprofDB(profPath_))
                {
                    INFO("Nothing to export for the DB deliverable, skip DB export.");
                    break;
                }
                buildContext.dbPath = DBAssembler::MakeDBPath(profPath_);
                if (buildContext.dbPath.empty())
                {
                    ERROR("Prepare DB export path failed.");
                    return false;
                }
                DBAssembler::GetTopologyRoots(selection.dbProcesses, roots);
                break;
            }
            case ExportMode::TIMELINE:
                if (!selection.timelineProcesses.empty())
                {
                    TimelineManager::GetTopologyRoots(selection.timelineProcesses, roots);
                }
                break;
            case ExportMode::SUMMARY:
                if (!selection.summaryProcesses.empty())
                {
                    SummaryManager::GetTopologyRoots(selection.summaryProcesses, roots);
                }
                break;
            default:
                ERROR("Unsupported ExportMode: %.", static_cast<int>(exportMode));
                return false;
        }
    }
    RemoveDuplicateRoots(roots);
    return true;
}

bool ExportManager::GetExportSelection(const std::set<ExportMode>& exportModeSet, ExportSelection& selection,
                                       bool strict)
{
    selection.timelineProcesses.clear();
    selection.summaryProcesses.clear();
    selection.dbProcesses.clear();
    const bool exportTimeline = exportModeSet.find(ExportMode::TIMELINE) != exportModeSet.end();
    const bool exportSummary = exportModeSet.find(ExportMode::SUMMARY) != exportModeSet.end();
    const bool exportDB = exportModeSet.find(ExportMode::DB) != exportModeSet.end();
    if (!exportTimeline && !exportSummary && !exportDB)
    {
        return true;
    }
    // The report file decides the whole selection, so the defaults are only expanded when it is not given.
    if (jsonPath_.empty())
    {
        ApplyDefaultSelection(exportTimeline, exportSummary, exportDB, selection);
        INFO("The report parameter is not used.");
        return true;
    }
    FileReader fd(jsonPath_);
    nlohmann::json config;
    if (fd.ReadJson(config) != ANALYSIS_OK)
    {
        if (strict)
        {
            ERROR("Load report config failed: '%'.", jsonPath_);
            return false;
        }
        ApplyDefaultSelection(exportTimeline, exportSummary, exportDB, selection);
        INFO("The report config is unreadable and ignored, all default deliverables are kept.");
        return true;
    }
    INFO("Load report config from '%', content: %.", jsonPath_, config.dump());
    if (!config.is_object())
    {
        ERROR("Report selection must be a JSON object.");
        if (strict)
        {
            return false;
        }
        ApplyDefaultSelection(exportTimeline, exportSummary, exportDB, selection);
        INFO("The report config is not a JSON object and is ignored, all default deliverables are kept.");
        return true;
    }
    if (exportTimeline)
    {
        const auto appendTimelineProcess = [&selection](const std::string& name)
        {
            if (GetTimelineNodeKey(name).empty())
            {
                return false;
            }
            selection.timelineProcesses.emplace_back(name);
            return true;
        };
        const auto state =
            ParseReportSection(config["json_process"], "json_process", "Timeline", strict, appendTimelineProcess);
        if (state == ReportSectionState::INVALID)
        {
            if (strict)
            {
                return false;
            }
            selection.timelineProcesses.clear();
            selection.timelineProcesses = GetAllTimelineJsonKeys();
        }
    }
    if (exportSummary)
    {
        const auto appendSummaryProcess = [&selection](const std::string& name)
        {
            if (GetSummaryNodeKey(name).empty())
            {
                return false;
            }
            selection.summaryProcesses.emplace_back(name);
            return true;
        };
        const auto state =
            ParseReportSection(config["summary_process"], "summary_process", "Summary", strict, appendSummaryProcess);
        if (state == ReportSectionState::INVALID)
        {
            if (strict)
            {
                return false;
            }
            selection.summaryProcesses.clear();
            selection.summaryProcesses = GetAllSummaryJsonKeys();
        }
    }
    if (exportDB)
    {
        const auto appendDBProcess = [&selection](const std::string& name)
        {
            if (GetDBSaverNames(name).empty())
            {
                return false;
            }
            selection.dbProcesses.emplace_back(name);
            return true;
        };
        const auto state = ParseReportSection(config["db_process"], "db_process", "DB", strict, appendDBProcess);
        if (state == ReportSectionState::INVALID)
        {
            if (strict)
            {
                return false;
            }
            selection.dbProcesses.clear();
            selection.dbProcesses = GetAllDBJsonKeys();
        }
    }
    return true;
}
}  // namespace Application
}  // namespace Analysis
