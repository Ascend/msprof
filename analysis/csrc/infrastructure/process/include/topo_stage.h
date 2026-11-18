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
#ifndef ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_TOPO_STAGE_H
#define ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_TOPO_STAGE_H

#include <cstdint>

namespace Analysis
{
namespace Application
{
// TopoNodeStage is part of the node identity, and it also travels with the runtime process so a failure report can
// tell the data processing node and the export node that share one name apart.
enum class TopoNodeStage : uint8_t
{
    DATA_PROCESSING,       // Domain data processing.
    SUMMARY_GENERATION,    // Application summary generation.
    DATABASE_PERSISTENCE,  // Application database persistence.
    TIMELINE_EXPORT,       // Application timeline export.
    FLOW_CONTROL,          // Topology lifecycle control.
    HOST_PROCESSING,
    DEVICE_PROCESSING,
    DEVICE_PERSISTENCE,  // Device intermediate SQLite persistence.
    // Runtime nodes assembled without a topology definition keep this stage, and the diagnostics drop it.
    UNKNOWN,
};

// The stage name used by the diagnostics, operators read it in the analysis log, so it never follows code renames.
inline const char* TopoStageName(TopoNodeStage stage)
{
    switch (stage)
    {
        case TopoNodeStage::DATA_PROCESSING:
            return "DATA_PROCESSING";
        case TopoNodeStage::SUMMARY_GENERATION:
            return "SUMMARY_GENERATION";
        case TopoNodeStage::DATABASE_PERSISTENCE:
            return "DATABASE_PERSISTENCE";
        case TopoNodeStage::TIMELINE_EXPORT:
            return "TIMELINE_EXPORT";
        case TopoNodeStage::FLOW_CONTROL:
            return "FLOW_CONTROL";
        case TopoNodeStage::HOST_PROCESSING:
            return "HOST_PROCESSING";
        case TopoNodeStage::DEVICE_PROCESSING:
            return "DEVICE_PROCESSING";
        case TopoNodeStage::DEVICE_PERSISTENCE:
            return "DEVICE_PERSISTENCE";
        case TopoNodeStage::UNKNOWN:
            break;
    }
    return "";
}
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_TOPO_STAGE_H
