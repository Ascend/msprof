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

"""
PROF 目录级并行解析的数据模型、当前层收集、资源探测与并发规划。
本模块只承担纯计算与只读探测，不创建进程；进程调度见
``prof_parallel_scheduler``，子进程命令重建见 ``prof_task_executor``。
CPU 调度额度固定为 32，内存按最大 PROF 数据量的 15 倍预留，外层最多 8 个进程。
"""

import os
import re
from dataclasses import dataclass, replace
from fractions import Fraction
from typing import Callable, List, Optional, Tuple

from common_func.common import get_data_file_memory, is_linux, warn
from common_func.data_check_manager import DataCheckManager
from common_func.ms_constant.str_constant import StrConstant

PROF_MEMORY_FACTOR = 15
MAX_PROF_PROCESSES = 8
# 单 PROF 的固定 CPU 调度额度，不代表实际用核数或内部线程数。
PROF_CPU_CORES = 32
# 资源探测与规划原因码（打屏/日志诊断用，不是新增业务错误码）。
REASON_RESOURCE_UNKNOWN = 'RESOURCE_UNKNOWN'
REASON_INPUT_SIZE_UNKNOWN_OR_ZERO = 'INPUT_SIZE_UNKNOWN_OR_ZERO'
REASON_TASK_UNSUPPORTED = 'TASK_UNSUPPORTED'
REASON_PARALLEL_BUDGET_LT_2 = 'PARALLEL_BUDGET_LT_2'
REASON_SINGLE_TASK = 'SINGLE_TASK'


@dataclass(frozen=True)
class CommandOptions:
    """
    一次命令调用的不可变参数快照。
    """

    input_root: str
    command: str  # import / export
    command_type: Optional[str]  # summary / timeline / db
    cluster_flag: bool
    export_mode: Optional[int]
    model_id: Optional[int]
    iteration_id: Optional[int]  # 原始请求值，保留 None
    iteration_count: int
    export_format: Optional[str]
    reports_path: str
    clear_mode: bool


@dataclass(frozen=True)
class ProfTask:
    """
    一个待执行的 PROF 任务。
    """

    task_id: int  # 固定发现顺序
    prof_path: str  # 规范化绝对路径
    host_path: str  # 本次待执行 host，可为空
    device_paths: Tuple[str, ...]  # 本次待执行 device
    all_result_paths: Tuple[str, ...]  # 内存估算覆盖的完整 PROF 输入
    raw_bytes: Optional[int]  # None 表示无法可靠估算
    is_cluster: bool  # 沿用原业务的集群场景上下文，不代表实际多机多卡，也不参与并行准入

    def to_path_table(self, command: str) -> dict:
        """在原业务调用边界生成映射，避免重复存储可变的路径状态。"""
        paths = {
            StrConstant.HOST_PATH: self.host_path,
            StrConstant.DEVICE_PATH: list(self.device_paths),
            'collection_path': self.prof_path,
        }
        if command == 'import':
            paths['is_cluster'] = self.is_cluster
        return paths


@dataclass(frozen=True)
class ResourceSnapshot:
    """归一化后的规划输入。"""

    available_ram_bytes: Optional[int]
    effective_cpu: Optional[Fraction]
    unknown_reasons: Tuple[str, ...] = ()


@dataclass(frozen=True)
class ExecutionPlan:
    """一次调用的执行计划。"""

    mode: str  # EMPTY / SERIAL / PARALLEL
    max_workers: int
    reason_codes: Tuple[str, ...]


@dataclass(frozen=True)
class DiscoveryResult:
    """收集器的一次发现结果。"""

    tasks: List[ProfTask]
    valid_prof_count: int  # N_valid：合法采集根数量
    valid_data_count: int  # 按合法 host/device 计数，仅累加一次


