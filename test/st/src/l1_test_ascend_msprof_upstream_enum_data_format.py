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
dataFormat 上游枚举对齐校验。

上游权威定义：cann/metadef 的 inc/external/graph/c_types.h 中的 C_Format。
本仓库两份副本：
- Python: analysis/common_func/ms_constant/ge_enum_constant.py / GeDataFormat
- C++   : analysis/csrc/domain/services/persistence/host/number_mapping.cpp / geFormatMap

dataFormat 与 dataType 定义在同一份上游头文件里，历史上 dataType 的补充 PR 也顺带
修正过 FORMAT_NYUV(45) / FORMAT_NYUV_A(46)，因此单独用例看护。
"""

from src.upstream_enum_checker import UpstreamEnumChecker, run_case
from utils.upstream_enum import parse_c_enum

PY_ENUM_PATH = "analysis/common_func/ms_constant/ge_enum_constant.py"
CPP_MAP_PATH = "analysis/csrc/domain/services/persistence/host/number_mapping.cpp"


class TestUpstreamEnumDataFormat(UpstreamEnumChecker):
    CASE_NAME = "upstream_enum_data_format"
    ENUM_LABEL = "dataFormat"
    SOURCE_KEY = "c_types"
    SYNC_HINT = f"{PY_ENUM_PATH} / GeDataFormat 与 {CPP_MAP_PATH} / geFormatMap 需同步修改"

    @staticmethod
    def _normalize(entries):
        """去掉 DT_ 前缀：C++ 侧个别成员沿用 DT_ 写法时也能与上游对齐。"""
        return {
            value: (name[3:] if name.startswith("DT_") else name)
            for value, name in entries.items()
        }

    def parse_upstream(self, text):
        return parse_c_enum(text, "C_Format", self.SOURCE_KEY, "C_FORMAT_")

    def read_local_enum_pairs(self):
        python_enum = self._normalize(self.read_python_enum(PY_ENUM_PATH, "GeDataFormat"))
        cpp_enum = self._normalize(self.read_cpp_map(CPP_MAP_PATH, "geFormatMap"))
        return [("Python GeDataFormat", python_enum), ("C++ geFormatMap", cpp_enum)]


def test_upstream_enum_data_format():
    run_case(TestUpstreamEnumDataFormat)


if __name__ == "__main__":
    test_upstream_enum_data_format()
