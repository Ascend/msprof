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

// Device topology configuration. Business implementations contain no registration side effects.
#include <map>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/ascend_obj.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/entities/hal/include/hal.h"
#include "analysis/csrc/domain/entities/hal/include/hal_freq.h"
#include "analysis/csrc/domain/entities/hal/include/hal_llc_pcie.h"
#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/entities/hal/include/hal_npu_mem.h"
#include "analysis/csrc/domain/entities/hal/include/hal_npu_module_mem.h"
#include "analysis/csrc/domain/entities/hal/include/hal_qos.h"
#include "analysis/csrc/domain/entities/hal/include/hal_ub.h"
#include "analysis/csrc/domain/entities/hal/include/stream_expand_spec.h"
#include "analysis/csrc/domain/entities/hal/include/top_down_task.h"
#include "analysis/csrc/domain/entities/hccl/include/hccl_task.h"
#include "analysis/csrc/domain/entities/hccl/include/kfc_task.h"
#include "analysis/csrc/domain/services/association/calculator/hccl/include/hccl_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/hccl/include/kfc_calculator.h"
#include "analysis/csrc/domain/services/association/include/ascend_task_association.h"
#include "analysis/csrc/domain/services/association/include/pmu_association.h"
#include "analysis/csrc/domain/services/device_context/load_host_data.h"
#include "analysis/csrc/domain/services/device_context/load_stream_expand_spec_data.h"
#include "analysis/csrc/domain/services/init/include/device_task_process.h"
#include "analysis/csrc/domain/services/modeling/include/log_modeling.h"
#include "analysis/csrc/domain/services/modeling/include/pmu_modeling.h"
#include "analysis/csrc/domain/services/modeling/step_trace/include/step_trace_process.h"
#include "analysis/csrc/domain/services/parser/aicpu/include/aicpu_parser.h"
#include "analysis/csrc/domain/services/parser/ccu_parser.h"
#include "analysis/csrc/domain/services/parser/freq/include/freq_parser.h"
#include "analysis/csrc/domain/services/parser/freq/include/lpm_info_parser.h"
#include "analysis/csrc/domain/services/parser/llc/include/llc_parser.h"
#include "analysis/csrc/domain/services/parser/log/include/stars_soc_parser.h"
#include "analysis/csrc/domain/services/parser/log/include/stars_soc_profile_parser.h"
#include "analysis/csrc/domain/services/parser/npu_mem/include/npu_mem_parser.h"
#include "analysis/csrc/domain/services/parser/npu_module_mem/include/npu_module_mem_parser.h"
#include "analysis/csrc/domain/services/parser/pcie/include/pcie_parser.h"
#include "analysis/csrc/domain/services/parser/pmu/include/ffts_profile_parser.h"
#include "analysis/csrc/domain/services/parser/qos/include/qos_parser.h"
#include "analysis/csrc/domain/services/parser/track/include/ts_track_parser.h"
#include "analysis/csrc/domain/services/parser/ub/include/ub_parser.h"
#include "analysis/csrc/domain/services/persistence/device/acc_pmu_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/aicpu_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/ascend_task_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/ccu_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/chip_trans_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/device_hccl_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/freq_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/inter_soc_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/llc_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/low_power_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/lpm_info_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/metric_summary_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/npu_mem_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/npu_module_mem_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/pcie_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/qos_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/sio_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/step_trace_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/trace_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/ts_track_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/ub_persistence.h"
#include "analysis/csrc/infrastructure/process/include/topo_graph.h"