class ProfTaskCollector:
    """只收集输入目录直接子级中的合法采集根，不限制目录名称，不递归发现。

    直接输入含合法 host/device 元数据的根时仅生成根自身任务；普通入口
    保留公共迭代器的原逆序，目录合法性完全沿用公共迭代器。
    """

    _INVALID_DIR_TEMPLATE = 'Invalid parsing dir(%s), -dir must be profiling data dir, such as PROF_XXX_XXX_XXX'

    def __init__(self: any, file_name: str, on_valid_member: Optional[Callable[[str, bool], None]] = None) -> None:
        self._file_name = file_name
        self._on_valid_member = on_valid_member

    def collect(self: any, options: CommandOptions) -> DiscoveryResult:
        """发现当前层合法 PROF 并构造待执行任务。"""
        root_entries = list(
            DataCheckManager.iter_valid_profiling_sub_paths(options.input_root, self._file_name, skip_invalid=False)
        )
        root_valid = [entry for entry in root_entries if entry[2]]
        if root_valid:
            task = self._build_task(0, options.input_root, root_valid, False)
            return DiscoveryResult([task], 1, self._valid_data_count([task]))
        tasks = []
        for _name, prof_path, _metadata in root_entries:
            members = list(
                DataCheckManager.iter_valid_profiling_sub_paths(prof_path, self._file_name, skip_invalid=True)
            )
            valid_members = [member for member in members if member[2]]
            for _sub_name, sub_path, contain_info_json in members:
                if sub_path and not contain_info_json:
                    warn(self._file_name, self._INVALID_DIR_TEMPLATE % prof_path)
            if not valid_members:
                continue
            tasks.append(self._build_task(len(tasks), prof_path, valid_members, True))
        return DiscoveryResult(tasks, len(tasks), self._valid_data_count(tasks))

    @staticmethod
    def _valid_data_count(tasks: List[ProfTask]) -> int:
        return sum(int(bool(task.host_path)) + len(task.device_paths) for task in tasks)

    def _build_task(self: any, task_id: int, prof_path: str, valid_members: list, is_cluster: bool) -> ProfTask:
        host_path = ''
        device_paths = []
        for sub_dir, valid_sub_path, _contain in valid_members:
            if self._on_valid_member is not None:
                self._on_valid_member(valid_sub_path, is_cluster)
            if sub_dir == StrConstant.HOST_PATH:
                host_path = valid_sub_path
            else:
                device_paths.append(valid_sub_path)
        all_result_paths = tuple(([host_path] if host_path else []) + device_paths)
        return build_prof_task(task_id, prof_path, host_path, device_paths, all_result_paths, is_cluster)


def build_prof_task(
    task_id: int,
    prof_path: str,
    host_path: str,
    device_paths: list,
    all_result_paths: Tuple[str, ...],
    is_cluster: bool,
) -> ProfTask:
    """按已校验成员构造任务；集群路径复用同一构造规则。"""
    return ProfTask(
        task_id=task_id,
        prof_path=prof_path,
        host_path=host_path,
        device_paths=tuple(device_paths),
        all_result_paths=all_result_paths,
        raw_bytes=None,
        is_cluster=is_cluster,
    )


def _prepare_parallel_tasks(tasks: List[ProfTask]) -> List[ProfTask]:
    """仅为可能并行的整批任务统计原始数据大小，不重扫目录布局。"""
    return [replace(task, raw_bytes=_get_raw_data_bytes(task.all_result_paths)) for task in tasks]


def _get_raw_data_bytes(result_dirs: Tuple[str, ...]) -> Optional[int]:
    """data 仅含直属文件；复用原统计方法，将 MiB 转为字节，读取失败时预算未知。"""
    try:
        return sum(int(get_data_file_memory(result_dir) * (1 << 20)) for result_dir in result_dirs)
    except (OSError, ValueError):
        return None


class _ResourceProbeError(Exception):
    """
    适用资源约束无法可靠读取，不等同于无限制。
    """


