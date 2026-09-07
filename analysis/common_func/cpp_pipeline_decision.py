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

import importlib.util
import json
import logging
import os
import re
import sys
from dataclasses import dataclass, field
from typing import FrozenSet, Mapping, Optional, Tuple

from common_func.constant import Constant
from common_func.file_manager import FileOpen
from common_func.file_name_manager import FileNameManagerConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ExportMode
from framework.file_dispatch import FileDispatch
from msconfig.cpp_pipeline_capability_config import CapabilityRegistry
from msconfig.cpp_pipeline_capability_config import DataPosition
from msconfig.cpp_pipeline_capability_config import DEFAULT_CAPABILITY_REGISTRY
from msconfig.cpp_pipeline_capability_config import PipelineCommand
from msconfig.cpp_pipeline_capability_config import PipelineStage
from profiling_bean.prof_enum.chip_model import ChipModel


class DecisionDimension:
    ENVIRONMENT = "environment"
    COMMAND = "command"
    CHIP = "chip"
    VERSION = "version"
    SCENE = "scene"
    DATA = "data"
    INTERNAL = "internal"


@dataclass(frozen=True)
class CppPipelineDecisionRequest:
    command: str
    command_type: Optional[str]
    collection_path: str
    host_path: str = ""
    device_paths: Tuple[str, ...] = ()
    is_cluster: bool = False
    export_mode: ExportMode = ExportMode.ALL_EXPORT
    export_format: Optional[str] = None
    reports_path: str = ""
    model_id: Optional[int] = None
    iteration_id: Optional[int] = None
    iteration_count: int = 1
    clear_mode: bool = False

    @property
    def has_export_selection(self) -> bool:
        return self.model_id is not None or self.iteration_id is not None

    @classmethod
    def from_path_table(
        cls,
        command: str,
        command_type: Optional[str],
        path_table: Mapping[str, object],
        **kwargs,
    ):
        return cls(
            command=command,
            command_type=command_type,
            collection_path=str(path_table.get("collection_path", "")),
            host_path=str(path_table.get("host", "") or ""),
            device_paths=tuple(path_table.get("device", ()) or ()),
            **kwargs,
        )


@dataclass(frozen=True)
class DecisionIssue:
    dimension: str
    reason_code: str
    message: str
    details: Mapping[str, object] = field(default_factory=dict)


@dataclass(frozen=True)
class CppPipelineDecisionResult:
    can_run_in_cpp: bool
    issues: Tuple[DecisionIssue, ...]
    capability_version: str


@dataclass(frozen=True)
class ResultPathFacts:
    path: str
    position: str
    chip_model: Optional[ChipModel]
    collection_version: Optional[str]
    driver_version: Optional[int]
    all_data_export_supported: bool
    ai_core_mode: Optional[str]
    aiv_mode: Optional[str]
    custom_pmu_fields: Tuple[str, ...]
    analyzed: bool
    raw_tags: FrozenSet[str]
    unknown_raw_files: Tuple[str, ...]


@dataclass(frozen=True)
class CollectionFacts:
    result_paths: Tuple[ResultPathFacts, ...]
    slice_enabled: bool
    issues: Tuple[DecisionIssue, ...] = ()
    existing_export_outputs: Tuple[str, ...] = ()


@dataclass(frozen=True)
class RuntimeProbeResult:
    issues: Tuple[DecisionIssue, ...] = ()


