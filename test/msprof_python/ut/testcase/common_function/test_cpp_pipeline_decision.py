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

import json
import os
import sys
import tempfile
import unittest
from dataclasses import replace
from types import SimpleNamespace
from unittest import mock

from common_func.cpp_pipeline_decision import CollectionFacts
from common_func.cpp_pipeline_decision import CollectionFactsCollector
from common_func.cpp_pipeline_decision import CppPipelineDecider
from common_func.cpp_pipeline_decision import CppPipelineDecisionRequest
from common_func.cpp_pipeline_decision import ResultPathFacts
from common_func.cpp_pipeline_decision import RuntimeProbe
from common_func.cpp_pipeline_decision import RuntimeProbeResult
from common_func.profiling_scene import ExportMode
from msconfig.cpp_pipeline_capability_config import ChipCapabilityProfile
from msconfig.cpp_pipeline_capability_config import CommandCapability
from msconfig.cpp_pipeline_capability_config import DataCapability
from msconfig.cpp_pipeline_capability_config import DataPosition
from msconfig.cpp_pipeline_capability_config import DEFAULT_CAPABILITY_REGISTRY
from msconfig.cpp_pipeline_capability_config import PipelineCommand
from msconfig.cpp_pipeline_capability_config import PipelineStage
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag


class StubCollector:
    def __init__(self, facts):
        self.facts = facts

    def collect(self, request):
        return self.facts


class StubRuntimeProbe:
    def __init__(self, result=RuntimeProbeResult()):
        self.result = result

    def probe(self):
        return self.result