class SystemResourceProbe:
    """
    读取 Linux 有效 RAM/CPU；未知约束只能回原串行。
    """

    _V1_HUGE_LIMIT = 1 << 60

    def __init__(self, read_file=None, os_cpu_count=None):
        self._missing_files = set()
        self._cgroup_location = None
        self._read_file = read_file or self._default_read_file
        self._os_cpu_count = os_cpu_count or os.cpu_count

    def _default_read_file(self, path):
        try:
            with open(path, 'r', encoding='utf-8', errors='replace') as reader:
                return reader.read()
        except FileNotFoundError:
            self._missing_files.add(path)
            return None
        except OSError:
            return None

    def probe(self):
        if os.name != 'posix' or not is_linux():
            return ResourceSnapshot(None, None, ('RESOURCE_PROBE_PLATFORM_UNSUPPORTED',))
        self._missing_files.clear()
        self._cgroup_location = None
        values, reasons = [], []
        for name, callback in (('MEMORY', self._probe_ram), ('CPU', self._probe_cpu)):
            try:
                values.append(callback())
            except (_ResourceProbeError, OSError, ValueError, ZeroDivisionError) as error:
                values.append(None)
                reasons.append(name + '_UNKNOWN:' + str(error))
        return ResourceSnapshot(values[0], values[1], tuple(reasons))

    def _probe_ram(self):
        text = self._read_file('/proc/meminfo')
        if text is None:
            raise _ResourceProbeError('MEMINFO_UNREADABLE')
        available = None
        for line in text.splitlines():
            if line.startswith('MemAvailable:'):
                available = int(line.split()[1]) * 1024
                break
        if available is None or available < 0:
            raise _ResourceProbeError('MEM_AVAILABLE_UNKNOWN')
        for limit, usage in self._cgroup_limits(
            'memory', 'memory.max', 'memory.current', 'memory.limit_in_bytes', 'memory.usage_in_bytes'
        ):
            if limit is not None:
                available = min(available, max(0, limit - usage))
        return available

    def _probe_cpu(self):
        candidates = []
        count = self._os_cpu_count()
        if count is not None and count > 0:
            candidates.append(Fraction(count))
        try:
            candidates.append(Fraction(len(os.sched_getaffinity(0))))
        except AttributeError:
            value = self._cgroup_value('cpuset', 'cpuset.cpus.effective', 'cpuset.cpus')
            cpus = _parse_cpu_list(value or '')
            if cpus is None:
                raise _ResourceProbeError('AFFINITY_UNKNOWN')
            candidates.append(Fraction(cpus))
        for current, is_v2, is_root in self._cgroup_levels('cpu'):
            if is_v2:
                path = os.path.join(current, 'cpu.max')
                raw = self._read_file(path)
                if raw is None and self._cgroup_limit_absent('cpu', path, is_root):
                    continue
                if raw is None:
                    raise _ResourceProbeError('CPU_QUOTA_UNREADABLE:' + path)
                parts = raw.split()
                if len(parts) != 2 or int(parts[1]) <= 0:
                    raise _ResourceProbeError('CPU_QUOTA_UNPARSEABLE')
                if parts[0] != 'max':
                    quota = int(parts[0])
                    if quota < 0:
                        raise _ResourceProbeError('CPU_QUOTA_NEGATIVE')
                    candidates.append(Fraction(quota, int(parts[1])))
            else:
                quota = self._read_file(os.path.join(current, 'cpu.cfs_quota_us'))
                period = self._read_file(os.path.join(current, 'cpu.cfs_period_us'))
                if quota is None or period is None:
                    raise _ResourceProbeError('CPU_QUOTA_UNREADABLE')
                quota, period = int(quota), int(period)
                if period <= 0 or quota < -1:
                    raise _ResourceProbeError('CPU_QUOTA_UNPARSEABLE')
                if quota != -1:
                    candidates.append(Fraction(quota, period))
        if not candidates:
            raise _ResourceProbeError('CPU_CAPACITY_UNKNOWN')
        return min(candidates)

    def _cgroup_controllers(self, controller):
        if self._cgroup_location is None:
            self._cgroup_location = (self._read_file('/proc/self/cgroup'), self._read_file('/proc/self/mountinfo'))
        membership, mountinfo = self._cgroup_location
        if membership is None or mountinfo is None:
            raise _ResourceProbeError('CGROUP_LOCATION_UNREADABLE')
        declared = []
        for line in membership.splitlines():
            fields = line.split(':', 2)
            if len(fields) != 3:
                raise _ResourceProbeError('CGROUP_MEMBERSHIP_UNPARSEABLE')
            if fields[0] == '0' and fields[1] == '':
                declared.append((fields[2], True))
            elif controller in fields[1].split(','):
                declared.append((fields[2], False))
        results = []
        for line in mountinfo.splitlines():
            parts = line.split()
            if '-' not in parts:
                raise _ResourceProbeError('CGROUP_MOUNTINFO_UNPARSEABLE')
            dash = parts.index('-')
            if len(parts) <= dash + 3 or dash < 5:
                raise _ResourceProbeError('CGROUP_MOUNTINFO_UNPARSEABLE')
            root, mount_point = parts[3], parts[4]
            # mountinfo escapes spaces/backslashes with octal sequences.
            root = re.sub(r'\\([0-7]{3})', lambda match: chr(int(match[1], 8)), root)
            mount_point = re.sub(r'\\([0-7]{3})', lambda match: chr(int(match[1], 8)), mount_point)
            fs_type, options = parts[dash + 1], parts[dash + 3]
            for cgroup_path, is_v2 in declared:
                if not (
                    (is_v2 and fs_type == 'cgroup2')
                    or (not is_v2 and fs_type == 'cgroup' and controller in options.split(','))
                ):
                    continue
                if cgroup_path != root and not cgroup_path.startswith(root.rstrip('/') + '/'):
                    continue
                relative = cgroup_path[len(root) :].strip('/')
                if '..' in relative.split('/'):
                    raise _ResourceProbeError('CGROUP_HIDDEN_ANCESTOR')
                results.append(
                    (os.path.join(mount_point, relative) if relative else mount_point, mount_point, is_v2, root)
                )
        if declared and not results:
            raise _ResourceProbeError('CGROUP_MOUNT_UNKNOWN')
        # 同一适用层级存在多个挂载时，使用能覆盖祖先链的完整挂载。
        complete = [value for value in results if value[3] == '/']
        for _path, is_v2 in declared:
            if not any(value[2] == is_v2 for value in complete):
                raise _ResourceProbeError('CGROUP_HIDDEN_ANCESTOR')
        return [value[:3] for value in complete]

    def _cgroup_levels(self, controller):
        for current, mount_point, is_v2 in self._cgroup_controllers(controller):
            while True:
                yield current, is_v2, current == mount_point
                if current == mount_point:
                    break
                parent = os.path.dirname(current)
                if parent == current:
                    raise _ResourceProbeError('CGROUP_ANCESTOR_UNKNOWN')
                current = parent

    def _cgroup_value(self, controller, v2_file, v1_file):
        for current, is_v2, _root in self._cgroup_levels(controller):
            file_name = v2_file if is_v2 else v1_file
            if file_name:
                value = self._read_file(os.path.join(current, file_name))
                if value and value.strip():
                    return value
        return None

    def _cgroup_limit_absent(self, controller, path, is_root):
        """仅 ENOENT 且根层或父层明确未启用控制器时，本层才没有对应限制。"""
        if path not in self._missing_files:
            return False
        if is_root:
            return True
        parent = os.path.dirname(os.path.dirname(path))
        enabled = self._read_file(os.path.join(parent, 'cgroup.subtree_control'))
        return enabled is not None and controller not in enabled.split()

    def _cgroup_limits(self, controller, v2_limit, v2_usage, v1_limit, v1_usage):
        values = []
        for current, is_v2, is_root in self._cgroup_levels(controller):
            limit_path = os.path.join(current, v2_limit if is_v2 else v1_limit)
            limit = self._read_file(limit_path)
            if limit is None and is_v2 and self._cgroup_limit_absent(controller, limit_path, is_root):
                continue
            usage = self._read_file(os.path.join(current, v2_usage if is_v2 else v1_usage))
            if limit is None or usage is None:
                raise _ResourceProbeError(controller.upper() + '_LIMIT_UNREADABLE')
            if limit.strip() == 'max':
                values.append((None, 0))
                continue
            limit, usage = int(limit), int(usage)
            if limit < 0 or usage < 0:
                raise _ResourceProbeError(controller.upper() + '_LIMIT_NEGATIVE')
            unlimited = controller == 'memory' and not is_v2 and limit >= self._V1_HUGE_LIMIT
            values.append((None if unlimited else limit, usage))
        return values