class RuntimeProbe:
    MODULE_NAME = "msprof_analysis"

    def __init__(self, so_path: Optional[str] = None):
        analysis_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
        self._so_path = so_path or os.path.join(analysis_dir, "lib64", "msprof_analysis.so")

    def probe(self) -> RuntimeProbeResult:
        if not os.path.isfile(self._so_path) or not os.access(self._so_path, os.R_OK):
            return RuntimeProbeResult((self._issue("SO_NOT_FOUND", "msprof_analysis.so is missing or unreadable."),))
        module = sys.modules.get(self.MODULE_NAME)
        try:
            if module is None:
                spec = importlib.util.spec_from_file_location(self.MODULE_NAME, self._so_path)
                if spec is None or spec.loader is None:
                    return RuntimeProbeResult((self._issue("SO_LOAD_FAILED", "Cannot create an SO module loader."),))
                module = importlib.util.module_from_spec(spec)
                sys.modules[self.MODULE_NAME] = module
                try:
                    spec.loader.exec_module(module)
                except (ImportError, OSError, SystemError, ValueError, TypeError, RuntimeError):
                    sys.modules.pop(self.MODULE_NAME, None)
                    raise
        except (
            ImportError,
            OSError,
            SystemError,
            ValueError,
            TypeError,
            RuntimeError,
        ) as error:
            logging.warning("Failed to load full C pipeline runtime: %s", error, exc_info=True)
            return RuntimeProbeResult(
                (
                    self._issue(
                        "SO_LOAD_FAILED",
                        "msprof_analysis.so cannot be loaded.",
                        {"error": str(error)},
                    ),
                )
            )
        loaded_path = os.path.realpath(getattr(module, "__file__", ""))
        if loaded_path != os.path.realpath(self._so_path):
            return RuntimeProbeResult(
                (
                    self._issue(
                        "SO_WRONG_MODULE",
                        "Loaded msprof_analysis module does not match the configured SO.",
                        {"loaded_path": loaded_path},
                    ),
                )
            )
        return RuntimeProbeResult()

    def _issue(
        self,
        reason_code: str,
        message: str,
        details: Optional[Mapping[str, object]] = None,
    ):
        issue_details = {"so_path": self._so_path}
        issue_details.update(details or {})
        return DecisionIssue(DecisionDimension.ENVIRONMENT, reason_code, message, issue_details)


