# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

from dataclasses import dataclass
from typing import FrozenSet, Mapping, Optional, Tuple

from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag


class PipelineCommand:
    EXPORT = "export"
    IMPORT = "import"


class PipelineStage:
    HOST_PARSE = "host_parse"
    DEVICE_PARSE = "device_parse"
    CALCULATE = "calculate"
    DB_EXPORT = "db_export"
    TIMELINE_EXPORT = "timeline_export"
    SUMMARY_EXPORT = "summary_export"


class PipelineDeliverable:
    TIMELINE = "timeline"
    SUMMARY = "summary"
    UNIFIED_DB = "unified_db"
    SQLITE = "sqlite"
    LOG = "log"
    COMPLETE_MARKER = "all_file.complete"
    PERMISSIONS = "permissions"
    TEMP_DIR_CLEANUP = "temporary_directory_cleanup"


class PipelineFeature:
    CLEAR_RAW_DATA = "clear_raw_data"
    PARTIAL_EXPORT = "partial_export"
    REPORTS_FILTER = "reports_filter"
    TIMELINE_SLICING = "timeline_slicing"


class DataPosition:
    HOST = "host"
    DEVICE = "device"


@dataclass(frozen=True)
class ConditionalRequirement:
    source: str
    field: str
    expected: object
    feature: str
    reason_code: str


@dataclass(frozen=True)
class CommandRequirement:
    command: str
    command_type: Optional[str]
    stages: FrozenSet[str]
    deliverables: FrozenSet[str]
    formats: FrozenSet[Optional[str]]
    conditional_requirements: Tuple[ConditionalRequirement, ...] = ()


@dataclass(frozen=True)
class CommandCapability:
    command: str
    command_type: Optional[str]
    stages: FrozenSet[str]
    deliverables: FrozenSet[str]
    formats: FrozenSet[Optional[str]]
    features: FrozenSet[str]
    supports_cluster: bool = False


@dataclass(frozen=True)
class DataCapability:
    tag: str
    positions: FrozenSet[str]


@dataclass(frozen=True)
class ChipCapabilityProfile:
    chip_model: ChipModel
    stages: FrozenSet[str]
    data_capabilities: Tuple[DataCapability, ...]

    def supports_data(self, tag: str, position: str) -> bool:
        return any(item.tag == tag and position in item.positions for item in self.data_capabilities)


@dataclass(frozen=True)
class CapabilityRegistry:
    version: str
    requirements: Mapping[Tuple[str, Optional[str]], CommandRequirement]
    command_capabilities: Mapping[Tuple[str, Optional[str]], CommandCapability]
    chip_profiles: Mapping[ChipModel, ChipCapabilityProfile]


COMMON_PARSE_DELIVERABLES = frozenset(
    {
        PipelineDeliverable.SQLITE,
        PipelineDeliverable.LOG,
        PipelineDeliverable.COMPLETE_MARKER,
        PipelineDeliverable.PERMISSIONS,
    }
)

COMMON_EXPORT_DELIVERABLES = COMMON_PARSE_DELIVERABLES | frozenset({PipelineDeliverable.TEMP_DIR_CLEANUP})

EXPORT_CONDITIONS = (
    ConditionalRequirement(
        source="request",
        field="clear_mode",
        expected=True,
        feature=PipelineFeature.CLEAR_RAW_DATA,
        reason_code="CLEAR_UNSUPPORTED",
    ),
    ConditionalRequirement(
        source="request",
        field="has_export_selection",
        expected=True,
        feature=PipelineFeature.PARTIAL_EXPORT,
        reason_code="EXPORT_SELECTION_UNSUPPORTED",
    ),
)

COMMAND_REQUIREMENTS = {
    (PipelineCommand.EXPORT, "timeline"): CommandRequirement(
        command=PipelineCommand.EXPORT,
        command_type="timeline",
        stages=frozenset(
            {
                PipelineStage.CALCULATE,
                PipelineStage.DB_EXPORT,
                PipelineStage.TIMELINE_EXPORT,
            }
        ),
        deliverables=COMMON_EXPORT_DELIVERABLES
        | frozenset({PipelineDeliverable.TIMELINE, PipelineDeliverable.UNIFIED_DB}),
        formats=frozenset({None}),
        conditional_requirements=EXPORT_CONDITIONS
        + (
            ConditionalRequirement(
                source="request",
                field="reports_path",
                expected="nonempty",
                feature=PipelineFeature.REPORTS_FILTER,
                reason_code="REPORTS_FILTER_UNSUPPORTED",
            ),
            ConditionalRequirement(
                source="facts",
                field="slice_enabled",
                expected=True,
                feature=PipelineFeature.TIMELINE_SLICING,
                reason_code="TIMELINE_SLICING_UNSUPPORTED",
            ),
        ),
    ),
    (PipelineCommand.EXPORT, "summary"): CommandRequirement(
        command=PipelineCommand.EXPORT,
        command_type="summary",
        stages=frozenset(
            {
                PipelineStage.CALCULATE,
                PipelineStage.DB_EXPORT,
                PipelineStage.SUMMARY_EXPORT,
            }
        ),
        deliverables=COMMON_EXPORT_DELIVERABLES
        | frozenset({PipelineDeliverable.SUMMARY, PipelineDeliverable.UNIFIED_DB}),
        formats=frozenset({"csv", "json"}),
        conditional_requirements=EXPORT_CONDITIONS,
    ),
    (PipelineCommand.EXPORT, "db"): CommandRequirement(
        command=PipelineCommand.EXPORT,
        command_type="db",
        stages=frozenset(
            {
                PipelineStage.CALCULATE,
                PipelineStage.DB_EXPORT,
            }
        ),
        deliverables=COMMON_EXPORT_DELIVERABLES | frozenset({PipelineDeliverable.UNIFIED_DB}),
        formats=frozenset({None}),
    ),
    (PipelineCommand.IMPORT, None): CommandRequirement(
        command=PipelineCommand.IMPORT,
        command_type=None,
        stages=frozenset(),
        deliverables=COMMON_PARSE_DELIVERABLES,
        formats=frozenset({None}),
    ),
}

