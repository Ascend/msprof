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
上游枚举对齐校验的公共能力。

msprof 里有一批枚举是"原样拷贝"上游头文件定义的，例如：
- dataType / dataFormat：cann/metadef 的 inc/external/graph/c_types.h
- npu moduleId：cann/runtime 的 pkg_inc/driver/hal_pkg/common_pkg.h

上游新增枚举值时，工具侧不会自动得知，只能靠人工跟随。本模块在冒烟阶段实时拉取
上游头文件并解析枚举，与仓库内维护的枚举做差集，把"上游新增了什么、上游文件在哪"
打印出来，供维护人员判断是否需要同步适配。

设计约束：
- 只做实时拉取，不保存快照，避免快照过期后用例形同虚设；
- 拉取或解析失败一律抛异常，由用例按失败上报，不允许静默跳过；
- 仅使用标准库，CI 容器内无 requests 等第三方依赖。
"""

import re
import urllib.error
import urllib.request

# 上游枚举来源。新增受维护的枚举时在此登记，用例只引用这里的常量。
UPSTREAM_SOURCES = {
    "c_types": {
        "url": "https://raw.gitcode.com/cann/metadef/raw/master/inc/external/graph/c_types.h",
        "repo": "cann/metadef",
        "path": "inc/external/graph/c_types.h",
        "branch": "master",
    },
    "common_pkg": {
        "url": "https://raw.gitcode.com/cann/runtime/raw/master/pkg_inc/driver/hal_pkg/common_pkg.h",
        "repo": "cann/runtime",
        "path": "pkg_inc/driver/hal_pkg/common_pkg.h",
        "branch": "master",
    },
}

# 拉取超时。CI 网络抖动时宁可失败上报，也不要长时间挂住整个冒烟。
FETCH_TIMEOUT = 30

# 部分上游仓库对同一 IP 有访问频次限制，命中时重试次数。仍失败则按拉取失败上报。
FETCH_RETRY = 2

_HEADERS = {"User-Agent": "msprof-st-upstream-enum/1.0"}


class UpstreamFetchError(RuntimeError):
    """上游头文件拉取失败。用例应将其视为失败，而不是跳过。"""


class UpstreamParseError(RuntimeError):
    """上游头文件解析失败。通常意味着上游格式变化，需要人工介入。"""


def describe_source(source_key: str) -> str:
    """返回可读的上游来源描述，用于失败信息与差异报告。"""
    source = UPSTREAM_SOURCES[source_key]
    return f"{source['repo']}/{source['path']}@{source['branch']}"


def fetch_upstream_text(source_key: str) -> str:
    """
    拉取上游头文件全文。

    :param source_key: UPSTREAM_SOURCES 中的键
    :return: 文件全文
    :raises UpstreamFetchError: 网络异常、HTTP 非 200、内容为空
    """
    if source_key not in UPSTREAM_SOURCES:
        raise KeyError(f"unknown upstream source: {source_key}")

    source = UPSTREAM_SOURCES[source_key]
    url = source["url"]
    descriptor = describe_source(source_key)
    last_error = None

    for attempt in range(FETCH_RETRY + 1):
        try:
            request = urllib.request.Request(url, headers=_HEADERS)
            with urllib.request.urlopen(request, timeout=FETCH_TIMEOUT) as response:
                status = getattr(response, "status", 200)
                body = response.read().decode("utf-8", "replace")
            if status != 200:
                raise UpstreamFetchError(f"HTTP {status}")
            if not body.strip():
                raise UpstreamFetchError("empty response body")
            return body
        except (urllib.error.URLError, urllib.error.HTTPError, OSError, UpstreamFetchError) as err:
            last_error = err
            if attempt < FETCH_RETRY:
                continue

    raise UpstreamFetchError(
        f"failed to fetch upstream header {descriptor} ({url}) after {FETCH_RETRY + 1} attempts: {last_error}"
    )


def _strip_c_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def _iter_enum_entries(body: str, source_key: str):
    """
    逐项产出 (名称, 枚举值)。

    只接受 NAME 或 NAME = 整数字面量 两种形式。遇到无法静态求解的赋值表达式
    （如 (1 << 6)、符号引用、宏）时立即抛 UpstreamParseError：
    这类项若被跳过，既会丢失该成员，也会让后续隐式递增成员沿用错误的基准值，
    从而"错误但通过"对齐检查。

    :raises UpstreamParseError: 存在无法静态求解的赋值表达式
    """
    next_value = 0
    for item in body.split(","):
        item = item.strip()
        if not item:
            continue
        if "=" in item:
            name, raw_value = item.split("=", 1)
            name = name.strip()
            raw_value = raw_value.strip()
            try:
                next_value = int(raw_value, 0)
            except ValueError:
                raise UpstreamParseError(
                    f"unsupported enumerator value '{raw_value}' for '{name}' in "
                    f"{describe_source(source_key)}; only integer literals are supported, "
                    f"because a skipped member would corrupt the implicit values that follow it"
                )
        else:
            name = item
        yield name, next_value
        next_value += 1


def _collect_enum_entries(body: str, source_key: str, name_prefix: str = "") -> dict:
    """
    把枚举体解析为 {枚举值: 枚举名}，并按需去掉上游统一前缀。

    :param body: 枚举体（已去注释）
    :param source_key: 来源键，仅用于错误信息
    :param name_prefix: 上游枚举名统一前缀，如 C_DT_
    :return: {枚举值: 枚举名}
    """
    entries = {}
    for name, value in _iter_enum_entries(body, source_key):
        entries[value] = normalize_name(name, name_prefix)
    if not entries:
        raise UpstreamParseError(
            f"enum in {describe_source(source_key)} parsed to an empty result"
        )
    return entries


def parse_c_enum(text: str, enum_name: str, source_key: str, name_prefix: str = "") -> dict:
    """
    解析形如 typedef enum { ... } EnumName; 的 C 枚举。

    :param text: 头文件全文
    :param enum_name: typedef 的别名，如 C_DataType
    :param source_key: 来源键，仅用于错误信息
    :param name_prefix: 上游枚举名统一前缀，如 C_DT_，用于和仓库内名字对齐
    :return: {枚举值: 枚举名}
    :raises UpstreamParseError: 找不到枚举定义或解析结果为空
    """
    # 枚举体内不会出现右花括号，用 [^}]* 限定范围，避免非贪婪跨到下一个枚举。
    pattern = r"typedef\s+enum\s*\{([^}]*)\}\s*" + re.escape(enum_name) + r"\s*;"
    matched = re.search(pattern, text, re.S)
    if not matched:
        raise UpstreamParseError(
            f"enum '{enum_name}' not found in upstream header {describe_source(source_key)}; "
            f"the upstream layout may have changed"
        )
    return _collect_enum_entries(_strip_c_comments(matched.group(1)), source_key, name_prefix)


def _find_module_id_enum(stripped: str):
    """
    定位含 module id 的匿名枚举体。

    头文件里可能有多个匿名 enum，不能简单取第一个。这里按"枚举体内出现 *_MODULE_ID"
    来筛选，避免取错枚举导致比对基准整体错位。
    """
    for matched in re.finditer(r"enum\s*\{\s*(.*?)\}\s*;", stripped, re.S):
        if re.search(r"\b[A-Za-z_][A-Za-z0-9_]*_MODULE_ID\b", matched.group(1)):
            return matched
    return None


def parse_module_ids(text: str, source_key: str) -> dict:
    """
    解析 runtime 的 module id 定义。

    该头文件里有两处信息需要合并：
    - 匿名枚举给出 module id 的全集（含"已分配但未命名"的 id）；
    - SVM_DECLARE_MODULE_NAME 宏给出用于展示的字符串名。
    以枚举为取值权威来源，宏里的字符串作为名字，宏未覆盖的 id 退回枚举名。

    MAX_MODULE_ID 是边界标记而非真实模块，其数值随上游新增模块而变大（上游头文件
    注释即要求新增 id 后抬高该值），因此这里动态读取并剔除，不与固定值比对——
    否则"上游新增模块"这一最主要场景会被误判成解析失败，看不到新增了哪个 id。

    :param text: 头文件全文
    :param source_key: 来源键
    :return: {module id: 展示名}
    :raises UpstreamParseError: 枚举缺失、边界标记缺失或解析为空
    """
    stripped = _strip_c_comments(text)
    if not re.search(r"\bMAX_MODULE_ID\s*=\s*\d+", stripped):
        raise UpstreamParseError(
            f"sentinel 'MAX_MODULE_ID' not found in upstream header "
            f"{describe_source(source_key)}; module id enum boundary may have changed"
        )

    enum_match = _find_module_id_enum(stripped)
    if not enum_match:
        raise UpstreamParseError(f"no enum definition found in upstream header {describe_source(source_key)}")

    # 枚举名 -> id
    enum_entries = {}
    for name, value in _iter_enum_entries(enum_match.group(1), source_key):
        enum_entries[name] = value
    enum_entries.pop("MAX_MODULE_ID", None)
    if not enum_entries:
        raise UpstreamParseError(f"module id enum in {describe_source(source_key)} parsed to an empty result")

    # SVM_DECLARE_MODULE_NAME 里 [XXX_MODULE_ID] = "NAME" 给出展示名
    display_names = {}
    macro_match = re.search(r"SVM_DECLARE_MODULE_NAME\s*\([^)]*\)[^=]*=\s*\{(.*?)\}", stripped, re.S)
    if macro_match:
        for item in re.finditer(r"\[([A-Za-z_][A-Za-z0-9_]*)\]\s*=\s*\"([^\"]*)\"", macro_match.group(1)):
            display_names[item.group(1)] = item.group(2)

    result = {}
    for enum_name, value in enum_entries.items():
        if enum_name == "UNKNOWN_MODULE_ID":
            result[value] = "UNKNOWN"
            continue
        base = enum_name
        for suffix in ("_MODULE_ID",):
            if base.endswith(suffix):
                base = base[: -len(suffix)]
        # 宏给出的是工具侧实际展示名，优先采用；否则用去掉后缀的枚举名。
        result[value] = display_names.get(enum_name, base)
    return result


def normalize_name(name: str, prefix: str = "") -> str:
    """去掉上游枚举名里的统一前缀，便于和我们维护的名字比对。"""
    result = name.strip()
    if prefix and result.startswith(prefix):
        result = result[len(prefix):]
    return result


def diff_enum(ours: dict, upstream: dict, label: str, source_key: str) -> list:
    """
    对比仓库内枚举与上游枚举，返回可读的差异报告行。

    比对规则是单向包含：上游有的每个枚举值，本地必须存在且名称一致，否则报错。
    本地独有的值不视为问题——那可能是有意为之的特殊适配，上游既然没用，
    本地用一下不影响业务正确性，因此不参与比对、也不输出。

    值为 NAME 的映射两侧都可能带前缀差异，因此在调用前应先用 normalize_name 归一化。

    :param ours: {值: 名称}，仓库内维护的枚举
    :param upstream: {值: 名称}，上游解析结果
    :param label: 枚举用途标签，如 dataType
    :param source_key: 上游来源键
    :return: 差异描述行列表，空列表表示上游的每个值本地都有且名称一致
    """
    lines = []
    ours_values = set(ours)
    upstream_values = set(upstream)

    added = sorted(upstream_values - ours_values)
    name_mismatch = sorted(
        value
        for value in ours_values & upstream_values
        if ours[value] != upstream[value]
    )

    if not (added or name_mismatch):
        return lines

    lines.append(f"[UPSTREAM-ENUM-DRIFT] {label}")
    lines.append(f"  upstream: {describe_source(source_key)}")
    lines.append(f"  上游新增（本地缺失，需同步适配）: {_format_entries(upstream, added)}")
    lines.append(f"  同值异名（本地名称与上游不一致）: {_format_name_mismatch(ours, upstream, name_mismatch)}")
    return lines


def _format_entries(entries: dict, keys: list) -> str:
    if not keys:
        return "无"
    return ", ".join(f"{key} -> {entries[key]}" for key in keys)


def _format_name_mismatch(ours: dict, upstream: dict, keys: list) -> str:
    if not keys:
        return "无"
    return ", ".join(f"{key}: 仓库={ours[key]} / 上游={upstream[key]}" for key in keys)
