# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import io
import os
import tempfile
import unittest

from msparser.ccu_source_calculator import CcuOffsetCalculator
from msparser.ccu_source_calculator import mark_ccu_files_complete


class TestCcuOffsetCalculator(unittest.TestCase):
    def test_prefix_should_span_short_slices(self):
        with tempfile.TemporaryDirectory() as root:
            os.mkdir(os.path.join(root, "data"))
            chunks = [b"xx", b"xxx", b"xx" + bytes(range(64))]
            names = ["ccu0.instr.1.slice_{}".format(i) for i in range(3)]
            for name, chunk in zip(names, chunks):
                with open(os.path.join(root, "data", name), "wb") as file:
                    file.write(chunk)
            calculator = CcuOffsetCalculator(names, 64, root)
            result = b"".join(calculator.pre_process(io.BytesIO(chunk), len(chunk)) for chunk in chunks)
            self.assertEqual(bytes(range(64)), result)
            self.assertEqual(b"", calculator.last_cache)

    def test_marker_and_archive_sizes_should_not_affect_offset(self):
        with tempfile.TemporaryDirectory() as root:
            os.mkdir(os.path.join(root, "data"))
            name = "ccu0.instr.1.slice_0"
            with open(os.path.join(root, "data", name), "wb") as file:
                file.write(bytes(range(64)))
            calculator = CcuOffsetCalculator([name, name + ".zip", name + ".complete"], 64, root)
            self.assertEqual(bytes(range(64)), calculator.pre_process(io.BytesIO(bytes(range(64))), 64))

    def test_partly_completed_source_should_be_rejected(self):
        with tempfile.TemporaryDirectory() as root:
            os.mkdir(os.path.join(root, "data"))
            names = ["ccu0.instr.1.slice_0", "ccu0.instr.1.slice_1"]
            with open(os.path.join(root, "data", names[0] + ".complete"), "wb"):
                pass
            calculator = CcuOffsetCalculator(names, 64, root)
            with self.assertRaisesRegex(ValueError, "Partially completed"):
                calculator.pre_process(io.BytesIO(bytes(64)), 64)

    def test_missing_marker_after_writer_call_should_fail(self):
        with tempfile.TemporaryDirectory() as root:
            os.mkdir(os.path.join(root, "data"))
            name = "ccu0.instr.1.slice_0"
            with open(os.path.join(root, "data", name), "wb"):
                pass

            self.assertFalse(mark_ccu_files_complete(root, [name], lambda *_: None))
