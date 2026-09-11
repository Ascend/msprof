# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# -------------------------------------------------------------------------

import json
import os
import shutil
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
from common_func.profiling_scene import ExportMode
from msconfig.cpp_pipeline_capability_config import DataPosition
from msconfig.cpp_pipeline_capability_config import PipelineCommand
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag


class StubCollector:
    def __init__(self, facts):
        self.facts = facts

    def collect(self, _request):
        return self.facts


class StubRuntimeProbe:
    def __init__(self, issue=None):
        self.issue = issue

    def probe(self):
        return self.issue


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

    def decide(self, request=None, facts=None, runtime_issue=None):
        return CppPipelineDecider(
            collector=StubCollector(facts or self.facts),
            runtime_probe=StubRuntimeProbe(runtime_issue),
        ).decide(request or self.request)

    @staticmethod
    def reason_codes(result):
        return {issue.reason_code for issue in result.issues}

    def test_supported_timeline_can_run_in_cpp(self):
        result = self.decide()
        self.assertTrue(result.can_run_in_cpp)
        self.assertEqual(result.issues, ())

    def test_command_and_feature_boundaries(self):
        cases = (
            ({"command": PipelineCommand.IMPORT, "command_type": None}, set()),
            ({"command": PipelineCommand.IMPORT, "command_type": None, "is_cluster": True}, {"CLUSTER_UNSUPPORTED"}),
            ({"command_type": "summary", "export_format": "csv"}, set()),
            ({"command_type": "summary", "export_format": "json"}, {"EXPORT_FORMAT_UNSUPPORTED"}),
            ({"clear_mode": True}, {"CLEAR_UNSUPPORTED"}),
            ({"model_id": 0}, {"EXPORT_SELECTION_UNSUPPORTED"}),
        )
        for options, expected in cases:
            with self.subTest(options=options):
                result = self.decide(replace(self.request, **options))
                self.assertEqual(self.reason_codes(result), expected)
                self.assertEqual(result.can_run_in_cpp, not expected)

    def test_timeline_reports_filter_is_supported_but_slicing_is_not(self):
        with tempfile.NamedTemporaryFile() as reports_file:
            request = replace(self.request, reports_path=reports_file.name)
            for slice_enabled in (False, True):
                with self.subTest(slice_enabled=slice_enabled):
                    result = self.decide(request, facts=replace(self.facts, slice_enabled=slice_enabled))
                    expected = {"TIMELINE_SLICING_UNSUPPORTED"} if slice_enabled else set()
                    self.assertEqual(self.reason_codes(result), expected)
                    self.assertEqual(result.can_run_in_cpp, not slice_enabled)

    def test_invalid_reports_path_falls_back(self):
        result = self.decide(replace(self.request, reports_path="/not/exist/reports.json"))
        self.assertFalse(result.can_run_in_cpp)
        self.assertEqual(self.reason_codes(result), {"REPORTS_PATH_INVALID"})

    def test_reports_path_is_ignored_for_commands_without_reports_filter(self):
        cases = (
            (PipelineCommand.IMPORT, None, None),
            (PipelineCommand.EXPORT, "summary", "csv"),
            (PipelineCommand.EXPORT, "db", None),
        )
        for command, command_type, export_format in cases:
            with self.subTest(command=command, command_type=command_type):
                request = replace(
                    self.request,
                    command=command,
                    command_type=command_type,
                    export_format=export_format,
                    reports_path="/not/exist/reports.json",
                )
                result = self.decide(request)
                self.assertTrue(result.can_run_in_cpp)
                self.assertEqual(result.issues, ())

    def test_unsupported_command_does_not_load_runtime(self):
        runtime_probe = mock.Mock(spec=RuntimeProbe)
        request = replace(self.request, command="unknown", command_type=None)
        result = CppPipelineDecider(collector=StubCollector(self.facts), runtime_probe=runtime_probe).decide(request)
        runtime_probe.probe.assert_not_called()
        self.assertFalse(result.can_run_in_cpp)
        self.assertEqual(self.reason_codes(result), {"COMMAND_UNSUPPORTED"})

    def test_unknown_chip_does_not_report_all_export_capability(self):
        facts = replace(
            self.facts,
            result_paths=(replace(self.path_facts, chip_model=None, all_data_export_supported=False),),
        )
        result = self.decide(facts=facts)
        self.assertFalse(result.can_run_in_cpp)
        self.assertEqual(self.reason_codes(result), {"CHIP_UNSUPPORTED"})

    def test_collection_facts_are_accumulated(self):
        facts = replace(
            self.facts,
            result_paths=(
                replace(
                    self.path_facts,
                    collection_version="bad",
                    driver_version=None,
                    all_data_export_supported=False,
                    ai_core_mode="sample-based",
                    custom_pmu_fields=("ai_core_metrics",),
                    raw_tags=frozenset({DataTag.QOS.name}),
                    unknown_raw_files=("future.data",),
                ),
                replace(
                    self.path_facts,
                    path="/collection/device_1",
                    chip_model=ChipModel.CHIP_V6_1_0,
                ),
            ),
        )
        result = self.decide(facts=facts)
        self.assertFalse(result.can_run_in_cpp)
        self.assertTrue(
            {
                "COLLECTION_VERSION_MISMATCH",
                "DRIVER_VERSION_UNSUPPORTED",
                "ALL_DATA_EXPORT_UNSUPPORTED",
                "SAMPLE_BASED_UNSUPPORTED",
                "CUSTOM_PMU_UNSUPPORTED",
                "DATA_TAG_UNSUPPORTED",
                "MIXED_CHIP_MODELS",
                "CHIP_UNSUPPORTED",
            }.issubset(self.reason_codes(result))
        )

    def test_runtime_issue_does_not_skip_collection_checks(self):
        runtime_issue = RuntimeProbe("/not/exist/msprof_analysis.so").probe()
        facts = replace(self.facts, result_paths=(replace(self.path_facts, chip_model=None),))
        result = self.decide(facts=facts, runtime_issue=runtime_issue)
        self.assertEqual(result.issues[0], runtime_issue)
        self.assertIn("CHIP_UNSUPPORTED", self.reason_codes(result))

    def test_exception_falls_back_and_request_is_built_from_path_table(self):
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
        self.assertEqual(
            (request.collection_path, request.host_path, request.device_paths),
            ("/prof", "/prof/host", ("/prof/device_0",)),
        )
        collector = mock.Mock()
        collector.collect.side_effect = RuntimeError("collector failed")
        with self.assertLogs(level="WARNING"):
            result = CppPipelineDecider(collector=collector, runtime_probe=StubRuntimeProbe()).decide(request)
        self.assertEqual(self.reason_codes(result), {"DECISION_EXCEPTION"})


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

    def _request(self, result_path=None, command_type="timeline"):
        return CppPipelineDecisionRequest(
            PipelineCommand.EXPORT,
            command_type,
            self.collection_path,
            device_paths=(result_path or self.result_path,),
        )

    def test_collects_supported_and_independent_raw_tags(self):
        self._write_raw("ffts_profile.data.0.slice_0")
        self._write_raw("aicore.data.0.slice_0")
        facts = self.collector.collect(self._request())
        self.assertEqual(facts.issues, ())
        self.assertEqual(
            facts.result_paths[0].raw_tags,
            frozenset({DataTag.FFTS_PMU.name, DataTag.AI_CORE.name}),
        )

    def test_control_files_do_not_add_raw_tags_or_unknown_files(self):
        self._write_raw("stars_soc.data.0.slice_0")
        for name in (
            "aicore.data.0.slice_0.done",
            "aicore.data.0.slice_0.complete",
            "aicore.data.0.slice_0.zip",
            "unknown.raw.done",
            "unknown.raw.complete",
            "unknown.raw.zip",
        ):
            self._write_raw(name)
        facts = self.collector.collect(self._request())
        self.assertEqual(facts.issues, ())
        self.assertEqual(facts.result_paths[0].raw_tags, frozenset({DataTag.STARS_LOG.name}))
        self.assertEqual(facts.result_paths[0].unknown_raw_files, ())

    def test_stream_sq_info_is_ignored_but_unsupported_tags_still_reject(self):
        self._write_raw("stars_soc.data.0.slice_0")
        self._write_raw("unaging.additional.stream_sq_info.slice_0")
        facts = self.collector.collect(self._request())
        self.assertEqual(facts.result_paths[0].raw_tags, frozenset({DataTag.STARS_LOG.name}))
        decider = CppPipelineDecider(collector=self.collector, runtime_probe=StubRuntimeProbe())
        result = decider.decide(self._request())
        self.assertTrue(result.can_run_in_cpp, result.issues)

        self._write_raw("aicore.data.0.slice_0")
        result = decider.decide(self._request())
        self.assertFalse(result.can_run_in_cpp)
        self.assertEqual([issue.reason_code for issue in result.issues], ["DATA_TAG_UNSUPPORTED"])
        self.assertEqual(result.issues[0].details["tags"], [DataTag.AI_CORE.name])
        self.assertEqual(result.issues[0].details["path"], self.result_path)

    def test_enabled_host_tags_pass_but_disabled_static_op_mem_rejects(self):
        cases = (
            ("compact.expand_stream_spec", DataTag.STREAM_EXPAND),
            ("compact.capture_stream_info", DataTag.CAPTURE_STREAM_INFO),
            ("compact.capture_stream_info_v2", DataTag.CAPTURE_STREAM_INFO),
            ("additional.mc2_comm_info", DataTag.MC2_COMM_INFO),
            ("additional.capture_op_info", DataTag.RUNTIME_OP_INFO),
            ("variable.capture_op_info", DataTag.RUNTIME_OP_INFO),
            ("additional.ccu_task_info", DataTag.CCU_TASK),
            ("additional.ccu_wait_signal_info", DataTag.CCU_WAIT_SIGNAL),
            ("additional.ccu_group_info", DataTag.CCU_GROUP),
        )
        request = replace(self._request(), host_path=self.result_path, device_paths=())
        decider = CppPipelineDecider(collector=self.collector, runtime_probe=StubRuntimeProbe())
        expected_tags = set()
        self._write_raw("unaging.additional.stream_sq_info.slice_0")
        for file_type, tag in cases:
            with self.subTest(file_type=file_type):
                self._write_raw("unaging.%s.slice_0" % file_type)
                expected_tags.add(tag.name)
                facts = self.collector.collect(request)
                self.assertEqual(facts.result_paths[0].raw_tags, frozenset(expected_tags))
                result = decider.decide(request)
                self.assertTrue(result.can_run_in_cpp, result.issues)
        self._write_raw("unaging.additional.static_op_mem.slice_0")
        result = decider.decide(request)
        self.assertFalse(result.can_run_in_cpp)
        self.assertEqual(result.issues[0].details["tags"], [DataTag.STATIC_OP_MEM.name])

    def test_unknown_data_and_invalid_metadata_are_reported(self):
        self._write_raw("unknown.raw")
        self._write_raw("empty.raw", b"")
        os.remove(os.path.join(self.result_path, "sample.json"))
        facts = self.collector.collect(self._request())
        self.assertEqual(facts.result_paths[0].unknown_raw_files, ("unknown.raw",))
        result = CppPipelineDecider(collector=self.collector, runtime_probe=StubRuntimeProbe()).decide(self._request())
        self.assertIn("UNKNOWN_RAW_DATA", {issue.reason_code for issue in result.issues})
        self.assertIn("METADATA_MISSING", {issue.reason_code for issue in facts.issues})
        invalid_facts = self.collector.collect(self._request("/missing"))
        self.assertIn("RESULT_PATH_INVALID", {issue.reason_code for issue in invalid_facts.issues})

    def test_slice_config_is_read_only_for_timeline(self):
        self._write_raw("stars_soc.data.0.slice_0")
        self._write_json(self.slice_config_path, {"slice_switch": "on"})
        self.assertTrue(self.collector.collect(self._request()).slice_enabled)
        with mock.patch.object(self.collector, "_read_slice_enabled", side_effect=AssertionError("unused")):
            self.assertFalse(self.collector.collect(self._request(command_type="summary")).slice_enabled)

    def test_path_read_and_metadata_errors_fall_back_without_decision_exception(self):
        self._write_raw("stars_soc.data.0.slice_0")
        next_path = os.path.join(self.collection_path, "device_1")
        shutil.copytree(self.result_path, next_path)
        with mock.patch.object(
            self.collector,
            "_collect_raw_data",
            side_effect=[
                PermissionError("denied"),
                (frozenset({DataTag.STARS_LOG.name}), ()),
            ],
        ):
            facts = self.collector.collect(replace(self._request(), device_paths=(self.result_path, next_path)))
        self.assertEqual([issue.reason_code for issue in facts.issues], ["RESULT_PATH_READ_FAILED"])
        with mock.patch(
            "common_func.cpp_pipeline_decision.FileOpen",
            side_effect=PermissionError("denied"),
        ):
            result = CppPipelineDecider(collector=self.collector, runtime_probe=StubRuntimeProbe()).decide(
                self._request()
            )
        reason_codes = {issue.reason_code for issue in result.issues}
        self.assertIn("METADATA_INVALID", reason_codes)
        self.assertNotIn("DECISION_EXCEPTION", reason_codes)

    def test_unwritable_directories_are_reported(self):
        self._write_raw("stars_soc.data.0.slice_0")
        with (
            mock.patch("common_func.file_manager.is_root_user", return_value=False),
            mock.patch("common_func.file_manager.os.access", side_effect=lambda _path, mode: mode == os.R_OK),
        ):
            facts = self.collector.collect(self._request())
        self.assertTrue(
            {
                "COLLECTION_PATH_NOT_WRITABLE",
                "RESULT_PATH_NOT_WRITABLE",
                "DATA_DIR_NOT_WRITABLE",
            }
            <= {issue.reason_code for issue in facts.issues}
        )


