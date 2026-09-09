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

import os
import re
from dataclasses import dataclass
from enum import Enum
from enum import unique
from typing import Any
from typing import Optional
from typing import Tuple

from common_func.file_name_manager import FileNameManagerConstant
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.chip_model import ChipModel


@unique
class CcuDataKind(Enum):
    MISSION = "mission"
    CHANNEL = "channel"


@unique
class CcuMetricKind(Enum):
    DELAY = "delay"
    BANDWIDTH = "bandwidth"
    UNKNOWN = "unknown"


@unique
class CcuMetricUnit(Enum):
    RAW = "raw"
    CYCLE = "cycle"
    NS = "ns"
    US = "us"
    BYTE_PER_SECOND = "byte_per_second"
    MB_PER_SECOND = "mb_per_second"
    UNKNOWN = "unknown"


@dataclass(frozen=True)
class CcuMetricProfile:
    metric_id: str
    data_kind: CcuDataKind
    kind: CcuMetricKind
    unit: CcuMetricUnit


@dataclass(frozen=True)
class CcuFormatProfile:
    version: int
    record_size: int
    decoder_id: str
    is_default: bool = True
    source_pattern: str = ""


@dataclass(frozen=True)
class CcuHardwareProfile:
    profile_id: str
    supported_chip_models: Tuple[ChipModel, ...]
    die_count: int
    channel_count: int
    relative_time_scale: int
    metrics: Tuple[CcuMetricProfile, ...]
    formats: Tuple[Tuple[CcuDataKind, CcuFormatProfile], ...]

    def get_formats(self, data_kind: CcuDataKind) -> Tuple[CcuFormatProfile, ...]:
        return tuple(format_profile for kind, format_profile in self.formats if kind == data_kind)

    def get_format(self, data_kind: CcuDataKind, version: int = None) -> Optional[CcuFormatProfile]:
        format_profiles = self.get_formats(data_kind)
        if version is not None:
            return next((item for item in format_profiles if item.version == version), None)
        return next((item for item in format_profiles if item.is_default), None)

    def get_metric(self, metric_id: str) -> Optional[CcuMetricProfile]:
        return next((metric for metric in self.metrics if metric.metric_id == metric_id), None)

    def supports_metric_kind(self, metric_kind: CcuMetricKind) -> bool:
        return any(metric.kind == metric_kind for metric in self.metrics)

    def get_source_scope(self, data_kind: CcuDataKind, file_name: str, format_version: int = None) -> Tuple[int, int]:
        format_profile = self.get_format(data_kind, format_version)
        if format_profile is None:
            raise ValueError("CCU profile {} does not define {} format".format(self.profile_id, data_kind.value))
        if not format_profile.source_pattern:
            raise ValueError(
                "CCU profile {} does not define {} source pattern".format(self.profile_id, data_kind.value)
            )
        match = re.match(format_profile.source_pattern, os.path.basename(file_name))
        if match is None:
            raise ValueError("CCU source file does not match profile {}: {}".format(self.profile_id, file_name))
        die_id = int(match.group(1))
        source_id = int(match.group(2))
        if die_id < 0 or die_id >= self.die_count:
            raise ValueError("CCU source scope is out of range for profile {}: {}".format(self.profile_id, file_name))
        return die_id, source_id


CCU_V6_1_PROFILE = CcuHardwareProfile(
    profile_id="ccu_v6_1",
    supported_chip_models=(ChipModel.CHIP_V6_1_0,),
    die_count=2,
    channel_count=128,
    relative_time_scale=4,
    metrics=(
        CcuMetricProfile(
            metric_id="channel_delay",
            data_kind=CcuDataKind.CHANNEL,
            kind=CcuMetricKind.DELAY,
            unit=CcuMetricUnit.UNKNOWN,
        ),
    ),
    formats=(
        (
            CcuDataKind.MISSION,
            CcuFormatProfile(
                version=0,
                record_size=StructFmt.CCU_MISSION_FMT_SIZE,
                decoder_id="ccu_v6_1_mission",
                source_pattern=FileNameManagerConstant.CCU_MISSION_PATTERN,
            ),
        ),
        (
            CcuDataKind.CHANNEL,
            CcuFormatProfile(
                version=0,
                record_size=StructFmt.CCU_CHANNEL_FMT_SIZE,
                decoder_id="ccu_v6_1_channel",
                source_pattern=FileNameManagerConstant.CCU_CHANNEL_PATTERN,
            ),
        ),
    ),
)