def _parse_cpu_list(value: str) -> Optional[int]:
    """解析 ``0-3,8`` 形式的 CPU 列表，返回数量。"""
    total = 0
    if not value:
        return None
    for item in value.split(','):
        item = item.strip()
        if not item:
            continue
        if '-' in item:
            start, _, end = item.partition('-')
            try:
                total += int(end) - int(start) + 1
            except ValueError:
                return None
        else:
            try:
                total += 1
                int(item)
            except ValueError:
                return None
    return total if total > 0 else None


def _parallel_task_supported(task: ProfTask, options: CommandOptions) -> bool:
    """仅限定并行命令范围；原集群场景标记不参与准入，合法性和芯片适配沿用原解析链。"""
    if not task.all_result_paths or options.command not in ('import', 'export'):
        return False
    if options.command == 'export' and options.command_type not in ('summary', 'timeline', 'db'):
        return False
    return True


def _build_plan(
    tasks: Tuple[ProfTask, ...],
    snapshot: ResourceSnapshot,
    eligibility_reasons: Tuple[str, ...],
) -> ExecutionPlan:
    """统一计算 CPU、内存及固定进程上限共同约束的并发计划。"""

    def serial(reason):
        return ExecutionPlan('SERIAL', 1, (reason,))

    if not tasks:
        return ExecutionPlan('EMPTY', 0, ())
    if eligibility_reasons:
        return serial(eligibility_reasons[0])
    if snapshot.unknown_reasons:
        return ExecutionPlan('SERIAL', 1, (REASON_RESOURCE_UNKNOWN,) + snapshot.unknown_reasons)
    if snapshot.available_ram_bytes is None or snapshot.effective_cpu is None:
        return serial(REASON_RESOURCE_UNKNOWN)
    if any(task.raw_bytes is None or task.raw_bytes <= 0 for task in tasks):
        return serial(REASON_INPUT_SIZE_UNKNOWN_OR_ZERO)
    # 固定并发度下，各槽位均按最大任务预留，保证任意任务顺序和后续补位都满足内存预算。
    memory = PROF_MEMORY_FACTOR * max(task.raw_bytes for task in tasks)
    cpu = Fraction(PROF_CPU_CORES)
    limits = [
        len(tasks),
        MAX_PROF_PROCESSES,
        max(0, snapshot.available_ram_bytes) // memory,
        int(max(Fraction(0), snapshot.effective_cpu) // cpu),
    ]
    workers = min(limits)
    if workers < 2:
        return serial(REASON_PARALLEL_BUDGET_LT_2)
    return ExecutionPlan('PARALLEL', workers, ())


def plan_prof_tasks(
    tasks: List[ProfTask],
    options: CommandOptions,
    snapshot: Optional[ResourceSnapshot] = None,
    eligibility_reasons: Tuple[str, ...] = (),
) -> ExecutionPlan:
    """以启动上下文与初始 CPU/内存生成固定并发计划。"""
    current = snapshot or ResourceSnapshot(None, None)
    reasons = (REASON_SINGLE_TASK,) if len(tasks) == 1 else tuple(eligibility_reasons)
    plan = _build_plan(tuple(tasks), current, reasons)
    if plan.mode == 'PARALLEL' and not all(_parallel_task_supported(task, options) for task in tasks):
        return ExecutionPlan('SERIAL', 1, (REASON_TASK_UNSUPPORTED,))
    return plan
