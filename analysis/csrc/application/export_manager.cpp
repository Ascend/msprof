/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 * http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/application/include/export_manager.h"

#include <algorithm>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "analysis/csrc/application/database/db_assembler.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/summary/summary_manager.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/application/timeline/timeline_manager.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_control.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain;
namespace
{
std::string CreateOutputPath(const std::string& profPath)
{
    std::string outputPath = File::PathJoin({profPath, Analysis::Common::OUTPUT_PATH});
    if (!File::Exist(outputPath) && !File::CreateDir(outputPath))
    {
        ERROR("Create mindstudio_profiler_output error, can't export data");
        PRINT_ERROR("Create mindstudio_profiler_output error, can't export data");
        return "";
    }
    return outputPath;
}

bool HasExportedMsprofDB(const std::string& profPath)
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

}  // namespace

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

    ExportSelection selection;
    if (!GetExportSelection(exportModeSet, selection))
    {
        return false;
    }

    TopoBuildContext buildContext;
    buildContext.profPath = profPath_;
    buildContext.outputPath = profPath_;
    buildContext.timelineProcesses = selection.timelineProcesses;
    std::vector<TopoNodeId> roots;
    for (const auto& exportMode : exportModeSet)
    {
        switch (exportMode)
        {
            case ExportMode::DB:
            {
                if (HasExportedMsprofDB(profPath_))
                {
                    INFO("The exported msprof db already exists, skip DB export.");
                    break;
                }
                buildContext.dbSession =
                    std::shared_ptr<DBAssembler>(new (std::nothrow) DBAssembler(profPath_, profPath_));
                if (buildContext.dbSession == nullptr || !DBAssembler::GetTopologyRoots(roots))
                {
                    ERROR("Build DB topology roots failed.");
                    return false;
                }
                break;
            }
            case ExportMode::TIMELINE:
            {
                const std::string outputPath = CreateOutputPath(profPath_);
                if (outputPath.empty())
                {
                    return false;
                }
                buildContext.outputPath = outputPath;
                buildContext.timelineSession =
                    std::shared_ptr<TimelineManager>(new (std::nothrow) TimelineManager(profPath_, outputPath));
                if (buildContext.timelineSession == nullptr ||
                    !TimelineManager::GetTopologyRoots(selection.timelineProcesses, roots))
                {
                    return false;
                }
                break;
            }
            case ExportMode::SUMMARY:
                if (CreateOutputPath(profPath_).empty() ||
                    !SummaryManager::GetTopologyRoots(selection.summaryDeliverables, roots))
                {
                    return false;
                }
                break;
            default:
                ERROR("Unsupported ExportMode: %.", static_cast<int>(exportMode));
                return false;
        }
    }

    Infra::ProcessCollection processes;
    TopoGraphBuilder graphBuilder;
    if (!graphBuilder.Build(buildContext, roots, processes))
    {
        ERROR("Build export topology failed.");
        return false;
    }

    DataInventory dataInventory;
    Infra::Context context;
    Infra::ProcessControl processControl(processes);
    if (!processControl.ExecuteProcess(dataInventory, context))
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

bool ExportManager::GetExportSelection(const std::set<ExportMode>& exportModeSet, ExportSelection& selection)
{
    selection.timelineProcesses.clear();
    selection.summaryDeliverables.clear();
    const bool exportTimeline = exportModeSet.find(ExportMode::TIMELINE) != exportModeSet.end();
    const bool exportSummary = exportModeSet.find(ExportMode::SUMMARY) != exportModeSet.end();
    if (exportTimeline)
    {
        selection.timelineProcesses = allProcesses;
    }
    if (jsonPath_.empty())
    {
        INFO("The report parameter is not used.");
        PRINT_INFO("The report parameter is not used.");
        return true;
    }
    FileReader fd(jsonPath_);
    nlohmann::json config;
    if (fd.ReadJson(config) != ANALYSIS_OK)
    {
        ERROR("Load report config failed: '%'.", jsonPath_);
        PRINT_ERROR("Load report config failed: '%'.", jsonPath_);
        return true;
    }

    if (exportTimeline)
    {
        const auto jsonProcessConfig = config["json_process"];
        if (jsonProcessConfig.is_null() || !jsonProcessConfig.is_object() || jsonProcessConfig.empty())
        {
            INFO("The json_process is not exist.");
            PRINT_INFO("The json_process is not exist.");
        }
        else
        {
            std::vector<JsonProcess> jsonProcesses;
            bool valid = true;
            for (nlohmann::json::const_iterator it = jsonProcessConfig.begin(); it != jsonProcessConfig.end(); ++it)
            {
                if (strToJsonProcess.find(it.key()) == strToJsonProcess.end())
                {
                    ERROR("Json process contains invalid key.");
                    PRINT_ERROR("Json process contains invalid key.");
                    valid = false;
                    break;
                }
                if (!it.value().is_boolean())
                {
                    ERROR("Json contains invalid value, only the bool type is supported.");
                    PRINT_ERROR("Json contains invalid value, only the bool type is supported.");
                    valid = false;
                    break;
                }
                if (it.value())
                {
                    const auto processEnum = strToJsonProcess.at(it.key());
                    jsonProcesses.emplace_back(processEnum);
                    if (it.key() == "freq")
                    {
                        jsonProcesses.emplace_back(JsonProcess::LOW_POWER);
                    }
                }
            }
            if (valid)
            {
                if (jsonProcesses.empty())
                {
                    ERROR("Json process has no enabled timeline deliverable.");
                    return false;
                }
                selection.timelineProcesses = std::move(jsonProcesses);
            }
        }
    }

    if (exportSummary)
    {
        const auto summaryProcessConfig = config["summary_process"];
        if (summaryProcessConfig.is_null())
        {
            return true;
        }
        if (!summaryProcessConfig.is_object())
        {
            ERROR("Summary process must be a JSON object.");
            PRINT_ERROR("Summary process must be a JSON object.");
            return false;
        }
        if (summaryProcessConfig.empty())
        {
            INFO("The summary_process is empty.");
            PRINT_INFO("The summary_process is empty.");
            return true;
        }

        std::vector<std::string> summaryDeliverables;
        for (nlohmann::json::const_iterator it = summaryProcessConfig.begin(); it != summaryProcessConfig.end(); ++it)
        {
            if (!SummaryManager::IsDeliverableSupported(it.key()))
            {
                ERROR("Summary process contains invalid key.");
                PRINT_ERROR("Summary process contains invalid key.");
                return false;
            }
            if (!it.value().is_boolean())
            {
                ERROR("Summary process value must be bool.");
                PRINT_ERROR("Summary process value must be bool.");
                return false;
            }
            if (it.value())
            {
                summaryDeliverables.emplace_back(it.key());
            }
        }
        if (summaryDeliverables.empty())
        {
            ERROR("Summary process has no enabled deliverable.");
            return false;
        }
        selection.summaryDeliverables = std::move(summaryDeliverables);
    }
    return true;
}
}  // namespace Application
}  // namespace Analysis