class CcuHardwareProfileRegistry:
    # CHIP_V6_2_0 and later profiles are registered only after their decoders land here.
    PROFILES = (CCU_V6_1_PROFILE,)

    @classmethod
    def get_profile(cls, chip_model: Any) -> Optional[CcuHardwareProfile]:
        normalized_chip_model = cls._normalize_chip_model(chip_model)
        if normalized_chip_model is None:
            return None
        for profile in cls.PROFILES:
            if normalized_chip_model in profile.supported_chip_models:
                return profile
        return None

    @classmethod
    def require_profile(cls, chip_model: Any) -> CcuHardwareProfile:
        profile = cls.get_profile(chip_model)
        if profile is None:
            raise ValueError("Unsupported CCU chip model: {}".format(chip_model))
        return profile

    @classmethod
    def supported_chip_models_csv(cls) -> str:
        chip_values = {
            str(chip_model.value) for profile in cls.PROFILES for chip_model in profile.supported_chip_models
        }
        return ",".join(sorted(chip_values, key=int))

    @classmethod
    def validate(cls, profiles: Tuple[CcuHardwareProfile, ...] = None) -> None:
        registered_profiles = cls.PROFILES if profiles is None else profiles
        profile_ids = set()
        chip_models = set()
        for profile in registered_profiles:
            if not profile.profile_id or profile.profile_id in profile_ids:
                raise ValueError("Duplicate or empty CCU profile id: {}".format(profile.profile_id))
            profile_ids.add(profile.profile_id)
            if min(profile.die_count, profile.channel_count, profile.relative_time_scale) <= 0:
                raise ValueError("CCU profile {} contains a non-positive capability".format(profile.profile_id))
            if not profile.supported_chip_models:
                raise ValueError("CCU profile {} has no supported chip model".format(profile.profile_id))
            for chip_model in profile.supported_chip_models:
                if chip_model in chip_models:
                    raise ValueError("CCU chip model is registered more than once: {}".format(chip_model))
                chip_models.add(chip_model)
            cls._validate_metrics(profile)
            cls._validate_formats(profile)

    @staticmethod
    def _normalize_chip_model(chip_model: Any) -> Optional[ChipModel]:
        if isinstance(chip_model, ChipModel):
            return chip_model
        try:
            return ChipModel(int(chip_model))
        except (TypeError, ValueError):
            return None

    @staticmethod
    def _validate_metrics(profile: CcuHardwareProfile) -> None:
        metric_ids = set()
        format_data_kinds = {data_kind for data_kind, _ in profile.formats}
        if not profile.metrics:
            raise ValueError("CCU profile {} has no metric capability".format(profile.profile_id))
        for metric in profile.metrics:
            if not metric.metric_id or metric.metric_id in metric_ids:
                raise ValueError("Duplicate or empty CCU metric id: {}".format(metric.metric_id))
            metric_ids.add(metric.metric_id)
            if metric.data_kind not in format_data_kinds:
                raise ValueError("CCU metric {} refers to an unsupported data kind".format(metric.metric_id))

    @staticmethod
    def _validate_formats(profile: CcuHardwareProfile) -> None:
        formats_by_kind = {}
        for data_kind, format_profile in profile.formats:
            formats_by_kind.setdefault(data_kind, []).append(format_profile)
        if not formats_by_kind:
            raise ValueError("CCU profile {} has no data format".format(profile.profile_id))
        for data_kind, format_profiles in formats_by_kind.items():
            versions = set()
            defaults = 0
            for format_profile in format_profiles:
                if format_profile.version in versions:
                    raise ValueError(
                        "Duplicate CCU {} format version {}".format(data_kind.value, format_profile.version)
                    )
                versions.add(format_profile.version)
                defaults += int(format_profile.is_default)
                if format_profile.record_size <= 0 or not format_profile.decoder_id:
                    raise ValueError("Invalid CCU {} format in profile {}".format(data_kind.value, profile.profile_id))
                if format_profile.source_pattern:
                    source_regex = re.compile(format_profile.source_pattern)
                    if source_regex.groups < 2:
                        raise ValueError(
                            "CCU source pattern must capture die and source ids: {}".format(data_kind.value)
                        )
                else:
                    raise ValueError("CCU hardware format must define a source pattern: {}".format(data_kind.value))
            if defaults != 1:
                raise ValueError("CCU {} must have exactly one default format".format(data_kind.value))


CcuHardwareProfileRegistry.validate()