namespace Analysis
{
namespace Domain
{
using namespace Application;
namespace
{
template <typename T>
TopoNodeDefinition DeviceDefinition(TopoNodeId id, bool mandatory, std::vector<TopoNodeId> dependencies,
                                    std::vector<uint32_t> chips, TopoDependencyResolver resolver = nullptr)
{
    TopoNodeDefinition definition;
    definition.id = std::move(id);
    definition.scopeKind = TopoScopeKind::DEVICE;
    definition.runtimeType = typeid(T);
    definition.mandatory = mandatory;
    definition.chipIds = std::move(chips);
    definition.processDependencies = std::move(dependencies);
    definition.processDependencies.push_back(TOPO_NODE(DEVICE_PROCESSING, "Device.Init"));
    definition.dependencyResolver = std::move(resolver);
    definition.creatorFactory = [](const TopoBuildContext&) -> Infra::ProcessCreator
    {
        return
            []() -> std::unique_ptr<Infra::Process> { return std::unique_ptr<Infra::Process>(new (std::nothrow) T); };
    };
    return definition;
}

TopoDependencyResolver ChipDependencies(std::vector<std::string> names)
{
    return [names](const TopoBuildContext& context, const std::vector<TopoNodeId>&)
    {
        std::vector<TopoNodeId> dependencies;
        for (const auto& name : names)
        {
            const auto id = TOPO_NODE(DEVICE_PROCESSING, name);
            const auto* definition = TopoNodeRegistry::Find(id);
            // A missing declared implementation is a planning error, not a chip alternative.
            if (!definition || TopoNodeRegistry::SupportsChip(*definition, context.scopeChips.at(context.currentScope)))
            {
                dependencies.push_back(id);
            }
        }
        return dependencies;
    };
}

// Wait for Host only at consumers of its generated databases. Device-only and legacy
// single-device requests have no Host parse root and keep reading existing Host results.
TopoDependencyResolver HostDataDependencies(TopoDependencyResolver deviceDependencies = nullptr)
{
    return [deviceDependencies](const TopoBuildContext& context, const std::vector<TopoNodeId>& roots)
    {
        std::vector<TopoNodeId> dependencies;
        if (deviceDependencies)
        {
            dependencies = deviceDependencies(context, roots);
        }
        for (const auto& root : context.parseRoots)
        {
            if (root.stage == TopoNodeStage::HOST_PROCESSING)
            {
                dependencies.push_back(root);
            }
        }
        return dependencies;
    };
}

}  // namespace

// Parsers.
REGISTER_TOPO_NODE_DEFINITION(
    DeviceDefinition<AicpuParser>(TOPO_NODE(DEVICE_PROCESSING, "AicpuParser"), true,
                                  TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "LoadStreamExpandSpec")), {CHIP_ID_ALL}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PROCESSING, "AicpuParser"), StreamExpandSpec);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<CcuChannelParser>(TOPO_NODE(DEVICE_PROCESSING, "CcuChannelParser"), true,
                                                                 TOPO_DEPS(), {CHIP_V6_1_0, CHIP_V6_1_1}));

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<CcuMissionParser>(TOPO_NODE(DEVICE_PROCESSING, "CcuMissionParser"), true,
                                                                 TOPO_DEPS(), {CHIP_V6_1_0, CHIP_V6_1_1},
                                                                 HostDataDependencies()));

REGISTER_TOPO_NODE_DEFINITION(
    DeviceDefinition<FftsProfileParser>(TOPO_NODE(DEVICE_PROCESSING, "FftsProfileParser"), true,
                                        TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "LoadStreamExpandSpec")),
                                        {CHIP_V4_1_0, CHIP_V6_1_0, CHIP_V6_1_1, CHIP_V6_2_0}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PROCESSING, "FftsProfileParser"), StreamExpandSpec);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<FreqParser>(TOPO_NODE(DEVICE_PROCESSING, "FreqParser"), true,
                                                           TOPO_DEPS(), {CHIP_V1_1_1, CHIP_V6_1_0, CHIP_V6_1_1}));

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<LpmInfoParser>(TOPO_NODE(DEVICE_PROCESSING, "LpmInfoParser"), true,
                                                              TOPO_DEPS(), {CHIP_V4_1_0}));

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<LlcParser>(TOPO_NODE(DEVICE_PROCESSING, "LlcParser"), true, TOPO_DEPS(),
                                                          {CHIP_V4_1_0, CHIP_V6_1_0}));

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<PcieParser>(TOPO_NODE(DEVICE_PROCESSING, "PcieParser"), true,
                                                           TOPO_DEPS(), {CHIP_V4_1_0, CHIP_V6_1_0}));

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<NpuMemParser>(TOPO_NODE(DEVICE_PROCESSING, "NpuMemParser"), true,
                                                             TOPO_DEPS(), {CHIP_V4_1_0, CHIP_V6_1_0}));

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<NpuModuleMemParser>(TOPO_NODE(DEVICE_PROCESSING, "NpuModuleMemParser"),
                                                                   true, TOPO_DEPS(), {CHIP_V4_1_0, CHIP_V6_1_0}));

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<QosParser>(TOPO_NODE(DEVICE_PROCESSING, "QosParser"), false, TOPO_DEPS(),
                                                          {CHIP_V4_1_0}));