class CollectionFactsCollector:
    ANALYSIS_VERSION = InfoConfReader().ANALYSIS_VERSION
    ALL_EXPORT_DRIVER_VERSION = InfoConfReader().ALL_EXPORT_VERSION
    SAMPLE_BASED = StrConstant.AIC_SAMPLE_BASED_MODE
    TASK_BASED = StrConstant.AIC_TASK_BASED_MODE

    HOST_SYSTEM_PATTERNS = {
        "HOST_CPU_USAGE": re.compile(FileNameManagerConstant.HOST_CPU_USAGE_PATTERN),
        "HOST_MEM_USAGE": re.compile(FileNameManagerConstant.HOST_MEM_USAGE_PATTERN),
        "HOST_DISK_USAGE": re.compile(FileNameManagerConstant.HOST_DISK_USAGE_PATTERN),
        "HOST_NETWORK_USAGE": re.compile(FileNameManagerConstant.HOST_NETWORK_USAGE_PATTERN),
        "HOST_SYSCALL": re.compile(FileNameManagerConstant.HOST_SYS_CALL_PATTERN),
        "HOST_PTHREAD": re.compile(FileNameManagerConstant.HOST_PTHREAD_CALL_PATTERN),
        "HOST_PLATFORM": re.compile(FileNameManagerConstant.HOST_PLATFORM_PATTERN),
    }
    CONTROL_PATTERNS = tuple(
        re.compile(pattern)
        for pattern in (
            FileNameManagerConstant.HOST_START_PATTERN,
            FileNameManagerConstant.DEV_START_PATTERN,
            FileNameManagerConstant.START_INFO_PATTERN,
            FileNameManagerConstant.END_INFO_PATTERN,
        )
    )

    def __init__(self, slice_config_path: Optional[str] = None):
        analysis_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
        self._slice_config_path = slice_config_path or os.path.join(analysis_dir, "msconfig", "msprof_slice.json")

    def collect(self, request: CppPipelineDecisionRequest) -> CollectionFacts:
        issues = []
        if not self._is_readable_dir(request.collection_path):
            issues.append(
                DecisionIssue(
                    DecisionDimension.ENVIRONMENT,
                    "COLLECTION_PATH_INVALID",
                    "Collection path is missing, unreadable, or not a directory.",
                    {"path": request.collection_path},
                )
            )
        elif not self._is_writable_dir(request.collection_path):
            issues.append(
                DecisionIssue(
                    DecisionDimension.ENVIRONMENT,
                    "COLLECTION_PATH_NOT_WRITABLE",
                    "Collection path is not writable by the full C pipeline.",
                    {"path": request.collection_path},
                )
            )
        path_items = []
        if request.host_path:
            path_items.append((request.host_path, DataPosition.HOST))
        path_items.extend((path, DataPosition.DEVICE) for path in request.device_paths)
        if not path_items:
            issues.append(
                DecisionIssue(
                    DecisionDimension.ENVIRONMENT,
                    "RESULT_PATH_MISSING",
                    "No host or device result path was provided.",
                )
            )
        result_facts = []
        for path, position in path_items:
            facts, path_issues = self._collect_result_path(path, position)
            issues.extend(path_issues)
            if facts is not None:
                result_facts.append(facts)
        return CollectionFacts(
            tuple(result_facts),
            self._read_slice_enabled(),
            tuple(issues),
            self._collect_export_outputs(request.collection_path),
        )

    def _collect_result_path(self, path: str, position: str):
        if not self._is_readable_dir(path):
            return None, (
                DecisionIssue(
                    DecisionDimension.ENVIRONMENT,
                    "RESULT_PATH_INVALID",
                    "Result path is missing, unreadable, or not a directory.",
                    {"path": path, "position": position},
                ),
            )
        issues = []
        if not self._is_writable_dir(path):
            issues.append(
                DecisionIssue(
                    DecisionDimension.ENVIRONMENT,
                    "RESULT_PATH_NOT_WRITABLE",
                    "Result path is not writable by the full C pipeline.",
                    {"path": path, "position": position},
                )
            )
        info, info_issue = self._read_config(path, re.compile(r"^info\.json(?:\.\d+)?$"), "info.json")
        sample, sample_issue = self._read_config(path, re.compile(r"^sample\.json$"), "sample.json")
        issues.extend(item for item in (info_issue, sample_issue) if item is not None)
        data_dir = os.path.join(path, "data")
        if not self._is_readable_dir(data_dir):
            issues.append(
                DecisionIssue(
                    DecisionDimension.ENVIRONMENT,
                    "DATA_DIR_INVALID",
                    "Raw data directory is missing or unreadable.",
                    {"path": data_dir},
                )
            )
            raw_tags, unknown_files = frozenset(), ()
        else:
            if not self._is_writable_dir(data_dir):
                issues.append(
                    DecisionIssue(
                        DecisionDimension.ENVIRONMENT,
                        "DATA_DIR_NOT_WRITABLE",
                        "Raw data directory is not writable for the completion marker.",
                        {"path": data_dir},
                    )
                )
            raw_tags, unknown_files = self._collect_raw_data(data_dir)
            if not raw_tags and not unknown_files:
                issues.append(
                    DecisionIssue(
                        DecisionDimension.DATA,
                        "RAW_DATA_MISSING",
                        "No raw profiling data was found.",
                        {"path": data_dir},
                    )
                )
        chip_manager = ChipManager()
        chip_model = chip_manager.CHIP_RELATION_MAP.get(str(info.get(Constant.PLATFORM_VERSION)))
        driver_version = self._to_int(info.get("drvVersion"))
        custom_fields = tuple(
            field_name
            for field_name in ("ai_core_metrics", "aiv_metrics")
            if str(sample.get(field_name, "")).startswith("Custom")
        )
        sqlite_dir = os.path.join(path, "sqlite")
        has_complete_marker = os.path.isfile(os.path.join(data_dir, FileNameManagerConstant.ALL_FILE_TAG))
        has_sqlite_data = bool(os.listdir(sqlite_dir) if os.path.isdir(sqlite_dir) else [])
        facts = ResultPathFacts(
            path=path,
            position=position,
            chip_model=chip_model,
            collection_version=info.get("version"),
            driver_version=driver_version,
            all_data_export_supported=chip_model is not None
            and chip_model not in chip_manager.ALL_DATA_EXPORT_CHIP_BLACKLIST,
            ai_core_mode=sample.get("ai_core_profiling_mode"),
            aiv_mode=sample.get("aiv_profiling_mode"),
            custom_pmu_fields=custom_fields,
            analyzed=has_complete_marker or has_sqlite_data,
            raw_tags=raw_tags,
            unknown_raw_files=unknown_files,
        )
        return facts, tuple(issues)

    def _read_config(self, path: str, pattern, display_name: str):
        matches = sorted(name for name in os.listdir(path) if pattern.match(name))
        if not matches:
            return {}, DecisionIssue(
                DecisionDimension.ENVIRONMENT,
                "METADATA_MISSING",
                "%s is missing." % display_name,
                {"path": path, "file": display_name},
            )
        config_path = os.path.join(path, matches[0])
        try:
            with FileOpen(config_path, "r") as config_file:
                data = json.load(config_file.file_reader)
            if not isinstance(data, dict):
                raise ValueError("JSON root must be an object")
            return data, None
        except (OSError, ValueError, TypeError) as error:
            logging.warning("Failed to read full C pipeline metadata %s: %s", config_path, error)
            return {}, DecisionIssue(
                DecisionDimension.ENVIRONMENT,
                "METADATA_INVALID",
                "%s cannot be read as a JSON object." % display_name,
                {"path": config_path, "error": str(error)},
            )

    def _collect_raw_data(self, data_dir: str):
        raw_tags = set()
        unknown_files = []
        for name in sorted(os.listdir(data_dir)):
            path = os.path.join(data_dir, name)
            if self._is_control_file(name):
                continue
            matched_tags = {
                data_tag.name
                for data_tag, patterns in FileDispatch.FILES_FILTER_MAP.items()
                if any(pattern.match(name) for pattern in patterns)
            }
            matched_tags.update(tag for tag, pattern in self.HOST_SYSTEM_PATTERNS.items() if pattern.match(name))
            if matched_tags:
                raw_tags.update(matched_tags)
            elif self._is_nonempty(path):
                unknown_files.append(name)
        if "FFTS_PMU" in raw_tags:
            raw_tags.discard("AI_CORE")
        if "FREQ" in raw_tags:
            raw_tags.discard("LPM_INFO")
        return frozenset(raw_tags), tuple(unknown_files)

    def _read_slice_enabled(self):
        try:
            with FileOpen(self._slice_config_path, "r") as config_file:
                config = json.load(config_file.file_reader)
            return config.get("slice_switch", "on") == "on"
        except (OSError, ValueError, TypeError):
            # Existing export behavior falls back to slicing off for an invalid config.
            logging.warning("Failed to read timeline slice configuration for full C pipeline decision.")
            return False

    @staticmethod
    def _collect_export_outputs(collection_path: str) -> Tuple[str, ...]:
        if not os.path.isdir(collection_path):
            return ()
        outputs = []
        for name in sorted(os.listdir(collection_path)):
            path = os.path.join(collection_path, name)
            if name.startswith("msprof_") and name.endswith(".db"):
                outputs.append(path)
            elif name == "mindstudio_profiler_output" and os.path.isdir(path):
                if os.listdir(path):
                    outputs.append(path)
        return tuple(outputs)

    @classmethod
    def _is_control_file(cls, name: str) -> bool:
        if name.endswith((Constant.DONE_TAG, Constant.COMPLETE_TAG, Constant.ZIP_TAG)):
            return True
        return any(pattern.match(name) for pattern in cls.CONTROL_PATTERNS)

    @staticmethod
    def _is_readable_dir(path: str) -> bool:
        return bool(path) and os.path.isdir(path) and os.access(path, os.R_OK)

    @staticmethod
    def _is_writable_dir(path: str) -> bool:
        return bool(path) and os.access(path, os.W_OK | os.X_OK)

    @staticmethod
    def _is_nonempty(path: str) -> bool:
        if os.path.isfile(path):
            return os.path.getsize(path) > 0
        if os.path.isdir(path):
            return bool(os.listdir(path))
        return True

    @staticmethod
    def _to_int(value) -> Optional[int]:
        try:
            return int(value, 0) if isinstance(value, str) else int(value)
        except (ValueError, TypeError):
            return None


