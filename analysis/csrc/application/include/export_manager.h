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

#ifndef ANALYSIS_APPLICATION_EXPORT_MANAGER_H
#define ANALYSIS_APPLICATION_EXPORT_MANAGER_H

#include <set>
#include <string>
#include <vector>

#include "analysis/csrc/application/include/export_mode_enum.h"
#include "analysis/csrc/application/timeline/json_process_enum.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Infra;

struct ExportSelection
{
    std::vector<JsonProcess> timelineProcesses;
    std::vector<std::string> summaryDeliverables;
};

class ExportManager
{
   public:
    explicit ExportManager(const std::string& profPath) : profPath_(profPath) {}
    ExportManager(const std::string& profPath, const std::string& jsonPath) : profPath_(profPath), jsonPath_(jsonPath)
    {
    }
    bool Run(const std::set<ExportMode>& exportModeSet);
    bool GetExportSelection(const std::set<ExportMode>& exportModeSet, ExportSelection& selection);

   private:
    bool Init();
    bool CheckProfDirsValid();

   private:
    std::string profPath_;
    std::string jsonPath_;
};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_EXPORT_MANAGER_H