REGISTER_TOPO_NODE_DEFINITION(
    DeviceDefinition<StarsSocParser>(TOPO_NODE(DEVICE_PROCESSING, "StarsSocParser"), true,
                                     TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "LoadStreamExpandSpec")), {CHIP_V4_1_0}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PROCESSING, "StarsSocParser"), StreamExpandSpec);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<StarsSocParserV6>(TOPO_NODE(DEVICE_PROCESSING, "StarsSocParserV6"), true,
                                                                 TOPO_DEPS(), {CHIP_V6_1_0, CHIP_V6_1_1}));

REGISTER_TOPO_NODE_DEFINITION(
    DeviceDefinition<StarsSocProfileParser>(TOPO_NODE(DEVICE_PROCESSING, "StarsSocProfileParser"), false, TOPO_DEPS(),
                                            {CHIP_V4_1_0, CHIP_V1_1_1, CHIP_V6_1_0}));

REGISTER_TOPO_NODE_DEFINITION(
    DeviceDefinition<TsTrackParser>(TOPO_NODE(DEVICE_PROCESSING, "TsTrackParser"), true,
                                    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "LoadStreamExpandSpec")), {CHIP_ID_ALL}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PROCESSING, "TsTrackParser"), StreamExpandSpec);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<UbParser>(TOPO_NODE(DEVICE_PROCESSING, "UbParser"), false, TOPO_DEPS(),
                                                         {CHIP_V6_1_0, CHIP_V6_1_1}));

// Device inputs and modeling.
REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<DeviceTaskProcess>(TOPO_NODE(DEVICE_PROCESSING, "DeviceTaskProcess"),
                                                                  true, TOPO_DEPS(), {CHIP_ID_ALL}));

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<LoadHostData>(
    TOPO_NODE(DEVICE_PROCESSING, "LoadHostData"), false, TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "DeviceTaskProcess")),
    {CHIP_ID_ALL}, HostDataDependencies(ChipDependencies({"LogModeling", "LogModelingV6"}))));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PROCESSING, "LoadHostData"),
                                  std::map<TaskId, std::vector<Domain::DeviceTask>>, AicpuOpNameMap,
                                  std::vector<Mc2CommInfo>);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<LoadStreamExpandSpec>(
    TOPO_NODE(DEVICE_PROCESSING, "LoadStreamExpandSpec"), true, TOPO_DEPS(), {CHIP_ID_ALL}, HostDataDependencies()));

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<LogModeling>(
    TOPO_NODE(DEVICE_PROCESSING, "LogModeling"), true,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "StarsSocParser"), TOPO_NODE(DEVICE_PROCESSING, "TsTrackParser"),
              TOPO_NODE(DEVICE_PROCESSING, "DeviceTaskProcess")),
    {CHIP_V4_1_0}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PROCESSING, "LogModeling"), std::vector<Domain::HalLogData>,
                                  std::vector<Domain::HalTrackData>,
                                  std::map<Domain::TaskId, std::vector<Domain::DeviceTask>>);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<LogModelingV6>(
    TOPO_NODE(DEVICE_PROCESSING, "LogModelingV6"), true,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "StarsSocParserV6"), TOPO_NODE(DEVICE_PROCESSING, "TsTrackParser"),
              TOPO_NODE(DEVICE_PROCESSING, "DeviceTaskProcess")),
    {CHIP_V6_1_0, CHIP_V6_1_1}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PROCESSING, "LogModelingV6"), std::vector<Domain::HalLogData>,
                                  std::vector<Domain::HalTrackData>,
                                  std::map<Domain::TaskId, std::vector<Domain::DeviceTask>>);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<PmuModeling>(TOPO_NODE(DEVICE_PROCESSING, "PmuModeling"), true,
                                                            TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "FftsProfileParser"),
                                                                      TOPO_NODE(DEVICE_PROCESSING, "TsTrackParser")),
                                                            {CHIP_V4_1_0, CHIP_V6_1_0, CHIP_V6_1_1, CHIP_V6_2_0}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PROCESSING, "PmuModeling"), std::vector<HalPmuData>,
                                  std::vector<HalTrackData>);

REGISTER_TOPO_NODE_DEFINITION(
    DeviceDefinition<StepTraceProcess>(TOPO_NODE(DEVICE_PROCESSING, "StepTraceProcess"), false,
                                       TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "TsTrackParser")), {CHIP_ID_ALL}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PROCESSING, "StepTraceProcess"), std::vector<HalTrackData>);

