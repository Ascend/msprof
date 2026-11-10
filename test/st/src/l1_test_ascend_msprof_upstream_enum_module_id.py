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
npuModuleMem 的 moduleId 上游枚举对齐校验。

上游权威定义：cann/runtime 的 pkg_inc/driver/hal_pkg/common_pkg.h 中的模块 id 枚举，
展示名取自同一头文件的 SVM_DECLARE_MODULE_NAME 宏。
本仓库两份副本：
- Python: analysis/profiling_bean/prof_enum/data_tag.py / ModuleName
- C++   : analysis/csrc/application/database/db_constant.h / MODULE_NAME_TABLE

该枚举决定 npu_module_mem_*.csv 的 Component 列取值，上游新增模块时需要同步，
否则新模块的内存会被展示成兜底的 UNKNOWN。
"""

from src.upstream_enum_checker import UpstreamEnumChecker, run_case
from utils.upstream_enum import parse_module_ids

PY_ENUM_PATH = "analysis/profiling_bean/prof_enum/data_tag.py"
CPP_MAP_PATH = "analysis/csrc/application/database/db_constant.h"

# 本仓库额外登记的 moduleId：PYPTO(59) 上游 common_pkg.h 暂未补充，按业务需要先登记。
# 上游未使用该 id，本地使用不影响正确性，因此不参与比对；若上游日后新增 59
# 但名称与本地不同，会由"同值异名"检查报出。


class TestUpstreamEnumModuleId(UpstreamEnumChecker):
    CASE_NAME = "upstream_enum_module_id"
    ENUM_LABEL = "moduleId"
    SOURCE_KEY = "common_pkg"
    SYNC_HINT = f"{PY_ENUM_PATH} / ModuleName 与 {CPP_MAP_PATH} / MODULE_NAME_TABLE 需同步修改"

    def parse_upstream(self, text):
        return parse_module_ids(text, self.SOURCE_KEY)

    def read_local_enum_pairs(self):
        python_enum = self.read_python_enum(PY_ENUM_PATH, "ModuleName")
        cpp_enum = self.read_cpp_string_map(CPP_MAP_PATH, "MODULE_NAME_TABLE")
        return [("Python ModuleName", python_enum), ("C++ MODULE_NAME_TABLE", cpp_enum)]


def test_upstream_enum_module_id():
    run_case(TestUpstreamEnumModuleId)


if __name__ == "__main__":
    test_upstream_enum_module_id()
