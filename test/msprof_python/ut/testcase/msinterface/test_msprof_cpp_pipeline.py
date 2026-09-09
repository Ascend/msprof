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

import logging
import os
import sys
import tempfile
import unittest
from argparse import Namespace
from dataclasses import replace
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

from common_func.cpp_pipeline_decision import CppPipelineDecisionRequest
from common_func.cpp_pipeline_decision import CppPipelineDecisionResult
from common_func.cpp_pipeline_decision import DecisionIssue
from common_func.cpp_pipeline_decision import RuntimeProbe
from common_func.msprof_exception import ProfException
from common_func.profiling_scene import ExportMode
from common_func.profiling_scene import ProfilingScene
from msinterface import msprof_c_interface
from msinterface.msprof_cpp_pipeline import CppPipelineRunner
from msinterface.msprof_cpp_pipeline import try_full_cpp_pipeline
from msinterface.msprof_export import ExportCommand
from msinterface.msprof_import import ImportCommand


NAMESPACE = "msinterface.msprof_cpp_pipeline"


class TestCppPipelineRunner(unittest.TestCase):
    def setUp(self):
        prepare_log = mock.patch(NAMESPACE + ".prepare_log")
        prepare_log.start()
        self.addCleanup(prepare_log.stop)
        self.request = CppPipelineDecisionRequest(
            "export", "timeline", "/PROF_0", "/PROF_0/host", ("/PROF_0/device_0", "/PROF_0/device_1")
        )

    def test_native_flags_and_paths_match_pr474(self):
        cases = (
            (self.request, 0x0B),
            (replace(self.request, reports_path="/reports.json"), 0x0B),
            (replace(self.request, command_type="db"), 0x07),
            (replace(self.request, command_type="summary", export_format="csv"), 0x13),
            (replace(self.request, command="import", command_type=None), 0x03),
            (replace(self.request, host_path=""), 0x0A),
            (replace(self.request, device_paths=()), 0x09),
        )
        for request, expected_flags in cases:
            with self.subTest(request=request):
                with mock.patch(NAMESPACE + ".msprof_c_interface.run_pipeline", return_value=0) as native:
                    CppPipelineRunner()._run(request)
                native.assert_called_once_with(
                    "/PROF_0",
                    expected_flags,
                    request.host_path or None,
                    request.device_paths[0] if request.device_paths else None,
                    request.reports_path or None,
                )

    def test_native_failure_and_exception_are_propagated(self):
        for status in (1, 101, None):
            with self.subTest(status=status):
                with mock.patch(NAMESPACE + ".msprof_c_interface.run_pipeline", return_value=status):
                    with self.assertRaises(ProfException):
                        CppPipelineRunner()._run(self.request)
        with (
            mock.patch(NAMESPACE + ".msprof_c_interface.run_pipeline", side_effect=ImportError("load failed")),
            self.assertRaises(ImportError),
        ):
            CppPipelineRunner()._run(self.request)

    def test_pr474_interface_reuses_module_loaded_during_decision(self):
        native = mock.Mock(return_value=0)
        so_path = os.path.join(os.path.realpath(msprof_c_interface.SO_DIR), "msprof_analysis.so")
        module = SimpleNamespace(__file__=so_path, parser=SimpleNamespace(run_pipeline=native))
        loader = mock.Mock()
        with (
            mock.patch.dict(sys.modules),
            mock.patch.object(sys, "path", list(sys.path)),
            mock.patch("common_func.cpp_pipeline_decision.check_so_valid", return_value=True),
            mock.patch.object(msprof_c_interface, "check_so_valid", return_value=True),
            mock.patch(
                "common_func.cpp_pipeline_decision.importlib.util.spec_from_file_location",
                return_value=SimpleNamespace(loader=loader),
            ) as create_spec,
            mock.patch("common_func.cpp_pipeline_decision.importlib.util.module_from_spec", return_value=module),
        ):
            sys.modules.pop("msprof_analysis", None)
            self.assertIsNone(RuntimeProbe().probe())
            CppPipelineRunner()._run(self.request)
            self.assertIs(sys.modules["msprof_analysis"], module)
            create_spec.assert_called_once()
            loader.exec_module.assert_called_once_with(module)
        native.assert_called_once_with("/PROF_0", 0x0B, "/PROF_0/host", "/PROF_0/device_0", None)

    def test_process_receives_full_request_and_exit_status_is_checked(self):
        for exitcode in (0, 1, -11):
            with self.subTest(exitcode=exitcode):
                with mock.patch(NAMESPACE + ".multiprocessing.Process") as process_type:
                    process = process_type.return_value
                    process.exitcode = exitcode
                    runner = CppPipelineRunner()
                    if exitcode:
                        with self.assertRaises(ProfException):
                            runner.run(self.request)
                    else:
                        runner.run(self.request)
                    process_type.assert_called_once_with(target=runner._run, args=(self.request,))
                    process.start.assert_called_once_with()
                    process.join.assert_called_once_with()

    def test_only_preflight_rejection_returns_false(self):
        for allowed in (False, True):
            with self.subTest(allowed=allowed):
                decision = CppPipelineDecisionResult(allowed, (), "1.0")
                with (
                    mock.patch(NAMESPACE + ".decide_cpp_pipeline", return_value=decision),
                    mock.patch(NAMESPACE + ".CppPipelineRunner.run") as run,
                ):
                    self.assertEqual(try_full_cpp_pipeline(self.request), allowed)
                    if allowed:
                        self.assertIs(run.call_args.args[0], self.request)
                    else:
                        run.assert_not_called()
        with (
            mock.patch(NAMESPACE + ".decide_cpp_pipeline", return_value=CppPipelineDecisionResult(True, (), "1.0")),
            mock.patch(NAMESPACE + ".CppPipelineRunner.run", side_effect=ProfException(8)),
            self.assertRaises(ProfException),
        ):
            try_full_cpp_pipeline(self.request)