class TestRuntimeProbe(unittest.TestCase):
    def setUp(self):
        self.cached_module = sys.modules.pop(RuntimeProbe.MODULE_NAME, None)

    def tearDown(self):
        sys.modules.pop(RuntimeProbe.MODULE_NAME, None)
        if self.cached_module is not None:
            sys.modules[RuntimeProbe.MODULE_NAME] = self.cached_module

    def test_missing_or_unloadable_so_is_rejected(self):
        self.assertEqual(
            RuntimeProbe("/not/exist/msprof_analysis.so").probe().reason_code,
            "SO_NOT_FOUND",
        )
        loader = mock.Mock()
        loader.exec_module.side_effect = RuntimeError("initialization failed")
        with (
            mock.patch("common_func.cpp_pipeline_decision.check_so_valid", return_value=True),
            mock.patch(
                "common_func.cpp_pipeline_decision.importlib.util.spec_from_file_location",
                return_value=SimpleNamespace(loader=loader),
            ),
            mock.patch(
                "common_func.cpp_pipeline_decision.importlib.util.module_from_spec",
                return_value=SimpleNamespace(__file__="/tmp/msprof_analysis.so"),
            ),
        ):
            self.assertEqual(
                RuntimeProbe("/tmp/msprof_analysis.so").probe().reason_code,
                "SO_LOAD_FAILED",
            )
        self.assertNotIn(RuntimeProbe.MODULE_NAME, sys.modules)

    def test_loaded_module_is_validated_and_reused(self):
        module = SimpleNamespace(
            __file__="/tmp/msprof_analysis.so",
            parser=SimpleNamespace(run_pipeline=mock.Mock()),
        )
        loader = mock.Mock()
        with (
            mock.patch("common_func.cpp_pipeline_decision.check_so_valid", return_value=True),
            mock.patch(
                "common_func.cpp_pipeline_decision.importlib.util.spec_from_file_location",
                return_value=SimpleNamespace(loader=loader),
            ),
            mock.patch(
                "common_func.cpp_pipeline_decision.importlib.util.module_from_spec",
                return_value=module,
            ),
        ):
            probe = RuntimeProbe("/tmp/msprof_analysis.so")
            self.assertIsNone(probe.probe())
            self.assertIsNone(probe.probe())
        loader.exec_module.assert_called_once_with(module)
        sys.modules[RuntimeProbe.MODULE_NAME] = SimpleNamespace(__file__="/tmp/other.so")
        with mock.patch("common_func.cpp_pipeline_decision.check_so_valid", return_value=True):
            self.assertEqual(
                RuntimeProbe("/tmp/msprof_analysis.so").probe().reason_code,
                "SO_WRONG_MODULE",
            )

    def test_old_or_incomplete_parser_is_rejected(self):
        for parser in (None, SimpleNamespace(), SimpleNamespace(run_pipeline=0)):
            with self.subTest(parser=parser):
                sys.modules[RuntimeProbe.MODULE_NAME] = SimpleNamespace(
                    __file__="/tmp/msprof_analysis.so", parser=parser
                )
                with mock.patch("common_func.cpp_pipeline_decision.check_so_valid", return_value=True):
                    self.assertEqual(
                        RuntimeProbe("/tmp/msprof_analysis.so").probe().reason_code,
                        "PIPELINE_INTERFACE_MISSING",
                    )


if __name__ == "__main__":
    unittest.main()
