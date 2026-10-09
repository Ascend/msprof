# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2025 Huawei Technologies Co.,Ltd.
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
import unittest
from collections import namedtuple
from dataclasses import dataclass
from common_func.msprof_object import CustomizedNamedtupleFactory


class TestCustomizedNamedtupleFactory(unittest.TestCase):
    @dataclass
    class DtoForTest:
        empty_default: str
        int_attr: int = 0
        str_attr: str = ''

        @property
        def above_zero(self):
            return self.int_attr > 0

    def test_generate_named_tuple_type_from_dto(self):
        tuple_type = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(
            TestCustomizedNamedtupleFactory.DtoForTest, [["sql_unique_attr"]])
        self.assertEqual(tuple_type.__name__, "DtoForTest")
        obj = tuple_type()
        self.assertIsNone(obj.empty_default)
        self.assertFalse(obj.above_zero)
        obj2 = tuple_type("", "", 1, "")
        self.assertTrue(obj2.above_zero)

    def test__enhance_namedtuple_func(self):
        origin_tuple = namedtuple("test_name_tuple", ["a", "b", "c"])
        enhanced_tuple = CustomizedNamedtupleFactory.enhance_namedtuple(origin_tuple, {"kk": 100})
        self.assertTrue(hasattr(enhanced_tuple, "kk"))
        self.assertTrue(hasattr(enhanced_tuple, "replace"))
        obj = enhanced_tuple(1, 2, 3)
        self.assertEqual(obj.kk, 100)
