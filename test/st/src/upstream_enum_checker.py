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
上游枚举对齐校验基类。

msprof 里有一批枚举是原样拷贝上游头文件定义的，并且同时在 Python 与 C++ 两份代码里
各维护一份。上游新增枚举值时工具侧不会自动得知，只能人工跟随。本基类负责：
1. 读取仓库内维护的两份枚举（Python 侧 + C++ 侧）；
2. 校验两份副本彼此一致（历史上多次出现只改一侧的情况）；
3. 实时拉取上游头文件并解析，做差集比对；
4. 把差异整理成可读报告，标注"新增了什么、上游文件在哪、需要同步哪些文件"。

拉取或解析失败一律按失败上报，不做静默跳过。
"""

import os
import re
import sys

from utils.upstream_enum import (
    UpstreamFetchError,
    UpstreamParseError,
    describe_source,
    diff_enum,
    fetch_upstream_text,
    parse_c_enum,
    parse_module_ids,
)

# 本文件位于 <repo>/test/st/src/，向上三级即仓库根目录。
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))


class UpstreamEnumChecker:
    """上游枚举对齐校验的公共实现，由各具体用例继承。"""

    # 用例名，用于报告标题
    CASE_NAME = ""
    # 枚举用途标签，如 dataType
    ENUM_LABEL = ""
    # 上游来源键，见 utils.upstream_enum.UPSTREAM_SOURCES
    SOURCE_KEY = ""
    # 仓库内需要同步维护的文件，diff 时提示给维护者
    SYNC_HINT = ""

    # ------------------------- 仓库内枚举读取 -------------------------

    @staticmethod
    def read_python_enum(relative_path, class_name):
        """
        读取 Python 侧 Enum 类定义，返回 {值: 名称}。

        只解析"名称 = 整数字面量"形式的成员，够覆盖本次受维护的枚举。
        """
        full_path = os.path.join(REPO_ROOT, relative_path)
        if not os.path.isfile(full_path):
            raise FileNotFoundError(f"Python enum source not found: {full_path}")
        with open(full_path, "r", encoding="utf-8") as handle:
            source = handle.read()

        block = re.search(
            r"class\s+" + re.escape(class_name) + r"\s*\(\s*Enum\s*\)\s*:(.*?)(?=\n@|\nclass\s|\Z)",
            source,
            re.S,
        )
        if not block:
            raise ValueError(f"class '{class_name}' not found in {relative_path}")

        entries = {}
        for line in block.group(1).splitlines():
            member = re.match(
                r"\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(0[xX][0-9a-fA-F]+|\d+)\s*(?:#.*)?$", line)
            if member:
                entries[int(member.group(2), 0)] = member.group(1)
        if not entries:
            raise ValueError(f"class '{class_name}' in {relative_path} parsed to an empty result")
        return entries

    @staticmethod
    def read_cpp_map(relative_path, map_name):
        """读取 C++ 侧 unordered_map<..., std::string> 常量，返回 {值: 名称}。"""
        full_path = os.path.join(REPO_ROOT, relative_path)
        if not os.path.isfile(full_path):
            raise FileNotFoundError(f"C++ mapping source not found: {full_path}")
        with open(full_path, "r", encoding="utf-8") as handle:
            source = handle.read()

        block = re.search(re.escape(map_name) + r"\s*\{(.*?)\n\};", source, re.S)
        if not block:
            raise ValueError(f"map '{map_name}' not found in {relative_path}")

        entries = {}
        for item in re.finditer(r"\{\s*([^,\s]+)\s*,\s*\"([^\"]*)\"\s*\}", block.group(1)):
            raw_value = item.group(1)
            if not re.match(r"^(0[xX][0-9a-fA-F]+|\d+|UINT32_MAX|UINT16_MAX)$", raw_value):
                # 键不是数字字面量时无法参与比对，若静默跳过会漏检该值的漂移（假通过），
                # 与"解析失败一律按失败上报"的约定冲突，这里直接判失败。
                raise ValueError(
                    f"unsupported key '{raw_value}' in map '{map_name}' of {relative_path}; "
                    f"only numeric literals, UINT32_MAX and UINT16_MAX are supported"
                )
            if raw_value == "UINT32_MAX":
                value = 0xFFFFFFFF
            elif raw_value == "UINT16_MAX":
                value = 0xFFFF
            else:
                value = int(raw_value, 0)
            entries[value] = item.group(2)
        if not entries:
            raise ValueError(f"map '{map_name}' in {relative_path} parsed to an empty result")
        return entries

    @staticmethod
    def read_cpp_string_map(relative_path, map_name):
        """
        读取 C++ 侧 unordered_map<std::string, uint16_t> 常量，返回 {值: 名称}。

        与 read_cpp_map 保持同一策略：不认识的条目一律报错，不静默丢弃。
        条目被丢掉时它既不参与比对也不会报错，会造成"假通过"。
        """
        full_path = os.path.join(REPO_ROOT, relative_path)
        if not os.path.isfile(full_path):
            raise FileNotFoundError(f"C++ mapping source not found: {full_path}")
        with open(full_path, "r", encoding="utf-8") as handle:
            source = handle.read()

        block = re.search(re.escape(map_name) + r"\s*=\s*\{(.*?)\};", source, re.S)
        if not block:
            raise ValueError(f"map '{map_name}' not found in {relative_path}")

        entries = {}
        for item in re.finditer(r"\{\s*([^{}]*?)\s*\}", block.group(1)):
            body = item.group(1)
            matched = re.fullmatch(r"\"([^\"]+)\"\s*,\s*(0[xX][0-9a-fA-F]+|\d+)", body)
            if not matched:
                # 例如 {NA, UINT16_MAX - 1}、宏或表达式，无法静态求解，直接判失败。
                raise ValueError(
                    f"unsupported entry '{{{body}}}' in map '{map_name}' of {relative_path}; "
                    f"only {{\"NAME\", integer literal}} entries are supported"
                )
            entries[int(matched.group(2), 0)] = matched.group(1)
        if not entries:
            raise ValueError(f"map '{map_name}' in {relative_path} parsed to an empty result")
        return entries

    # ------------------------- 校验编排 -------------------------

    def parse_upstream(self, text):
        """解析上游头文件，由子类实现。"""
        raise NotImplementedError

    def read_local_enum_pairs(self):
        """
        返回 [(展示名, {值: 名称})]，至少一个元素；多个元素表示需要互相一致的两份副本。
        由子类实现。
        """
        raise NotImplementedError

    def _check_local_consistency(self, local_pairs):
        """校验仓库内多份副本彼此一致，返回报告行。"""
        lines = []
        if len(local_pairs) < 2:
            return lines
        base_name, base_entries = local_pairs[0]
        for other_name, other_entries in local_pairs[1:]:
            base_only = sorted(set(base_entries) - set(other_entries))
            other_only = sorted(set(other_entries) - set(base_entries))
            name_mismatch = sorted(
                value for value in set(base_entries) & set(other_entries)
                if base_entries[value] != other_entries[value]
            )
            if not (base_only or other_only or name_mismatch):
                continue
            lines.append(f"[LOCAL-ENUM-INCONSISTENT] {self.ENUM_LABEL}: {base_name} vs {other_name}")
            if base_only:
                lines.append(f"  仅存在于 {base_name}: {self._format(base_entries, base_only)}")
            if other_only:
                lines.append(f"  仅存在于 {other_name}: {self._format(other_entries, other_only)}")
            if name_mismatch:
                lines.append(
                    "  同值异名: "
                    + ", ".join(
                        f"{value}: {base_name}={base_entries[value]} / "
                        f"{other_name}={other_entries[value]}"
                        for value in name_mismatch
                    )
                )
            lines.append(f"  两份副本必须同步修改: {self.SYNC_HINT}")
        return lines

    @staticmethod
    def _format(entries, keys):
        return ", ".join(f"{key} -> {entries[key]}" for key in keys)

    def check(self):
        """
        执行完整校验。存在任何差异即抛 AssertionError，使冒烟标红。

        校验规则：上游有的每个枚举值，本地必须存在且名称一致。本地独有的值
        可能是特殊适配，不视为问题（详见 utils.upstream_enum.diff_enum）。
        """
        text = fetch_upstream_text(self.SOURCE_KEY)
        upstream = self.parse_upstream(text)

        local_pairs = self.read_local_enum_pairs()
        lines = self._check_local_consistency(local_pairs)

        # 以本地每份副本分别与上游比对，任一副本缺值或异名都判失败。
        for display_name, local_entries in local_pairs:
            drift = diff_enum(local_entries, upstream, f"{self.ENUM_LABEL} ({display_name})", self.SOURCE_KEY)
            lines.extend(drift)

        if lines:
            report = "\n".join(lines)
            report += f"\n  上游文件: {describe_source(self.SOURCE_KEY)}"
            report += f"\n  同步位置: {self.SYNC_HINT}"
            raise AssertionError(f"上游枚举与本仓库不一致:\n{report}")

        print(
            f"[UPSTREAM-ENUM-OK] {self.ENUM_LABEL}: {len(local_pairs)} 份本地副本已覆盖上游全部枚举值，"
            f"上游枚举 {len(upstream)} 项，来源 {describe_source(self.SOURCE_KEY)}"
        )


def run_case(checker_class):
    """用例统一入口，保证拉取/解析失败以非 0 退出码在冒烟中可见。"""
    try:
        checker_class().check()
    except (UpstreamFetchError, UpstreamParseError) as err:
        print(f"[UPSTREAM-ENUM-UNAVAILABLE] {type(err).__name__}: {err}", file=sys.stderr)
        raise