class TestPipelineLogging(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_dir.cleanup)
        self.root_logger = logging.getLogger()
        # Isolate file handlers from pytest capture and restore them after each test.
        handlers = mock.patch.object(self.root_logger, "handlers", [])
        handlers.start()
        self.addCleanup(handlers.stop)
        self.addCleanup(self.root_logger.setLevel, self.root_logger.level)
        self.addCleanup(self._close_handlers)

    def _close_handlers(self):
        for handler in self.root_logger.handlers[:]:
            self.root_logger.removeHandler(handler)
            handler.close()

    def test_decision_and_outcome_append_to_existing_result_log(self):
        for has_host, outcome in ((True, "fallback"), (False, "success"), (True, "failure")):
            with self.subTest(has_host=has_host, outcome=outcome):
                collection = Path(self.temp_dir.name) / (str(has_host) + outcome)
                host = collection / "host"
                device = collection / "device_2"
                device.mkdir(parents=True)
                if has_host:
                    host.mkdir()
                log_dir = collection / "mindstudio_profiler_log"
                log_dir.mkdir()
                log_file = log_dir / ("collection_host.log" if has_host else "collection_device_2.log")
                log_file.write_text("existing log\n", encoding="utf-8")
                request = CppPipelineDecisionRequest(
                    "import",
                    None,
                    str(collection),
                    str(host) if has_host else "",
                    (str(device), str(collection / "device_3")),
                )
                issue = DecisionIssue("environment", "SO_NOT_FOUND", "Runtime unavailable")

                def decide(_request):
                    logging.warning("Runtime probe diagnostic")
                    return CppPipelineDecisionResult(outcome != "fallback", (issue,), "1.0")

                with (
                    mock.patch(NAMESPACE + ".decide_cpp_pipeline", side_effect=decide),
                    mock.patch(NAMESPACE + ".CppPipelineRunner.run") as run,
                ):
                    if outcome == "failure":
                        error = ProfException(8, "native failed")
                        run.side_effect = error
                        with self.assertRaises(ProfException) as raised:
                            try_full_cpp_pipeline(request)
                        self.assertIs(raised.exception, error)
                    else:
                        self.assertEqual(try_full_cpp_pipeline(request), outcome == "success")
                    self.assertEqual(run.call_count, int(outcome != "fallback"))
                self._close_handlers()
                content = log_file.read_text(encoding="utf-8")
                self.assertTrue(content.startswith("existing log\n"))
                self.assertIn("Runtime probe diagnostic", content)
                self.assertIn(str(collection), content)
                if outcome == "fallback":
                    self.assertIn("SO_NOT_FOUND", content)
                elif outcome == "success":
                    self.assertIn("Full C pipeline completed", content)
                else:
                    self.assertIn("Full C pipeline failed", content)
                    self.assertNotIn("Full C pipeline completed", content)
                self.assertEqual(list(log_dir.iterdir()), [log_file])

    def test_log_directory_is_created_before_decision(self):
        device = Path(self.temp_dir.name) / "PROF_0" / "device_0"
        device.mkdir(parents=True)
        request = CppPipelineDecisionRequest("import", None, str(device.parent), device_paths=(str(device),))
        with mock.patch(NAMESPACE + ".decide_cpp_pipeline", return_value=CppPipelineDecisionResult(False, (), "1.0")):
            self.assertFalse(try_full_cpp_pipeline(request))
        self._close_handlers()
        log_file = device.parent / "mindstudio_profiler_log" / "collection_device_0.log"
        self.assertIn("Full C pipeline is unavailable", log_file.read_text(encoding="utf-8"))


