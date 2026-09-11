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
from typing import FrozenSet, List, Mapping, Optional, Pattern, Tuple

from common_func.constant import Constant
from common_func.file_manager import FileOpen
from common_func.file_manager import check_dir_readable
from common_func.file_manager import check_dir_can_create_entry
from common_func.file_manager import check_so_valid
from common_func.file_name_manager import FileNameManagerConstant
from common_func.file_name_manager import get_file_name_pattern_match
from common_func.file_name_manager import get_info_json_compiles
from common_func.file_name_manager import get_sample_json_compiles
from common_func.file_slice_helper import FileSliceHelper
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ExportMode
from framework.file_dispatch import FileDispatch
from msconfig.cpp_pipeline_capability_config import CapabilityRegistry
from msconfig.cpp_pipeline_capability_config import CommandCapability
from msconfig.cpp_pipeline_capability_config import DataPosition
from msconfig.cpp_pipeline_capability_config import DEFAULT_CAPABILITY_REGISTRY
from msconfig.cpp_pipeline_capability_config import FEATURE_REASON_CODES
from msconfig.cpp_pipeline_capability_config import PipelineCommand
from msconfig.cpp_pipeline_capability_config import PipelineFeature
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag


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
    ) -> "CppPipelineDecisionRequest":
        return cls(
            command=command,
            command_type=command_type,
            collection_path=str(path_table.get("collection_path", "")),
            host_path=str(path_table.get(StrConstant.HOST_PATH, "") or ""),
            device_paths=tuple(path_table.get(StrConstant.DEVICE_PATH, ()) or ()),
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
    raw_tags: FrozenSet[str]
    unknown_raw_files: Tuple[str, ...]


@dataclass(frozen=True)
class CollectionFacts:
    result_paths: Tuple[ResultPathFacts, ...]
    slice_enabled: bool
    issues: Tuple[DecisionIssue, ...] = ()


class RuntimeProbe:
    MODULE_NAME = "msprof_analysis"
    LOAD_ERRORS = (ImportError, OSError, SystemError, ValueError, TypeError, RuntimeError)

    def __init__(self, so_path: Optional[str] = None):
        analysis_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
        self._so_path = so_path or os.path.join(analysis_dir, "lib64", "msprof_analysis.so")

    def probe(self) -> Optional[DecisionIssue]:
        if not check_so_valid(self._so_path):
            return self._issue("SO_NOT_FOUND", "msprof_analysis.so is missing or invalid.")
        module = sys.modules.get(self.MODULE_NAME)
        try:
            if module is None:
                spec = importlib.util.spec_from_file_location(self.MODULE_NAME, self._so_path)
                if spec is None or spec.loader is None:
                    return self._issue("SO_LOAD_FAILED", "Cannot create an SO module loader.")
                module = importlib.util.module_from_spec(spec)
                sys.modules[self.MODULE_NAME] = module
                try:
                    spec.loader.exec_module(module)
                except self.LOAD_ERRORS:
                    sys.modules.pop(self.MODULE_NAME, None)
                    raise
        except self.LOAD_ERRORS as error:
            logging.warning("Failed to load full C pipeline runtime: %s", error, exc_info=True)
            return self._issue(
                "SO_LOAD_FAILED",
                "msprof_analysis.so cannot be loaded.",
                {"error": str(error)},
            )
        loaded_path = os.path.realpath(getattr(module, "__file__", ""))
        if loaded_path != os.path.realpath(self._so_path):
            return self._issue(
                "SO_WRONG_MODULE",
                "Loaded msprof_analysis module does not match the configured SO.",
                {"loaded_path": loaded_path},
            )
        if not callable(getattr(getattr(module, "parser", None), "run_pipeline", None)):
            return self._issue(
                "PIPELINE_INTERFACE_MISSING",
                "msprof_analysis.parser.run_pipeline is not available.",
            )
        return None

    def _issue(
        self,
        reason_code: str,
        message: str,
        details: Optional[Mapping[str, object]] = None,
    ) -> DecisionIssue:
        issue_details = {"so_path": self._so_path}
        issue_details.update(details or {})
        return DecisionIssue(DecisionDimension.ENVIRONMENT, reason_code, message, issue_details)


class CollectionFactsCollector:
    ANALYSIS_VERSION = InfoConfReader().ANALYSIS_VERSION
    IGNORED_UNTAGGED_PATTERNS = ("stream_sq_info",)
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
        self._slice_config_path = slice_config_path or FileSliceHelper.SLICE_CONFIG_PATH

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
            try:
                facts = self._collect_result_path(path, position, issues)
            except (OSError, ProfException) as error:
                # 单个路径不可读时保留已有原因，并继续检查其他路径。
                logging.warning("Failed to collect full C pipeline facts from %s: %s", path, error)
                issues.append(
                    DecisionIssue(
                        DecisionDimension.ENVIRONMENT,
                        "RESULT_PATH_READ_FAILED",
                        "Cannot read profiling data from the result path.",
                        {"path": path, "position": position, "error": str(error)},
                    )
                )
                continue
            if facts is not None:
                result_facts.append(facts)
        is_export = request.command == PipelineCommand.EXPORT
        return CollectionFacts(
            result_paths=tuple(result_facts),
            slice_enabled=is_export and request.command_type == "timeline" and self._read_slice_enabled(),
            issues=tuple(issues),
        )

    def _collect_result_path(
        self,
        path: str,
        position: str,
        issues: List[DecisionIssue],
    ) -> Optional[ResultPathFacts]:
        if not self._is_readable_dir(path):
            issues.append(
                DecisionIssue(
                    DecisionDimension.ENVIRONMENT,
                    "RESULT_PATH_INVALID",
                    "Result path is missing, unreadable, or not a directory.",
                    {"path": path, "position": position},
                )
            )
            return None
        if not self._is_writable_dir(path):
            issues.append(
                DecisionIssue(
                    DecisionDimension.ENVIRONMENT,
                    "RESULT_PATH_NOT_WRITABLE",
                    "Result path is not writable by the full C pipeline.",
                    {"path": path, "position": position},
                )
            )
        info_json, info_issue = self._read_config(path, get_info_json_compiles(), "info.json")
        sample_config, sample_issue = self._read_config(path, get_sample_json_compiles(), "sample.json")
        issues.extend(item for item in (info_issue, sample_issue) if item is not None)
        data_dir = PathManager.get_data_dir(path)
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
        chip_model = chip_manager.CHIP_RELATION_MAP.get(str(info_json.get(Constant.PLATFORM_VERSION)))
        driver_version = self._to_int(info_json.get("drvVersion"))
        custom_pmu_fields = tuple(
            field_name
            for field_name in (StrConstant.AI_CORE_PROFILING_METRICS, StrConstant.AIV_PROFILING_METRICS)
            if str(sample_config.get(field_name, "")).startswith("Custom")
        )
        return ResultPathFacts(
            path=path,
            position=position,
            chip_model=chip_model,
            collection_version=info_json.get("version"),
            driver_version=driver_version,
            all_data_export_supported=chip_model is not None
            and chip_model not in chip_manager.ALL_DATA_EXPORT_CHIP_BLACKLIST,
            ai_core_mode=sample_config.get(StrConstant.AICORE_PROFILING_MODE),
            aiv_mode=sample_config.get(StrConstant.AIV_PROFILING_MODE),
            custom_pmu_fields=custom_pmu_fields,
            raw_tags=raw_tags,
            unknown_raw_files=unknown_files,
        )

    def _read_config(
        self,
        path: str,
        patterns: Tuple[Pattern[str], ...],
        display_name: str,
    ) -> Tuple[Mapping[str, object], Optional[DecisionIssue]]:
        config_path = path
        try:
            config_path = InfoConfReader().get_conf_file_path(path, patterns)
            if not config_path:
                return {}, DecisionIssue(
                    DecisionDimension.ENVIRONMENT,
                    "METADATA_MISSING",
                    "%s is missing." % display_name,
                    {"path": path, "file": display_name},
                )
            with FileOpen(config_path, "r") as config_file:
                data = json.load(config_file.file_reader)
            if not isinstance(data, dict):
                raise ValueError("JSON root must be an object")
            return data, None
        except (OSError, ValueError, TypeError, ProfException) as error:
            logging.warning("Failed to read full C pipeline metadata %s: %s", config_path, error)
            return {}, DecisionIssue(
                DecisionDimension.ENVIRONMENT,
                "METADATA_INVALID",
                "%s cannot be read as a JSON object." % display_name,
                {"path": config_path, "error": str(error)},
            )

    def _collect_raw_data(self, data_dir: str) -> Tuple[FrozenSet[str], Tuple[str, ...]]:
        raw_tags = set()
        unknown_files = []
        for name in sorted(os.listdir(data_dir)):
            path = os.path.join(data_dir, name)
            if self._is_control_file(name):
                continue
            matched_tags = {
                data_tag.name
                for data_tag, patterns in FileDispatch.FILES_FILTER_MAP.items()
                if get_file_name_pattern_match(name, *patterns)
            }
            matched_tags.update(
                tag for tag, pattern in self.HOST_SYSTEM_PATTERNS.items() if get_file_name_pattern_match(name, pattern)
            )
            # 仅消除同一文件的规则重叠，不能删除其他独立文件贡献的标签。
            if DataTag.FFTS_PMU.name in matched_tags:
                matched_tags.discard(DataTag.AI_CORE.name)
            if DataTag.FREQ.name in matched_tags:
                matched_tags.discard(DataTag.LPM_INFO.name)
            if matched_tags:
                raw_tags.update(matched_tags)
            elif self._is_nonempty(path):
                unknown_files.append(name)
        return frozenset(raw_tags), tuple(unknown_files)

    def _read_slice_enabled(self) -> bool:
        try:
            return FileSliceHelper.read_slice_config(self._slice_config_path)[0] == "on"
        except (OSError, ValueError, TypeError, ProfException):
            # Existing export behavior falls back to slicing off for an invalid config.
            logging.warning("Failed to read timeline slice configuration for full C pipeline decision.")
            return False

    @classmethod
    def _is_control_file(cls, name: str) -> bool:
        if name.endswith((Constant.DONE_TAG, Constant.COMPLETE_TAG, Constant.ZIP_TAG)):
            return True
        return any(pattern.match(name) for pattern in cls.CONTROL_PATTERNS)

    @staticmethod
    def _is_readable_dir(path: str) -> bool:
        try:
            check_dir_readable(path)
        except (OSError, ProfException):
            return False
        return True

    @staticmethod
    def _is_writable_dir(path: str) -> bool:
        try:
            check_dir_can_create_entry(path)
        except (OSError, ProfException):
            return False
        return True

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
        capability = self._registry.command_capabilities.get(key)
        issues = []
        if capability is None:
            issues.append(
                DecisionIssue(
                    DecisionDimension.COMMAND,
                    "COMMAND_UNSUPPORTED",
                    "The full C pipeline does not support this command.",
                    {"command": request.command, "command_type": request.command_type},
                )
            )
        else:
            runtime_issue = self._runtime_probe.probe()
            if runtime_issue is not None:
                issues.append(runtime_issue)
        # 已发现问题仍继续检查，向调用方一次返回所有可判定的回退原因。
        facts = self._collector.collect(request)
        issues.extend(facts.issues)
        if request.is_cluster and (capability is None or not capability.supports_cluster):
            issues.append(
                DecisionIssue(
                    DecisionDimension.SCENE,
                    "CLUSTER_UNSUPPORTED",
                    "The full C pipeline does not support cluster data.",
                )
            )
        if request.command == PipelineCommand.EXPORT and request.export_mode != ExportMode.ALL_EXPORT:
            issues.append(
                DecisionIssue(
                    DecisionDimension.SCENE,
                    "EXPORT_MODE_UNSUPPORTED",
                    "Only all-export mode is supported by the full C pipeline.",
                    {"export_mode": str(request.export_mode)},
                )
            )
        if capability is not None:
            self._check_command_options(request, facts, capability, issues)
        self._check_result_paths(facts, issues)
        return CppPipelineDecisionResult(not issues, tuple(issues), self._registry.version)

    def _check_command_options(
        self,
        request: CppPipelineDecisionRequest,
        facts: CollectionFacts,
        capability: CommandCapability,
        issues: List[DecisionIssue],
    ) -> None:
        if (
            PipelineFeature.REPORTS_FILTER in capability.applicable_features
            and request.reports_path
            and not self._is_readable_file(request.reports_path)
        ):
            issues.append(
                DecisionIssue(
                    DecisionDimension.ENVIRONMENT,
                    "REPORTS_PATH_INVALID",
                    "The reports file is missing or unreadable.",
                    {"path": request.reports_path},
                )
            )
        if request.export_format not in capability.supported_formats:
            issues.append(
                DecisionIssue(
                    DecisionDimension.COMMAND,
                    "EXPORT_FORMAT_UNSUPPORTED",
                    "The requested output format is not supported by the C command capability.",
                    {"format": request.export_format},
                )
            )
        # 新增功能需同步启用条件、原因码及命令适用范围；此处顺序决定原因输出顺序。
        requested_features = {
            PipelineFeature.CLEAR_RAW_DATA: request.clear_mode,
            PipelineFeature.PARTIAL_EXPORT: request.has_export_selection,
            PipelineFeature.REPORTS_FILTER: bool(request.reports_path),
            PipelineFeature.TIMELINE_SLICING: facts.slice_enabled,
        }
        unsupported_features = capability.applicable_features - capability.supported_features
        for feature, enabled in requested_features.items():
            if enabled and feature in unsupported_features:
                issues.append(
                    DecisionIssue(
                        DecisionDimension.COMMAND,
                        FEATURE_REASON_CODES[feature],
                        "A requested Python-flow feature is not supported by the C command capability.",
                        {"feature": feature},
                    )
                )

    def _check_result_paths(
        self,
        facts: CollectionFacts,
        issues: List[DecisionIssue],
    ) -> None:
        chip_models = {path_facts.chip_model for path_facts in facts.result_paths if path_facts.chip_model is not None}
        if len(chip_models) > 1:
            issues.append(
                DecisionIssue(
                    DecisionDimension.CHIP,
                    "MIXED_CHIP_MODELS",
                    "Result paths contain different chip models.",
                    {"chips": sorted(chip.name for chip in chip_models)},
                )
            )
        for path_facts in facts.result_paths:
            self._check_result_path(path_facts, issues)

    def _check_result_path(
        self,
        path_facts: ResultPathFacts,
        issues: List[DecisionIssue],
    ) -> None:
        supported_tags_by_position = self._registry.supported_tags_by_chip.get(path_facts.chip_model)
        if supported_tags_by_position is None:
            issues.append(
                DecisionIssue(
                    DecisionDimension.CHIP,
                    "CHIP_UNSUPPORTED",
                    "The chip has no full C pipeline capability profile.",
                    {
                        "path": path_facts.path,
                        "chip": path_facts.chip_model.name if path_facts.chip_model else None,
                    },
                )
            )
        if path_facts.collection_version != CollectionFactsCollector.ANALYSIS_VERSION:
            issues.append(
                DecisionIssue(
                    DecisionDimension.VERSION,
                    "COLLECTION_VERSION_MISMATCH",
                    "The collection data version does not match this analyzer.",
                    {
                        "path": path_facts.path,
                        "actual": path_facts.collection_version,
                        "expected": CollectionFactsCollector.ANALYSIS_VERSION,
                    },
                )
            )
        if (
            path_facts.driver_version is None
            or path_facts.driver_version < CollectionFactsCollector.ALL_EXPORT_DRIVER_VERSION
        ):
            issues.append(
                DecisionIssue(
                    DecisionDimension.VERSION,
                    "DRIVER_VERSION_UNSUPPORTED",
                    "The driver version does not support all-data export.",
                    {"path": path_facts.path, "driver_version": path_facts.driver_version},
                )
            )
        if path_facts.chip_model is not None and not path_facts.all_data_export_supported:
            issues.append(
                DecisionIssue(
                    DecisionDimension.CHIP,
                    "ALL_DATA_EXPORT_UNSUPPORTED",
                    "The chip does not support all-data export.",
                    {"path": path_facts.path},
                )
            )
        self._check_profiling_scene(path_facts, issues)
        # Files without a FileDispatch tag (for example stream_sq_info) are ignored
        # by the parsing flow and must not reject otherwise supported data.
        unknown_files = [
            name
            for name in path_facts.unknown_raw_files
            if not any(pattern in name for pattern in CollectionFactsCollector.IGNORED_UNTAGGED_PATTERNS)
        ]
        if unknown_files:
            issues.append(
                DecisionIssue(
                    DecisionDimension.DATA,
                    "UNKNOWN_RAW_DATA",
                    "Non-empty raw files cannot be mapped to a known data tag.",
                    {"path": path_facts.path, "files": unknown_files},
                )
            )
        if supported_tags_by_position is None:
            return
        unsupported_tags = sorted(
            path_facts.raw_tags - supported_tags_by_position.get(path_facts.position, frozenset())
        )
        if unsupported_tags:
            issues.append(
                DecisionIssue(
                    DecisionDimension.DATA,
                    "DATA_TAG_UNSUPPORTED",
                    "Some raw data tags do not have an active C parser for this chip and position.",
                    {"path": path_facts.path, "tags": unsupported_tags, "position": path_facts.position},
                )
            )

    @staticmethod
    def _check_profiling_scene(path_facts: ResultPathFacts, issues: List[DecisionIssue]) -> None:
        for engine, mode in (
            ("ai_core", path_facts.ai_core_mode),
            ("aiv", path_facts.aiv_mode),
        ):
            if mode == CollectionFactsCollector.SAMPLE_BASED:
                issues.append(
                    DecisionIssue(
                        DecisionDimension.SCENE,
                        "SAMPLE_BASED_UNSUPPORTED",
                        "Sample-based profiling is not supported by the full C pipeline.",
                        {"path": path_facts.path, "engine": engine},
                    )
                )
            elif mode not in (None, "", CollectionFactsCollector.TASK_BASED):
                issues.append(
                    DecisionIssue(
                        DecisionDimension.SCENE,
                        "PROFILING_MODE_UNSUPPORTED",
                        "The profiling mode is not recognized as task-based.",
                        {"path": path_facts.path, "engine": engine, "mode": mode},
                    )
                )
        if path_facts.custom_pmu_fields:
            issues.append(
                DecisionIssue(
                    DecisionDimension.SCENE,
                    "CUSTOM_PMU_UNSUPPORTED",
                    "Custom PMU metrics are not supported by the full C pipeline.",
                    {"path": path_facts.path, "fields": list(path_facts.custom_pmu_fields)},
                )
            )

    @staticmethod
    def _is_readable_file(path: str) -> bool:
        return os.path.isfile(path) and os.access(path, os.R_OK)


def decide_cpp_pipeline(
    request: CppPipelineDecisionRequest,
) -> CppPipelineDecisionResult:
    """Run the authoritative full C pipeline decision without invoking business methods."""
    return CppPipelineDecider().decide(request)
