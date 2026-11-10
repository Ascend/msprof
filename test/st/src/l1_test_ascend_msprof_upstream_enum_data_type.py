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
"""
dataType 上游枚举对齐校验。

上游权威定义：cann/metadef 的 inc/external/graph/c_types.h 中的 C_DataType。
本仓库两份副本：
- Python: analysis/common_func/ms_constant/ge_enum_constant.py / GeDataType
- C++   : analysis/csrc/domain/services/persistence/host/number_mapping.cpp / geDataTypeMap

上游命名统一带 C_DT_ 前缀，本仓库带 DT_ 前缀且早期成员不带前缀，
因此比对前统一去掉 DT_/C_DT_ 前缀再判等。
"""

from src.upstream_enum_checker import UpstreamEnumChecker, run_case

PY_ENUM_PATH = "analysis/common_func/ms_constant/ge_enum_constant.py"
CPP_MAP_PATH = "analysis/csrc/domain/services/persistence/host/number_mapping.cpp"


class TestUpstreamEnumDataType(UpstreamEnumChecker):
    CASE_NAME = "upstream_enum_data_type"
    ENUM_LABEL = "dataType"
    SOURCE_KEY = "c_types"
    SYNC_HINT = f"{PY_ENUM_PATH} / GeDataType 与 {CPP_MAP_PATH} / geDataTypeMap 需同步修改"

    @staticmethod
    def _normalize(entries):
        """去掉 DT_ 前缀，使 C_DT_HIFLOAT4 与 DT_HIFLOAT4 可比。"""
        return {
            value: (name[3:] if name.startswith("DT_") else name)
            for value, name in entries.items()
        }

    def parse_upstream(self, text):
        from utils.upstream_enum import parse_c_enum

        return parse_c_enum(text, "C_DataType", self.SOURCE_KEY, "C_DT_")

    def read_local_enum_pairs(self):
        python_enum = self._normalize(self.read_python_enum(PY_ENUM_PATH, "GeDataType"))
        cpp_enum = self._normalize(self.read_cpp_map(CPP_MAP_PATH, "geDataTypeMap"))
        return [("Python GeDataType", python_enum), ("C++ geDataTypeMap", cpp_enum)]


def test_upstream_enum_data_type():
    run_case(TestUpstreamEnumDataType)


if __name__ == "__main__":
    test_upstream_enum_data_type()