class TestPipelineCommandIntegration(unittest.TestCase):
    def setUp(self):
        self.path_table = {
            "collection_path": "/root/PROF_0",
            "host": "/root/PROF_0/host",
            "device": ["/root/PROF_0/device_0", "/root/PROF_0/device_1"],
        }
        self.old_mode = ProfilingScene().get_mode()
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def tearDown(self):
        ProfilingScene().set_mode(self.old_mode)

    def test_import_routes_success_and_fallback(self):
        command = ImportCommand(Namespace(collection_path="/root", cluster_flag=False))
        for allowed in (True, False):
            with self.subTest(allowed=allowed):
                with (
                    mock.patch("msinterface.msprof_import.try_full_cpp_pipeline", return_value=allowed) as run,
                    mock.patch.object(command, "_start_parse") as parse,
                ):
                    command._process_data(self.path_table)
                    request = run.call_args.args[0]
                    self.assertEqual(request.collection_path, "/root/PROF_0")
                    self.assertEqual(request.command, "import")
                    self.assertEqual(request.device_paths, tuple(self.path_table["device"]))
                    self.assertEqual(parse.call_count, int(not allowed))
        with (
            mock.patch("msinterface.msprof_import.try_full_cpp_pipeline", side_effect=ProfException(8)),
            mock.patch.object(command, "_start_parse") as parse,
            self.assertRaises(ProfException),
        ):
            command._process_data(self.path_table)
        parse.assert_not_called()

    def test_import_discovery_preserves_collection_and_cluster(self):
        command = ImportCommand(Namespace(collection_path="/root", cluster_flag=False))
        with (
            mock.patch(
                "msinterface.msprof_import.DataCheckManager.iter_valid_profiling_sub_paths",
                return_value=[("device_0", "/root/PROF_0/device_0", True)],
            ),
            mock.patch("msinterface.msprof_import.try_full_cpp_pipeline", return_value=True) as run,
        ):
            command._process_sub_dirs("PROF_0", is_cluster=True)
        request = run.call_args.args[0]
        self.assertEqual(request.collection_path, "/root/PROF_0")
        self.assertTrue(request.is_cluster)

    def test_export_preserves_all_request_options(self):
        command = ExportCommand(
            "timeline",
            Namespace(
                collection_path="/root",
                iteration_id=3,
                iteration_count=4,
                model_id=2,
                reports_path="/reports.json",
                clear_mode=True,
                export_format=None,
            ),
        )
        command._cluster_params['is_cluster_scene'] = True
        ProfilingScene().set_mode(ExportMode.STEP_EXPORT)
        with mock.patch("msinterface.msprof_export.try_full_cpp_pipeline", return_value=False) as run:
            self.assertFalse(command._try_full_cpp_pipeline(self.path_table))
        request = run.call_args.args[0]
        self.assertEqual(
            request,
            CppPipelineDecisionRequest(
                "export",
                "timeline",
                "/root/PROF_0",
                "/root/PROF_0/host",
                tuple(self.path_table["device"]),
                is_cluster=True,
                export_mode=ExportMode.STEP_EXPORT,
                reports_path="/reports.json",
                model_id=2,
                iteration_id=3,
                iteration_count=4,
                clear_mode=True,
            ),
        )

    def test_export_default_iteration_does_not_become_selection(self):
        command = ExportCommand("summary", Namespace(collection_path="/root", export_format="csv"))
        with mock.patch("msinterface.msprof_export.try_full_cpp_pipeline", return_value=True) as run:
            self.assertTrue(command._try_full_cpp_pipeline(self.path_table))
        request = run.call_args.args[0]
        self.assertFalse(request.has_export_selection)
        self.assertEqual(request.export_format, "csv")

    def test_export_discovery_routes_success_fallback_and_failure(self):
        for outcome in (True, False, ProfException(8)):
            with self.subTest(outcome=outcome):
                command = ExportCommand("db", Namespace(collection_path="/root/PROF_0"))
                with (
                    mock.patch(
                        "msinterface.msprof_export.DataCheckManager.iter_valid_profiling_sub_paths",
                        return_value=[("host", "/root/PROF_0/host", True)],
                    ),
                    mock.patch.object(command, "_update_cluster_params"),
                    mock.patch("msinterface.msprof_export.try_full_cpp_pipeline") as run,
                    mock.patch("msinterface.msprof_export.run_in_subprocess") as hybrid,
                ):
                    if isinstance(outcome, Exception):
                        run.side_effect = outcome
                        with self.assertRaises(ProfException):
                            command._process_sub_dirs()
                    else:
                        run.return_value = outcome
                        command._process_sub_dirs()
                    self.assertEqual(hybrid.call_count, int(outcome is False))
                    self.assertEqual(command.valid_data_count, 1)
