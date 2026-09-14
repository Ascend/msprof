# Copyright (c) 2026 Huawei Technologies Co., Ltd.

import os
import unittest
from unittest import mock

from host_prof.host_platform.host_platform_analysis import HostPlatformAnalysis


NAMESPACE = "host_prof.host_platform.host_platform_analysis"


class TestHostPlatformAnalysis(unittest.TestCase):
    def test_ms_run_exports_uncore_and_core_to_separate_databases(self):
        data_dir = os.path.join("prof", "host", "data")
        sqlite_dir = os.path.join("prof", "host", "sqlite")
        with (
            mock.patch(NAMESPACE + ".os.path.exists", return_value=True),
            mock.patch(NAMESPACE + ".PathManager.get_data_dir", return_value=data_dir),
            mock.patch(NAMESPACE + ".get_data_dir_sorted_files",
                       return_value=["host_platform_uncore.bin", "host_platform_core.bin"]),
            mock.patch.object(HostPlatformAnalysis, "create_sqlite_dir", return_value=sqlite_dir),
            mock.patch(NAMESPACE + ".export_platform") as export,
        ):
            HostPlatformAnalysis({"result_dir": "/prof/host"}).ms_run()

        self.assertEqual(
            export.call_args_list,
            [
                mock.call(1, os.path.join(data_dir, "host_platform_uncore.bin"),
                          os.path.join(sqlite_dir, "platform.db")),
                mock.call(2, os.path.join(data_dir, "host_platform_core.bin"),
                          os.path.join(sqlite_dir, "thread.db")),
            ],
        )

    def test_ms_run_exports_core_when_uncore_trace_is_absent(self):
        data_dir = os.path.join("prof", "host", "data")
        sqlite_dir = os.path.join("prof", "host", "sqlite")
        with (
            mock.patch(NAMESPACE + ".os.path.exists", return_value=True),
            mock.patch(NAMESPACE + ".PathManager.get_data_dir", return_value=data_dir),
            mock.patch(NAMESPACE + ".get_data_dir_sorted_files", return_value=["host_platform_core.bin"]),
            mock.patch.object(HostPlatformAnalysis, "create_sqlite_dir", return_value=sqlite_dir),
            mock.patch(NAMESPACE + ".export_platform") as export,
        ):
            HostPlatformAnalysis({"result_dir": "/prof/host"}).ms_run()

        export.assert_called_once_with(2, os.path.join(data_dir, "host_platform_core.bin"),
                                       os.path.join(sqlite_dir, "thread.db"))
