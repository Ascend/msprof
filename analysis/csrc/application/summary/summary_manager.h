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

#ifndef ANALYSIS_APPLICATION_SUMMARY_MANAGER_H
#define ANALYSIS_APPLICATION_SUMMARY_MANAGER_H

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/process/include/topo_graph.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Infra;

class SummaryManager
{
   public:
    explicit SummaryManager(const std::string& profPath, const std::string& outputPath)
        : profPath_(profPath), outputPath_(outputPath) {};
    static bool IsDeliverableSupported(const std::string& deliverableName);
    static TopoNodeCreatorFactory CreateSummaryAssembler(const std::string& name);
    static bool GetAssemblerList(const std::vector<std::string>& deliverableNames,
                                 std::vector<std::string>& assemblerNames);
    static bool GetTopologyRoots(const std::vector<std::string>& deliverableNames, std::vector<TopoNodeId>& roots);

   private:
    std::string profPath_;
    std::string outputPath_;
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_SUMMARY_MANAGER_H