class TestCppPipelineDecider(unittest.TestCase):
    def setUp(self):
        self.path_facts = ResultPathFacts(
            path="/collection/device_0",
            position=DataPosition.DEVICE,
            chip_model=ChipModel.CHIP_V4_1_0,
            collection_version="1.0",
            driver_version=0x072211,
            all_data_export_supported=True,
            ai_core_mode="task-based",
            aiv_mode="task-based",
            custom_pmu_fields=(),
            analyzed=False,
            raw_tags=frozenset({DataTag.STARS_LOG.name}),
            unknown_raw_files=(),
        )
        self.facts = CollectionFacts((self.path_facts,), False)
        self.request = CppPipelineDecisionRequest(
            command=PipelineCommand.EXPORT,
            command_type="timeline",
            collection_path="/collection",
            device_paths=("/collection/device_0",),
        )

    def decide(self, request=None, facts=None, registry=DEFAULT_CAPABILITY_REGISTRY):
        return CppPipelineDecider(
            registry=registry,
            collector=StubCollector(facts or self.facts),
            runtime_probe=StubRuntimeProbe(),
        ).decide(request or self.request)

    @staticmethod
    def reason_codes(result):
        return {issue.reason_code for issue in result.issues}

    def test_supported_v4_timeline_can_run_in_cpp(self):
        result = self.decide()
        self.assertTrue(result.can_run_in_cpp)
        self.assertEqual(result.issues, ())
        self.assertEqual(result.capability_version, "1.0")

    def test_import_is_supported_but_cluster_is_rejected(self):
        request = replace(
            self.request,
            command=PipelineCommand.IMPORT,
            command_type=None,
            is_cluster=True,
        )
        result = self.decide(request)
        self.assertFalse(result.can_run_in_cpp)
        self.assertIn("CLUSTER_UNSUPPORTED", self.reason_codes(result))

        result = self.decide(replace(request, is_cluster=False))
        self.assertTrue(result.can_run_in_cpp)

    def test_non_all_export_and_clear_are_rejected(self):
        request = replace(self.request, export_mode=ExportMode.GRAPH_EXPORT, clear_mode=True)
        result = self.decide(request)
        self.assertIn("EXPORT_MODE_UNSUPPORTED", self.reason_codes(result))
        self.assertIn("CLEAR_UNSUPPORTED", self.reason_codes(result))

    def test_existing_export_outputs_are_rejected(self):
        facts = replace(
            self.facts,
            existing_export_outputs=("/collection/msprof_1.db",),
        )
        result = self.decide(facts=facts)
        self.assertIn("EXPORT_OUTPUT_ALREADY_EXISTS", self.reason_codes(result))

    def test_model_or_iteration_selection_is_rejected_even_if_mode_is_inconsistent(
        self,
    ):
        request = replace(self.request, model_id=0, iteration_id=0)
        self.assertIn(
            "EXPORT_SELECTION_UNSUPPORTED", self.reason_codes(self.decide(request))
        )

    def test_summary_json_is_rejected(self):
        request = replace(self.request, command_type="summary", export_format="json")
        result = self.decide(request)
        self.assertIn("EXPORT_FORMAT_UNSUPPORTED", self.reason_codes(result))

    def test_format_outside_python_contract_is_rejected(self):
        request = replace(self.request, command_type="summary", export_format="yaml")
        result = self.decide(request)
        self.assertIn("EXPORT_FORMAT_INVALID", self.reason_codes(result))

    def test_summary_csv_is_supported(self):
        request = replace(self.request, command_type="summary", export_format="csv")
        self.assertTrue(self.decide(request).can_run_in_cpp)

    def test_timeline_slicing_is_rejected(self):
        result = self.decide(facts=replace(self.facts, slice_enabled=True))
        self.assertIn("TIMELINE_SLICING_UNSUPPORTED", self.reason_codes(result))

    def test_valid_reports_file_is_supported(self):
        with tempfile.NamedTemporaryFile() as reports_file:
            request = replace(self.request, reports_path=reports_file.name)
            self.assertTrue(self.decide(request).can_run_in_cpp)

    def test_invalid_reports_file_is_rejected(self):
        request = replace(self.request, reports_path="/not/exist/reports.json")
        self.assertIn("REPORTS_PATH_INVALID", self.reason_codes(self.decide(request)))

    def test_versions_and_all_export_capability_are_checked(self):
        path_facts = replace(
            self.path_facts,
            collection_version="2.0",
            driver_version=0x1,
            all_data_export_supported=False,
        )
        result = self.decide(facts=CollectionFacts((path_facts,), False))
        self.assertTrue(
            {
                "COLLECTION_VERSION_MISMATCH",
                "DRIVER_VERSION_UNSUPPORTED",
                "ALL_DATA_EXPORT_UNSUPPORTED",
            }.issubset(self.reason_codes(result))
        )

    def test_sample_modes_custom_pmu_and_analyzed_data_are_checked(self):
        path_facts = replace(
            self.path_facts,
            ai_core_mode="sample-based",
            aiv_mode="sample-based",
            custom_pmu_fields=("ai_core_metrics", "aiv_metrics"),
            analyzed=True,
        )
        result = self.decide(facts=CollectionFacts((path_facts,), False))
        codes = [issue.reason_code for issue in result.issues]
        self.assertEqual(codes.count("SAMPLE_BASED_UNSUPPORTED"), 2)
        self.assertIn("CUSTOM_PMU_UNSUPPORTED", codes)
        self.assertIn("DATA_ALREADY_ANALYZED", codes)

    def test_unrecognized_profiling_mode_is_rejected(self):
        path_facts = replace(self.path_facts, aiv_mode="future-mode")
        result = self.decide(facts=CollectionFacts((path_facts,), False))
        self.assertIn("PROFILING_MODE_UNSUPPORTED", self.reason_codes(result))

    def test_unsupported_tag_and_unknown_file_are_checked(self):
        path_facts = replace(
            self.path_facts,
            raw_tags=frozenset({DataTag.QOS.name}),
            unknown_raw_files=("future.data",),
        )
        result = self.decide(facts=CollectionFacts((path_facts,), False))
        self.assertIn("DATA_TAG_UNSUPPORTED", self.reason_codes(result))
        self.assertIn("UNKNOWN_RAW_DATA", self.reason_codes(result))

    def test_mixed_and_unsupported_chips_are_checked(self):
        v6_facts = replace(
            self.path_facts,
            path="/collection/device_1",
            chip_model=ChipModel.CHIP_V6_1_0,
        )
        result = self.decide(facts=CollectionFacts((self.path_facts, v6_facts), False))
        self.assertIn("MIXED_CHIP_MODELS", self.reason_codes(result))
        self.assertIn("CHIP_UNSUPPORTED", self.reason_codes(result))

    def test_all_meaningful_reasons_are_accumulated(self):
        path_facts = replace(
            self.path_facts,
            collection_version="bad",
            driver_version=None,
            ai_core_mode="sample-based",
            analyzed=True,
            raw_tags=frozenset({DataTag.QOS.name}),
            unknown_raw_files=("unknown.raw",),
        )
        request = replace(
            self.request, export_mode=ExportMode.STEP_EXPORT, clear_mode=True, is_cluster=True
        )
        result = self.decide(request, CollectionFacts((path_facts,), True))
        self.assertGreaterEqual(len(result.issues), 9)

    def test_unexpected_exception_falls_back_and_logs_warning(self):
        collector = mock.Mock()
        collector.collect.side_effect = RuntimeError("collector failed")
        with self.assertLogs(level="WARNING"):
            result = CppPipelineDecider(
                collector=collector,
                runtime_probe=StubRuntimeProbe(),
            ).decide(self.request)
        self.assertFalse(result.can_run_in_cpp)
        self.assertEqual(self.reason_codes(result), {"DECISION_EXCEPTION"})

    def test_request_can_be_built_from_path_table(self):
        request = CppPipelineDecisionRequest.from_path_table(
            PipelineCommand.EXPORT,
            "db",
            {
                "collection_path": "/prof",
                "host": "/prof/host",
                "device": ["/prof/device_0"],
            },
            export_mode=ExportMode.ALL_EXPORT,
        )
        self.assertEqual(request.collection_path, "/prof")
        self.assertEqual(request.host_path, "/prof/host")
        self.assertEqual(request.device_paths, ("/prof/device_0",))

    def test_new_chip_and_data_need_only_one_profile_registration(self):
        profile = ChipCapabilityProfile(
            chip_model=ChipModel.CHIP_V6_1_0,
            stages=DEFAULT_CAPABILITY_REGISTRY.chip_profiles[
                ChipModel.CHIP_V4_1_0
            ].stages,
            data_capabilities=(
                DataCapability(DataTag.QOS.name, frozenset({DataPosition.DEVICE})),
            ),
        )
        profiles = dict(DEFAULT_CAPABILITY_REGISTRY.chip_profiles)
        profiles[ChipModel.CHIP_V6_1_0] = profile
        registry = replace(DEFAULT_CAPABILITY_REGISTRY, chip_profiles=profiles)
        facts = CollectionFacts(
            (
                replace(
                    self.path_facts,
                    chip_model=ChipModel.CHIP_V6_1_0,
                    raw_tags=frozenset({DataTag.QOS.name}),
                ),
            ),
            False,
        )
        self.assertTrue(self.decide(facts=facts, registry=registry).can_run_in_cpp)

    def test_future_import_needs_only_one_command_capability_registration(self):
        requirement = DEFAULT_CAPABILITY_REGISTRY.requirements[
            (PipelineCommand.IMPORT, None)
        ]
        capabilities = dict(DEFAULT_CAPABILITY_REGISTRY.command_capabilities)
        capabilities[(PipelineCommand.IMPORT, None)] = CommandCapability(
            command=PipelineCommand.IMPORT,
            command_type=None,
            stages=requirement.stages | frozenset({PipelineStage.DEVICE_PARSE}),
            deliverables=requirement.deliverables,
            formats=requirement.formats,
            features=frozenset(),
        )
        registry = replace(
            DEFAULT_CAPABILITY_REGISTRY, command_capabilities=capabilities
        )
        request = replace(
            self.request, command=PipelineCommand.IMPORT, command_type=None
        )
        self.assertTrue(self.decide(request=request, registry=registry).can_run_in_cpp)

    def test_missing_command_stage_and_deliverable_are_rejected_generically(self):
        key = (PipelineCommand.EXPORT, "timeline")
        old_capability = DEFAULT_CAPABILITY_REGISTRY.command_capabilities[key]
        capabilities = dict(DEFAULT_CAPABILITY_REGISTRY.command_capabilities)
        capabilities[key] = replace(
            old_capability, stages=frozenset(), deliverables=frozenset()
        )
        registry = replace(
            DEFAULT_CAPABILITY_REGISTRY, command_capabilities=capabilities
        )
        result = self.decide(registry=registry)
        self.assertIn("STAGE_UNSUPPORTED", self.reason_codes(result))
        self.assertIn("DELIVERABLE_UNSUPPORTED", self.reason_codes(result))