class CppPipelineDecider:
    def __init__(
        self,
        registry: CapabilityRegistry = DEFAULT_CAPABILITY_REGISTRY,
        collector: Optional[CollectionFactsCollector] = None,
        runtime_probe: Optional[RuntimeProbe] = None,
    ):
        self._registry = registry
        self._collector = collector or CollectionFactsCollector()
        self._runtime_probe = runtime_probe or RuntimeProbe()

    def decide(self, request: CppPipelineDecisionRequest) -> CppPipelineDecisionResult:
        try:
            return self._decide(request)
        except Exception as error:  # pylint: disable=broad-except
            logging.warning(
                "Full C pipeline decision failed and will fall back to the existing flow: %s",
                error,
                exc_info=True,
            )
            issue = DecisionIssue(
                DecisionDimension.INTERNAL,
                "DECISION_EXCEPTION",
                "An unexpected error occurred during the full C pipeline decision.",
                {"error": str(error)},
            )
            return CppPipelineDecisionResult(False, (issue,), self._registry.version)

    def _decide(self, request: CppPipelineDecisionRequest) -> CppPipelineDecisionResult:
        key = (request.command, request.command_type)
        requirement = self._registry.requirements.get(key)
        capability = self._registry.command_capabilities.get(key)
        issues = []
        if requirement is None:
            issues.append(
                DecisionIssue(
                    DecisionDimension.COMMAND,
                    "COMMAND_INVALID",
                    "The command or export command_type is not part of the Python contract.",
                    {"command": request.command, "command_type": request.command_type},
                )
            )
        if capability is None:
            issues.append(
                DecisionIssue(
                    DecisionDimension.COMMAND,
                    "COMMAND_UNSUPPORTED",
                    "The full C pipeline does not support this command.",
                    {"command": request.command, "command_type": request.command_type},
                )
            )
        issues.extend(self._runtime_probe.probe().issues)
        facts = self._collector.collect(request)
        issues.extend(facts.issues)
        if request.command == PipelineCommand.EXPORT and facts.existing_export_outputs:
            issues.append(
                DecisionIssue(
                    DecisionDimension.SCENE,
                    "EXPORT_OUTPUT_ALREADY_EXISTS",
                    "Existing export outputs make full C pipeline rollback ambiguous.",
                    {"paths": list(facts.existing_export_outputs)},
                )
            )
        if request.is_cluster and (capability is None or not capability.supports_cluster):
            issues.append(
                DecisionIssue(
                    DecisionDimension.SCENE,
                    "CLUSTER_UNSUPPORTED",
                    "The full C pipeline does not support cluster data.",
                )
            )
        if request.command == PipelineCommand.EXPORT and not self._is_all_export(request.export_mode):
            issues.append(
                DecisionIssue(
                    DecisionDimension.SCENE,
                    "EXPORT_MODE_UNSUPPORTED",
                    "Only all-export mode is supported by the full C pipeline.",
                    {"export_mode": str(request.export_mode)},
                )
            )
        if request.reports_path and not self._is_readable_file(request.reports_path):
            issues.append(
                DecisionIssue(
                    DecisionDimension.ENVIRONMENT,
                    "REPORTS_PATH_INVALID",
                    "The reports file is missing or unreadable.",
                    {"path": request.reports_path},
                )
            )
        if requirement is not None and capability is not None:
            self._check_command_contract(request, facts, requirement, capability, issues)
        self._check_result_paths(facts, requirement, capability, issues)
        return CppPipelineDecisionResult(not issues, tuple(issues), self._registry.version)

    def _check_command_contract(self, request, facts, requirement, capability, issues):
        positions = set()
        if request.host_path:
            positions.add(DataPosition.HOST)
        if request.device_paths:
            positions.add(DataPosition.DEVICE)
        required_stages = self._get_required_stages(requirement, positions)
        missing_stages = sorted(required_stages - capability.stages)
        if missing_stages:
            issues.append(
                DecisionIssue(
                    DecisionDimension.COMMAND,
                    "STAGE_UNSUPPORTED",
                    "The C command capability is missing required stages.",
                    {"stages": missing_stages},
                )
            )
        missing_deliverables = sorted(requirement.deliverables - capability.deliverables)
        if missing_deliverables:
            issues.append(
                DecisionIssue(
                    DecisionDimension.COMMAND,
                    "DELIVERABLE_UNSUPPORTED",
                    "The C command capability is missing required deliverables.",
                    {"deliverables": missing_deliverables},
                )
            )
        if request.export_format not in requirement.formats:
            issues.append(
                DecisionIssue(
                    DecisionDimension.COMMAND,
                    "EXPORT_FORMAT_INVALID",
                    "The requested output format is not part of the Python command contract.",
                    {"format": request.export_format},
                )
            )
        elif request.export_format not in capability.formats:
            issues.append(
                DecisionIssue(
                    DecisionDimension.COMMAND,
                    "EXPORT_FORMAT_UNSUPPORTED",
                    "The requested output format is not supported by the C command capability.",
                    {"format": request.export_format},
                )
            )
        for conditional in requirement.conditional_requirements:
            source = request if conditional.source == "request" else facts
            value = getattr(source, conditional.field)
            matched = bool(value) if conditional.expected == "nonempty" else value == conditional.expected
            if matched and conditional.feature not in capability.features:
                issues.append(
                    DecisionIssue(
                        DecisionDimension.COMMAND,
                        conditional.reason_code,
                        "A requested Python-flow feature is not supported by the C command capability.",
                        {"feature": conditional.feature},
                    )
                )

    def _check_result_paths(self, facts, requirement, capability, issues):
        chip_models = {item.chip_model for item in facts.result_paths if item.chip_model is not None}
        if len(chip_models) > 1:
            issues.append(
                DecisionIssue(
                    DecisionDimension.CHIP,
                    "MIXED_CHIP_MODELS",
                    "Result paths contain different chip models.",
                    {"chips": sorted(chip.name for chip in chip_models)},
                )
            )
        for item in facts.result_paths:
            profile = self._registry.chip_profiles.get(item.chip_model)
            if profile is None:
                issues.append(
                    DecisionIssue(
                        DecisionDimension.CHIP,
                        "CHIP_UNSUPPORTED",
                        "The chip has no full C pipeline capability profile.",
                        {
                            "path": item.path,
                            "chip": item.chip_model.name if item.chip_model else None,
                        },
                    )
                )
            if item.collection_version != CollectionFactsCollector.ANALYSIS_VERSION:
                issues.append(
                    DecisionIssue(
                        DecisionDimension.VERSION,
                        "COLLECTION_VERSION_MISMATCH",
                        "The collection data version does not match this analyzer.",
                        {
                            "path": item.path,
                            "actual": item.collection_version,
                            "expected": CollectionFactsCollector.ANALYSIS_VERSION,
                        },
                    )
                )
            if item.driver_version is None or item.driver_version < CollectionFactsCollector.ALL_EXPORT_DRIVER_VERSION:
                issues.append(
                    DecisionIssue(
                        DecisionDimension.VERSION,
                        "DRIVER_VERSION_UNSUPPORTED",
                        "The driver version does not support all-data export.",
                        {"path": item.path, "driver_version": item.driver_version},
                    )
                )
            if not item.all_data_export_supported:
                issues.append(
                    DecisionIssue(
                        DecisionDimension.CHIP,
                        "ALL_DATA_EXPORT_UNSUPPORTED",
                        "The chip does not support all-data export.",
                        {"path": item.path},
                    )
                )
            self._check_profiling_scene(item, issues)
            if item.analyzed:
                issues.append(
                    DecisionIssue(
                        DecisionDimension.SCENE,
                        "DATA_ALREADY_ANALYZED",
                        "The result path has already been analyzed.",
                        {"path": item.path},
                    )
                )
            if item.unknown_raw_files:
                issues.append(
                    DecisionIssue(
                        DecisionDimension.DATA,
                        "UNKNOWN_RAW_DATA",
                        "Non-empty raw files cannot be mapped to a known data tag.",
                        {"path": item.path, "files": list(item.unknown_raw_files)},
                    )
                )
            if profile is not None:
                unsupported_tags = sorted(tag for tag in item.raw_tags if not profile.supports_data(tag, item.position))
                if unsupported_tags:
                    issues.append(
                        DecisionIssue(
                            DecisionDimension.DATA,
                            "DATA_TAG_UNSUPPORTED",
                            "Some raw data tags do not have an active C parser for this chip and position.",
                            {
                                "path": item.path,
                                "tags": unsupported_tags,
                                "position": item.position,
                            },
                        )
                    )
                if requirement is not None and capability is not None:
                    missing_profile_stages = sorted(
                        self._get_required_stages(requirement, {item.position}) - profile.stages
                    )
                    if missing_profile_stages:
                        issues.append(
                            DecisionIssue(
                                DecisionDimension.CHIP,
                                "CHIP_STAGE_UNSUPPORTED",
                                "The chip capability profile is missing required stages.",
                                {"path": item.path, "stages": missing_profile_stages},
                            )
                        )

    @staticmethod
    def _check_profiling_scene(item, issues):
        for field_name, mode in (
            ("ai_core", item.ai_core_mode),
            ("aiv", item.aiv_mode),
        ):
            if mode == CollectionFactsCollector.SAMPLE_BASED:
                issues.append(
                    DecisionIssue(
                        DecisionDimension.SCENE,
                        "SAMPLE_BASED_UNSUPPORTED",
                        "Sample-based profiling is not supported by the full C pipeline.",
                        {"path": item.path, "engine": field_name},
                    )
                )
            elif mode not in (None, "", CollectionFactsCollector.TASK_BASED):
                issues.append(
                    DecisionIssue(
                        DecisionDimension.SCENE,
                        "PROFILING_MODE_UNSUPPORTED",
                        "The profiling mode is not recognized as task-based.",
                        {"path": item.path, "engine": field_name, "mode": mode},
                    )
                )
        if item.custom_pmu_fields:
            issues.append(
                DecisionIssue(
                    DecisionDimension.SCENE,
                    "CUSTOM_PMU_UNSUPPORTED",
                    "Custom PMU metrics are not supported by the full C pipeline.",
                    {"path": item.path, "fields": list(item.custom_pmu_fields)},
                )
            )

    @staticmethod
    def _get_required_stages(requirement, positions):
        required_stages = set(requirement.stages)
        if DataPosition.HOST in positions:
            required_stages.add(PipelineStage.HOST_PARSE)
        if DataPosition.DEVICE in positions:
            required_stages.add(PipelineStage.DEVICE_PARSE)
        return frozenset(required_stages)

    @staticmethod
    def _is_all_export(export_mode: ExportMode) -> bool:
        return export_mode == ExportMode.ALL_EXPORT

    @staticmethod
    def _is_readable_file(path: str) -> bool:
        return os.path.isfile(path) and os.access(path, os.R_OK)


def decide_cpp_pipeline(
    request: CppPipelineDecisionRequest,
) -> CppPipelineDecisionResult:
    """Run the authoritative full C pipeline decision without invoking business methods."""
    return CppPipelineDecider().decide(request)