// Association and calculations.
REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<AscendTaskAssociation>(
    TOPO_NODE(DEVICE_PROCESSING, "AscendTaskAssociation"), true,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "LoadHostData"), TOPO_NODE(DEVICE_PROCESSING, "AicpuParser")),
    {CHIP_ID_ALL}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PROCESSING, "AscendTaskAssociation"),
                                  std::map<TaskId, std::vector<DeviceTask>>, std::map<TaskId, std::vector<HostTask>>,
                                  HostStreamInfo, DeviceStreamInfo);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<HcclCalculator>(
    TOPO_NODE(DEVICE_PROCESSING, "HcclCalculator"), true,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "AscendTaskAssociation"), TOPO_NODE(DEVICE_PROCESSING, "LoadHostData")),
    {CHIP_ID_ALL}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PROCESSING, "HcclCalculator"), std::vector<TopDownTask>,
                                  std::vector<HcclTask>, std::vector<HcclOp>);

REGISTER_TOPO_NODE_DEFINITION(
    DeviceDefinition<KfcCalculator>(TOPO_NODE(DEVICE_PROCESSING, "KfcCalculator"), true,
                                    TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "AicpuPersistence"),
                                              TOPO_NODE(DEVICE_PERSISTENCE, "AscendTaskPersistence")),
                                    {CHIP_ID_ALL}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PROCESSING, "KfcCalculator"), std::vector<KfcInfoData>,
                                  std::vector<MasterStreamTaskData>, std::vector<HcclOp>, std::vector<TopDownTask>,
                                  AicpuOpNameMap, std::vector<Mc2CommInfo>);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<PmuAssociation>(
    TOPO_NODE(DEVICE_PROCESSING, "PmuAssociation"), true,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "LoadHostData"), TOPO_NODE(DEVICE_PROCESSING, "AscendTaskAssociation")),
    {CHIP_ID_ALL}, ChipDependencies({"PmuModeling"})));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PROCESSING, "PmuAssociation"), std::vector<HalPmuData>,
                                  std::map<TaskId, std::vector<DeviceTask>>, std::vector<HalFreqLpmData>,
                                  HostStreamInfo);

// Persistence.
REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<AccPmuPersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "AccPmuPersistence"), true, TOPO_DEPS(), {CHIP_V4_1_0, CHIP_V6_1_0},
    ChipDependencies({"StarsSocParser", "StarsSocParserV6", "StarsSocProfileParser"})));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "AccPmuPersistence"), std::vector<HalLogData>,
                                  HalSocProfileData);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<AicpuPersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "AicpuPersistence"), true,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "AicpuParser"), TOPO_NODE(DEVICE_PROCESSING, "LoadHostData"),
              TOPO_NODE(DEVICE_PROCESSING, "TsTrackParser")),
    {CHIP_ID_ALL}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "AicpuPersistence"), std::vector<AicpuData>,
                                  DeviceStreamInfo, HostStreamInfo, GeHashMap, std::vector<HalTrackData>,
                                  std::vector<KfcInfoData>, std::vector<MasterStreamTaskData>);

REGISTER_TOPO_NODE_DEFINITION(
    DeviceDefinition<AscendTaskPersistence>(TOPO_NODE(DEVICE_PERSISTENCE, "AscendTaskPersistence"), true,
                                            TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "HcclCalculator")), {CHIP_ID_ALL}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "AscendTaskPersistence"), std::vector<TopDownTask>);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<CcuPersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "CcuPersistence"), true,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "CcuMissionParser"), TOPO_NODE(DEVICE_PROCESSING, "CcuChannelParser")),
    {CHIP_V6_1_0, CHIP_V6_1_1}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "CcuPersistence"), CcuMissionDataSet,
                                  CcuChannelDataSet);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<DeviceHcclPersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "DeviceHcclPersistence"), true,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "HcclCalculator"), TOPO_NODE(DEVICE_PROCESSING, "KfcCalculator")),
    {CHIP_ID_ALL}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "DeviceHcclPersistence"), std::vector<DeviceHcclOp>,
                                  std::vector<DeviceHcclTask>, std::vector<HcclStatistics>, std::vector<KfcTaskRecord>,
                                  std::vector<KfcOpRecord>, std::vector<KfcOpStatistics>);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<FreqPersistence>(TOPO_NODE(DEVICE_PERSISTENCE, "FreqPersistence"), true,
                                                                TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "FreqParser")),
                                                                {CHIP_V1_1_1, CHIP_V6_1_0, CHIP_V6_1_1}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "FreqPersistence"), std::vector<HalFreqLpmData>);