class TestCollectionFactsCollector(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.collection_path = self.temp_dir.name
        self.result_path = os.path.join(self.collection_path, "device_0")
        self.data_path = os.path.join(self.result_path, "data")
        os.makedirs(self.data_path)
        self._write_json(
            os.path.join(self.result_path, "info.json"),
            {"platform_version": "5", "version": "1.0", "drvVersion": 0x072211},
        )
        self._write_json(
            os.path.join(self.result_path, "sample.json"),
            {
                "ai_core_profiling_mode": "task-based",
                "aiv_profiling_mode": "task-based",
                "ai_core_metrics": "PipeUtilization",
                "aiv_metrics": "PipeUtilization",
            },
        )
        self.slice_config_path = os.path.join(self.collection_path, "slice.json")
        self._write_json(self.slice_config_path, {"slice_switch": "off"})
        self.collector = CollectionFactsCollector(self.slice_config_path)

    def tearDown(self):
        self.temp_dir.cleanup()

    @staticmethod
    def _write_json(path, data):
        with open(path, "w", encoding="utf-8") as output_file:
            json.dump(data, output_file)

    def _write_raw(self, name, content=b"raw"):
        with open(os.path.join(self.data_path, name), "wb") as output_file:
            output_file.write(content)

    def _request(self, result_path=None):
        return CppPipelineDecisionRequest(
            command=PipelineCommand.EXPORT,
            command_type="timeline",
            collection_path=self.collection_path,
            device_paths=(result_path or self.result_path,),
        )

    def test_collects_supported_tag_and_metadata_without_writes(self):
        self._write_raw("stars_soc.data.0.slice_0")
        before = sorted(os.listdir(self.data_path))
        facts = self.collector.collect(self._request())
        self.assertEqual(facts.issues, ())
        self.assertEqual(
            facts.result_paths[0].raw_tags, frozenset({DataTag.STARS_LOG.name})
        )
        self.assertEqual(sorted(os.listdir(self.data_path)), before)

    def test_overlapping_patterns_are_normalized(self):
        self._write_raw("ffts_profile.data.0.slice_0")
        self._write_raw("lpmFreqConv.data.0.slice_0")
        facts = self.collector.collect(self._request())
        self.assertEqual(
            facts.result_paths[0].raw_tags,
            frozenset({DataTag.FFTS_PMU.name, DataTag.FREQ.name}),
        )

    def test_genuine_pattern_overlap_is_retained(self):
        self._write_raw("ai_ctrl_cpu.data.0.slice_0")
        facts = self.collector.collect(self._request())
        self.assertEqual(
            facts.result_paths[0].raw_tags,
            frozenset({DataTag.AICPU.name, DataTag.CTRLCPU.name}),
        )

    def test_host_system_and_inactive_host_tags_are_collected_for_rejection(self):
        self._write_raw("host_cpu.data.slice_0")
        self._write_raw("aging.additional.Multi_Thread.slice_0")
        facts = self.collector.collect(self._request())
        self.assertEqual(
            facts.result_paths[0].raw_tags,
            frozenset({"HOST_CPU_USAGE", DataTag.MULTI_THREAD.name}),
        )

    def test_unknown_nonempty_file_is_collected_but_empty_file_is_ignored(self):
        self._write_raw("unknown.raw")
        self._write_raw("empty.raw", b"")
        facts = self.collector.collect(self._request())
        self.assertEqual(facts.result_paths[0].unknown_raw_files, ("unknown.raw",))

    def test_analyzed_state_matches_complete_marker_and_nonempty_sqlite(self):
        self._write_raw("stars_soc.data.0.slice_0")
        self._write_raw("all_file.complete", b"")
        sqlite_path = os.path.join(self.result_path, "sqlite")
        os.makedirs(sqlite_path)
        with open(os.path.join(sqlite_path, "data.db"), "wb") as db_file:
            db_file.write(b"db")
        facts = self.collector.collect(self._request())
        self.assertTrue(facts.result_paths[0].analyzed)

    def test_partial_sqlite_without_complete_marker_is_rejected_as_analyzed(self):
        self._write_raw("stars_soc.data.0.slice_0")
        sqlite_path = os.path.join(self.result_path, "sqlite")
        os.makedirs(sqlite_path)
        with open(os.path.join(sqlite_path, "partial.db"), "wb") as db_file:
            db_file.write(b"partial")
        decider = CppPipelineDecider(
            collector=self.collector,
            runtime_probe=StubRuntimeProbe(),
        )
        result = decider.decide(self._request())
        self.assertIn("DATA_ALREADY_ANALYZED", {issue.reason_code for issue in result.issues})

    def test_existing_export_outputs_are_collected(self):
        self._write_raw("stars_soc.data.0.slice_0")
        output_path = os.path.join(self.collection_path, "mindstudio_profiler_output")
        os.makedirs(output_path)
        with open(os.path.join(output_path, "msprof.json"), "wb") as output_file:
            output_file.write(b"output")
        with open(os.path.join(self.collection_path, "msprof_1.db"), "wb") as db_file:
            db_file.write(b"db")
        facts = self.collector.collect(self._request())
        self.assertEqual(
            facts.existing_export_outputs,
            (
                output_path,
                os.path.join(self.collection_path, "msprof_1.db"),
            ),
        )

    def test_invalid_paths_missing_metadata_and_no_raw_data_are_reported(self):
        invalid_result = os.path.join(self.collection_path, "missing")
        invalid_facts = self.collector.collect(self._request(invalid_result))
        self.assertIn(
            "RESULT_PATH_INVALID", {issue.reason_code for issue in invalid_facts.issues}
        )
        os.remove(os.path.join(self.result_path, "sample.json"))
        facts = self.collector.collect(self._request())
        codes = {issue.reason_code for issue in facts.issues}
        self.assertIn("METADATA_MISSING", codes)
        self.assertIn("RAW_DATA_MISSING", codes)

    def test_non_writable_paths_are_reported_without_writes(self):
        self._write_raw("stars_soc.data.0.slice_0")
        with mock.patch(
            "common_func.cpp_pipeline_decision.os.access",
            side_effect=lambda _path, mode: mode == os.R_OK,
        ):
            facts = self.collector.collect(self._request())
        codes = {issue.reason_code for issue in facts.issues}
        self.assertIn("COLLECTION_PATH_NOT_WRITABLE", codes)
        self.assertIn("RESULT_PATH_NOT_WRITABLE", codes)
        self.assertIn("DATA_DIR_NOT_WRITABLE", codes)

    def test_both_custom_pmu_fields_and_slice_switch_are_collected(self):
        self._write_raw("stars_soc.data.0.slice_0")
        self._write_json(
            os.path.join(self.result_path, "sample.json"),
            {"ai_core_metrics": "Custom:0x1", "aiv_metrics": "Custom:0x2"},
        )
        self._write_json(self.slice_config_path, {"slice_switch": "on"})
        facts = self.collector.collect(self._request())
        self.assertTrue(facts.slice_enabled)
        self.assertEqual(
            facts.result_paths[0].custom_pmu_fields, ("ai_core_metrics", "aiv_metrics")
        )


class TestRuntimeProbe(unittest.TestCase):
    def setUp(self):
        self._cached_module = sys.modules.pop(RuntimeProbe.MODULE_NAME, None)

    def tearDown(self):
        sys.modules.pop(RuntimeProbe.MODULE_NAME, None)
        if self._cached_module is not None:
            sys.modules[RuntimeProbe.MODULE_NAME] = self._cached_module

    def test_missing_so_is_rejected(self):
        result = RuntimeProbe("/not/exist/msprof_analysis.so").probe()
        self.assertEqual(result.issues[0].reason_code, "SO_NOT_FOUND")

    def test_so_load_failure_is_rejected(self):
        probe = RuntimeProbe("/tmp/msprof_analysis.so")
        with (
            mock.patch(
                "common_func.cpp_pipeline_decision.os.path.isfile", return_value=True
            ),
            mock.patch(
                "common_func.cpp_pipeline_decision.os.access", return_value=True
            ),
            mock.patch(
                "common_func.cpp_pipeline_decision.importlib.util.spec_from_file_location",
                side_effect=ImportError("dependency missing"),
            ),
        ):
            result = probe.probe()
        self.assertEqual(result.issues[0].reason_code, "SO_LOAD_FAILED")

    def test_wrong_module_is_rejected(self):
        module = SimpleNamespace(__file__="/tmp/other.so", parser=SimpleNamespace())
        result = self._probe_module(module)
        self.assertEqual(result.issues[0].reason_code, "SO_WRONG_MODULE")

    def test_module_without_parser_methods_is_accepted(self):
        module = SimpleNamespace(__file__="/tmp/msprof_analysis.so")
        result = self._probe_module(module)
        self.assertEqual(result.issues, ())

    def test_valid_module_is_accepted_without_parser_invocation(self):
        business_method = mock.Mock()
        module = SimpleNamespace(
            __file__="/tmp/msprof_analysis.so",
            parser=SimpleNamespace(run_cpp_pipeline=business_method),
        )
        result = self._probe_module(module)
        self.assertEqual(result.issues, ())
        business_method.assert_not_called()

    def test_loaded_module_is_reused_without_reexecuting_so(self):
        module = SimpleNamespace(
            __file__="/tmp/msprof_analysis.so",
            parser=SimpleNamespace(run_cpp_pipeline=mock.Mock()),
        )
        loader = mock.Mock()
        spec = SimpleNamespace(loader=loader)
        with (
            mock.patch(
                "common_func.cpp_pipeline_decision.os.path.isfile", return_value=True
            ),
            mock.patch(
                "common_func.cpp_pipeline_decision.os.access", return_value=True
            ),
            mock.patch(
                "common_func.cpp_pipeline_decision.importlib.util.spec_from_file_location",
                return_value=spec,
            ),
            mock.patch(
                "common_func.cpp_pipeline_decision.importlib.util.module_from_spec",
                return_value=module,
            ),
        ):
            probe = RuntimeProbe("/tmp/msprof_analysis.so")
            self.assertEqual(probe.probe().issues, ())
            self.assertEqual(probe.probe().issues, ())
        loader.exec_module.assert_called_once_with(module)

    @staticmethod
    def _probe_module(module):
        loader = mock.Mock()
        spec = SimpleNamespace(loader=loader)
        with (
            mock.patch(
                "common_func.cpp_pipeline_decision.os.path.isfile", return_value=True
            ),
            mock.patch(
                "common_func.cpp_pipeline_decision.os.access", return_value=True
            ),
            mock.patch(
                "common_func.cpp_pipeline_decision.importlib.util.spec_from_file_location",
                return_value=spec,
            ),
            mock.patch(
                "common_func.cpp_pipeline_decision.importlib.util.module_from_spec",
                return_value=module,
            ),
        ):
            return RuntimeProbe("/tmp/msprof_analysis.so").probe()


if __name__ == "__main__":
    unittest.main()
