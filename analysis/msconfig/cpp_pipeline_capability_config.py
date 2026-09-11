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


class PipelineFeature:
    CLEAR_RAW_DATA = "clear_raw_data"
    PARTIAL_EXPORT = "partial_export"
    REPORTS_FILTER = "reports_filter"
    TIMELINE_SLICING = "timeline_slicing"


class DataPosition:
    HOST = "host"
    DEVICE = "device"


FEATURE_REASON_CODES = {
    PipelineFeature.CLEAR_RAW_DATA: "CLEAR_UNSUPPORTED",
    PipelineFeature.PARTIAL_EXPORT: "EXPORT_SELECTION_UNSUPPORTED",
    PipelineFeature.REPORTS_FILTER: "REPORTS_FILTER_UNSUPPORTED",
    PipelineFeature.TIMELINE_SLICING: "TIMELINE_SLICING_UNSUPPORTED",
}


@dataclass(frozen=True)
class CommandCapability:
    supported_formats: FrozenSet[Optional[str]]
    applicable_features: FrozenSet[str]
    supported_features: FrozenSet[str]
    supports_cluster: bool = False


@dataclass(frozen=True)
class CapabilityRegistry:
    version: str
    command_capabilities: Mapping[Tuple[str, Optional[str]], CommandCapability]
    supported_tags_by_chip: Mapping[ChipModel, Mapping[str, FrozenSet[str]]]


TIMELINE_SUMMARY_FEATURES = frozenset({PipelineFeature.CLEAR_RAW_DATA, PipelineFeature.PARTIAL_EXPORT})

# 仅声明 C 支持的命令选项，参数合法性由 Python 入口校验。
COMMAND_CAPABILITIES = {
    (PipelineCommand.EXPORT, "timeline"): CommandCapability(
        supported_formats=frozenset({None}),
        applicable_features=TIMELINE_SUMMARY_FEATURES
        | frozenset({PipelineFeature.REPORTS_FILTER, PipelineFeature.TIMELINE_SLICING}),
        supported_features=frozenset({PipelineFeature.REPORTS_FILTER}),
    ),
    (PipelineCommand.EXPORT, "summary"): CommandCapability(
        supported_formats=frozenset({"csv"}),
        applicable_features=TIMELINE_SUMMARY_FEATURES,
        supported_features=frozenset(),
    ),
    (PipelineCommand.EXPORT, "db"): CommandCapability(
        supported_formats=frozenset({None}),
        applicable_features=frozenset(),
        supported_features=frozenset(),
    ),
    (PipelineCommand.IMPORT, None): CommandCapability(
        supported_formats=frozenset({None}),
        applicable_features=frozenset(),
        supported_features=frozenset(),
    ),
}

# Match enabled TreeBuildEventTypes/LookupEventTypes and HostTraceWorker dumpers.
# STATIC_OP_MEM remains disabled in both native entry points.
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
        DataTag.RUNTIME_OP_INFO.name,
        DataTag.CAPTURE_STREAM_INFO.name,
        DataTag.MC2_COMM_INFO.name,
        DataTag.STREAM_EXPAND.name,
        DataTag.CCU_TASK.name,
        DataTag.CCU_WAIT_SIGNAL.name,
        DataTag.CCU_GROUP.name,
    }
)

V4_DEVICE_TAGS = frozenset(
    {
        DataTag.STARS_LOG.name,
        DataTag.FFTS_PMU.name,
        DataTag.TS_TRACK.name,
        DataTag.FREQ.name,
        DataTag.AICPU_ADD_INFO.name,
        DataTag.LPM_INFO.name,
    }
)

# 当前全 C 流程仅放行 V4，其他芯片待对应 SO 能力交付后再补充注册。
DEFAULT_CAPABILITY_REGISTRY = CapabilityRegistry(
    version="1.0",
    command_capabilities=COMMAND_CAPABILITIES,
    supported_tags_by_chip={
        ChipModel.CHIP_V4_1_0: {DataPosition.HOST: V4_HOST_TAGS, DataPosition.DEVICE: V4_DEVICE_TAGS},
    },
)