# Side-effect deliverables are part of the unified entry contract.
UNIFIED_EXPORT_DELIVERABLES = COMMON_EXPORT_DELIVERABLES | frozenset({PipelineDeliverable.UNIFIED_DB})

COMMAND_CAPABILITIES = {
    (PipelineCommand.EXPORT, "timeline"): CommandCapability(
        command=PipelineCommand.EXPORT,
        command_type="timeline",
        stages=COMMAND_REQUIREMENTS[(PipelineCommand.EXPORT, "timeline")].stages
        | frozenset({PipelineStage.HOST_PARSE, PipelineStage.DEVICE_PARSE}),
        deliverables=UNIFIED_EXPORT_DELIVERABLES | frozenset({PipelineDeliverable.TIMELINE}),
        formats=frozenset({None}),
        features=frozenset({PipelineFeature.REPORTS_FILTER}),
    ),
    (PipelineCommand.EXPORT, "summary"): CommandCapability(
        command=PipelineCommand.EXPORT,
        command_type="summary",
        stages=COMMAND_REQUIREMENTS[(PipelineCommand.EXPORT, "summary")].stages
        | frozenset({PipelineStage.HOST_PARSE, PipelineStage.DEVICE_PARSE}),
        deliverables=UNIFIED_EXPORT_DELIVERABLES | frozenset({PipelineDeliverable.SUMMARY}),
        formats=frozenset({"csv"}),
        features=frozenset(),
    ),
    (PipelineCommand.EXPORT, "db"): CommandCapability(
        command=PipelineCommand.EXPORT,
        command_type="db",
        stages=COMMAND_REQUIREMENTS[(PipelineCommand.EXPORT, "db")].stages
        | frozenset({PipelineStage.HOST_PARSE, PipelineStage.DEVICE_PARSE}),
        deliverables=UNIFIED_EXPORT_DELIVERABLES,
        formats=frozenset({None}),
        features=frozenset(),
    ),
    (PipelineCommand.IMPORT, None): CommandCapability(
        command=PipelineCommand.IMPORT,
        command_type=None,
        stages=frozenset({PipelineStage.HOST_PARSE, PipelineStage.DEVICE_PARSE}),
        deliverables=COMMON_PARSE_DELIVERABLES,
        formats=frozenset({None}),
        features=frozenset(),
    ),
}

V4_HOST_TAGS = frozenset(
    {
        DataTag.API_EVENT.name,
        DataTag.HASH_DICT.name,
        DataTag.TASK_TRACK.name,
        DataTag.MEMCPY_INFO.name,
        DataTag.HCCL_INFO.name,
        DataTag.TENSOR_ADD_INFO.name,
        DataTag.FUSION_ADD_INFO.name,
        DataTag.GRAPH_ADD_INFO.name,
        DataTag.NODE_BASIC_INFO.name,
        DataTag.NODE_ATTR_INFO.name,
        DataTag.CTX_ID.name,
        DataTag.HCCL_OP_INFO.name,
        DataTag.DPU_TASK_TRACK.name,
    }
)

V4_DEVICE_TAGS = frozenset(
    {
        DataTag.STARS_LOG.name,
        DataTag.FFTS_PMU.name,
        DataTag.TS_TRACK.name,
        DataTag.FREQ.name,
        DataTag.AICPU_ADD_INFO.name,
    }
)

V4_STAGES = frozenset(
    {
        PipelineStage.HOST_PARSE,
        PipelineStage.DEVICE_PARSE,
        PipelineStage.CALCULATE,
        PipelineStage.DB_EXPORT,
        PipelineStage.TIMELINE_EXPORT,
        PipelineStage.SUMMARY_EXPORT,
    }
)

V4_PROFILE = ChipCapabilityProfile(
    chip_model=ChipModel.CHIP_V4_1_0,
    stages=V4_STAGES,
    data_capabilities=tuple(
        [DataCapability(tag, frozenset({DataPosition.HOST})) for tag in sorted(V4_HOST_TAGS)]
        + [DataCapability(tag, frozenset({DataPosition.DEVICE})) for tag in sorted(V4_DEVICE_TAGS)]
    ),
)

# 当前全 C 流程仅放行 V4，其他芯片待对应 SO 能力交付后再补充注册。
DEFAULT_CAPABILITY_REGISTRY = CapabilityRegistry(
    version="1.0",
    requirements=COMMAND_REQUIREMENTS,
    command_capabilities=COMMAND_CAPABILITIES,
    chip_profiles={ChipModel.CHIP_V4_1_0: V4_PROFILE},
)
