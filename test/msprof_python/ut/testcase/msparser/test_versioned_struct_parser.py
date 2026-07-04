# pylint: disable=duplicate-code
# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
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
import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from msparser.versioned_struct_parser import VersionedStructParser
from msparser.versioned_struct_parser import VersionedStructSpec


class TestVersionedStructParser(unittest.TestCase):
    def tearDown(self) -> None:
        InfoConfReader()._info_json = {}

    def test_iter_records_should_unpack_current_version_records(self):
        InfoConfReader()._info_json = {"version": "1.0"}
        parser = VersionedStructParser("TEST", {"1.0": VersionedStructSpec("II", 8)})
        data = struct.pack("=IIII", 1, 2, 3, 4)
        self.assertEqual(list(parser.iter_records(data)), [(1, 2), (3, 4)])

    def test_iter_decoded_records_should_apply_decoder(self):
        InfoConfReader()._info_json = {"version": "1.0"}
        parser = VersionedStructParser(
            "TEST", {"1.0": VersionedStructSpec("II", 8, lambda record: record[0] + record[1])}
        )
        data = struct.pack("=IIII", 1, 2, 3, 4)
        self.assertEqual(list(parser.iter_decoded_records(data)), [3, 7])

    def test_should_use_default_spec_when_collection_version_is_missing(self):
        InfoConfReader()._info_json = {}
        with mock.patch("msparser.versioned_struct_parser.logging.warning"):
            parser = VersionedStructParser("TEST", {"1.0": VersionedStructSpec("II", 8)})
        self.assertEqual(parser.spec.fmt, "II")