REGISTER_TOPO_NODE_DEFINITION(
    DeviceDefinition<LpmInfoPersistence>(TOPO_NODE(DEVICE_PERSISTENCE, "LpmInfoPersistence"), true,
                                         TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "LpmInfoParser")), {CHIP_V4_1_0}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "LpmInfoPersistence"), HalLpmInfoData);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<LlcPersistence>(TOPO_NODE(DEVICE_PERSISTENCE, "LlcPersistence"), true,
                                                               TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "LlcParser")),
                                                               {CHIP_V4_1_0, CHIP_V6_1_0}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "LlcPersistence"), std::vector<HalLlcData>);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<PciePersistence>(TOPO_NODE(DEVICE_PERSISTENCE, "PciePersistence"), true,
                                                                TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "PcieParser")),
                                                                {CHIP_V4_1_0, CHIP_V6_1_0}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "PciePersistence"), std::vector<HalPcieData>);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<MetricSummaryPersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "MetricSummaryPersistence"), false,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "PmuAssociation")), {CHIP_ID_ALL}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "MetricSummaryPersistence"),
                                  std::map<TaskId, std::vector<Domain::DeviceTask>>, std::vector<HalPmuData>,
                                  HostStreamInfo);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<NpuMemPersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "NpuMemPersistence"), true, TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "NpuMemParser")),
    {CHIP_V4_1_0, CHIP_V6_1_0}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "NpuMemPersistence"), std::vector<HalNpuMemData>);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<NpuModuleMemPersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "NpuModuleMemPersistence"), true,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "NpuModuleMemParser")), {CHIP_V4_1_0, CHIP_V6_1_0}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "NpuModuleMemPersistence"),
                                  std::vector<HalNpuModuleMemData>);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<QosPersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "QosPersistence"), false, TOPO_DEPS(), {CHIP_V4_1_0, CHIP_V6_1_0},
    ChipDependencies({"QosParser", "StarsSocProfileParser"})));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "QosPersistence"), std::vector<HalQosBwData>,
                                  HalSocProfileData);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<LowPowerPersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "LowPowerPersistence"), true,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "StarsSocProfileParser")), {CHIP_V6_1_0}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "LowPowerPersistence"), HalSocProfileData);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<ChipTransPersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "ChipTransPersistence"), false,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "StarsSocProfileParser")), {CHIP_V4_1_0, CHIP_V1_1_1, CHIP_V6_1_0}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "ChipTransPersistence"), HalSocProfileData);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<InterSocPersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "InterSocPersistence"), false,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "StarsSocProfileParser")), {CHIP_V4_1_0, CHIP_V6_1_0}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "InterSocPersistence"), HalSocProfileData);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<SioPersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "SioPersistence"), false,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "StarsSocProfileParser")), {CHIP_V4_1_0, CHIP_V1_1_1, CHIP_V6_1_0}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "SioPersistence"), HalSocProfileData);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<StepTracePersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "StepTracePersistence"), true,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "StepTraceProcess"), TOPO_NODE(DEVICE_PROCESSING, "TsTrackParser")),
    {CHIP_ID_ALL}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "StepTracePersistence"),
                                  std::map<uint32_t, std::vector<StepTraceTasks>>, std::vector<HalTrackData>);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<TracePersistence>(
    TOPO_NODE(DEVICE_PERSISTENCE, "TracePersistence"), true,
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "StepTraceProcess"), TOPO_NODE(DEVICE_PROCESSING, "TsTrackParser")),
    {CHIP_ID_ALL}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "TracePersistence"),
                                  std::map<uint32_t, std::vector<StepTraceTasks>>, std::vector<HalTrackData>);

REGISTER_TOPO_NODE_DEFINITION(
    DeviceDefinition<TsTrackPersistence>(TOPO_NODE(DEVICE_PERSISTENCE, "TsTrackPersistence"), true,
                                         TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "TsTrackParser")), {CHIP_ID_ALL}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "TsTrackPersistence"), std::vector<HalTrackData>);

REGISTER_TOPO_NODE_DEFINITION(DeviceDefinition<UbPersistence>(TOPO_NODE(DEVICE_PERSISTENCE, "UbPersistence"), false,
                                                              TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "UbParser")),
                                                              {CHIP_V6_1_0, CHIP_V6_1_1}));
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DEVICE_PERSISTENCE, "UbPersistence"), std::vector<HalUbBwData>);

}  // namespace Domain
}  // namespace Analysis
