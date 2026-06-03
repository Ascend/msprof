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

#include "analysis/csrc/application/include/export_manager.h"
#include <atomic>
#include "analysis/csrc/application/database/db_assembler.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/data_process/include/data_processor_factory.h"
#include "analysis/csrc/domain/data_process/ai_task/hash_init_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"
#include "analysis/csrc/application/summary/summary_manager.h"
#include "analysis/csrc/application/timeline/timeline_manager.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Domain;
namespace {
std::string CreateOutputPath(const std::string& profPath)
{
    std::string outputPath = File::PathJoin({profPath, OUTPUT_PATH});
    if (!File::Exist(outputPath) && !File::CreateDir(outputPath)) {
        ERROR("Create mindstudio_profiler_output error, can't export data");
        PRINT_ERROR("Create mindstudio_profiler_output error, can't export data");
        return "";
    }
    return outputPath;
}
}

bool ExportManager::ProcessData(DataInventory &dataInventory, const std::set<ExportMode>& exportModeSet)
{
    // hash数据作为其他流程的依赖数据，需要优先加载
    HashInitProcessor hashProcessor(profPath_);
    hashProcessor.Run(dataInventory, PROCESSOR_NAME_HASH);
    const uint16_t tableProcessors = 10; // 最多有10个线程
    Analysis::Utils::ThreadPool pool(tableProcessors);
    pool.Start();
    std::atomic<bool> retFlag(true);
    static const std::unordered_map<ExportMode, const std::set<std::string>&(*)()>
        processListFactory = {
        {ExportMode::DB,       &DBAssembler::GetProcessList},
        {ExportMode::TIMELINE, &TimelineManager::GetProcessList},
        {ExportMode::SUMMARY,  &SummaryManager::GetProcessList}
    };

    std::set<std::string> dataProcessList;
    for (auto exportMode : exportModeSet) {
        auto it = processListFactory.find(exportMode);
        if (it == processListFactory.end()) {
            ERROR("Unsupported ExportMode: %.", static_cast<int>(exportMode));
            return false;
        }
        auto tmpSet = it->second();
        dataProcessList.insert(tmpSet.begin(), tmpSet.end());
    }

    for (const auto& name : dataProcessList) {
        pool.AddTask([this, &name, &retFlag, &dataInventory]() {
            auto processor = DataProcessorFactory::GetDataProcessByName(profPath_, name);
            if (processor == nullptr) {
                ERROR("% is not defined", name);
                retFlag = false;
                return;
            }
            retFlag = processor->Run(dataInventory, name) && retFlag;
        });
    }
    pool.WaitAllTasks();
    pool.Stop();
    if (!retFlag) {
        ERROR("The % for data process failed to be executed.", profPath_);
        PRINT_ERROR("The % for data process failed to be executed. "
                    "Please check msprof_analysis_log in outputPath for more info.", profPath_);
        return false;
    }
    return true;
}

bool ExportManager::CheckProfDirsValid()
{
    if (profPath_.find("PROF") == std::string::npos) {
        ERROR("The path: % is not contains PROF, please check", profPath_);
        return false;
    }
    return true;
}

bool ExportManager::Init()
{
    if (!CheckProfDirsValid()) {
        ERROR("Check TimelineManager output path failed, path is %.", profPath_);
        PRINT_ERROR("Check TimelineManager output path failed, path is %. "
                    "Please check msprof_analysis_log in outputPath for more info.", profPath_);
        return false;
    }
    if (!Analysis::Domain::Environment::Context::GetInstance().Load({profPath_})) {
        ERROR("JSON parameter loading failed. Please check if the JSON data is complete.");
        PRINT_ERROR("JSON parameter loading failed. Please check if the JSON data is complete. "
                    "Please check msprof_analysis_log in outputPath for more info.");
        return false;
    }
    return true;
}

bool ExportManager::Run(const std::set<ExportMode>& exportModeSet)
{
    INFO("Start to load data!");
    PRINT_INFO("Start to load data!");
    if (!Init()) {
        return false;
    }
    DataInventory dataInventory;
    std::atomic<bool> runFlag(true);
    runFlag = ProcessData(dataInventory, exportModeSet);
    const std::map<ExportMode, std::function<bool(DataInventory&)>> operationMap = {
        {ExportMode::DB,       [this](DataInventory& dataInventory) -> bool {
            DBAssembler dbAssembler(profPath_, profPath_);
            return dbAssembler.Run(dataInventory);
        }},
        {ExportMode::TIMELINE, [this](DataInventory& dataInventory) -> bool {
            std::string outputPath = CreateOutputPath(profPath_);
            if (outputPath.empty()) {
                return false;
            }
            TimelineManager timelineManager(profPath_, outputPath);
            std::vector<JsonProcess> jsonProcesses = GetProcessEnum();
            return timelineManager.Run(dataInventory, jsonProcesses);
        }},
        {ExportMode::SUMMARY, [this](DataInventory& dataInventory) -> bool {
            std::string outputPath = CreateOutputPath(profPath_);
            if (outputPath.empty()) {
                return false;
            }
            SummaryManager summaryManager(profPath_, outputPath);
            return summaryManager.Run(dataInventory);
        }},
    };

    const uint16_t processorsLimit = 3; // 最多有3个线程
    Analysis::Utils::ThreadPool pool(processorsLimit);
    pool.Start();
    for (const auto& exportMode : exportModeSet) {
        auto iter = operationMap.find(exportMode);
        if (iter != operationMap.end()) {
            pool.AddTask([iter, &runFlag, &dataInventory]() {
                runFlag = iter->second(dataInventory) && runFlag;
            });
        }
    }
    pool.WaitAllTasks();
    pool.Stop();
    return runFlag;
}

std::vector<JsonProcess> ExportManager::GetProcessEnum()
{
    if (jsonPath_.empty()) {
        INFO("The report parameter is not used.");
        PRINT_INFO("The report parameter is not used.");
        return allProcesses;
    }
    FileReader fd(jsonPath_);
    nlohmann::json config;
    if (fd.ReadJson(config) != ANALYSIS_OK) {
        ERROR("Load report config failed: '%'.", jsonPath_);
        PRINT_ERROR("Load report config failed: '%'.", jsonPath_);
        return allProcesses;
    }
    auto jsonProcessConfig = config["json_process"];
    if (jsonProcessConfig.is_null() || !jsonProcessConfig.is_object() || jsonProcessConfig.empty()) {
        INFO("The json_process is not exist.");
        PRINT_INFO("The json_process is not exist.");
        return allProcesses;
    }
    std::vector<JsonProcess> jsonProcesses;
    for (nlohmann::json::iterator it = jsonProcessConfig.begin(); it != jsonProcessConfig.end(); ++it) {
        if (strToJsonProcess.find(it.key()) == strToJsonProcess.end()) {
            ERROR("Json process contains invalid key.");
            PRINT_ERROR("Json process contains invalid key.");
            return allProcesses;
        }
        if (!it.value().is_boolean()) {
            ERROR("Json contains invalid value, only the bool type is supported.");
            PRINT_ERROR("Json contains invalid value, only the bool type is supported.");
            return allProcesses;
        }
        if (it.value()) {
            auto processEnum = strToJsonProcess.at(it.key());
            jsonProcesses.push_back(processEnum);
            if (it.key() == "freq") {
                jsonProcesses.push_back(JsonProcess::LOW_POWER);
            }
        }
    }
    return std::move(jsonProcesses);
}
}
}