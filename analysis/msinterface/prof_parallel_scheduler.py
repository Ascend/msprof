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

"""有界 PROF 任务调度器与 worker 通道。

通道使用 Linux Unix SOCK_SEQPACKET 的完整 JSON 消息，父端非阻塞。
Linux 专用 API 的 no-member 局部豁免仅用于 Windows 静态检查，运行资格仍由 Linux/fork 门控。
进程/通道生命周期、取消与回收集中在此，不使用 pickle 或后台通信线程。
"""

import json
import logging
import math
import multiprocessing
import os
import select
import signal
import socket
import sys
import time

from collections import deque

from common_func.common import is_linux, print_info
from msinterface.prof_parallel_plan import MAX_PROF_PROCESSES
from msinterface.prof_parallel_plan import PROF_MEMORY_FACTOR, PROF_CPU_CORES, REASON_PARALLEL_BUDGET_LT_2
from msinterface.prof_parallel_plan import CommandOptions
from msinterface.prof_parallel_plan import ExecutionPlan
from msinterface.prof_parallel_plan import plan_prof_tasks
from msinterface.prof_parallel_plan import ProfTask
from msinterface.prof_parallel_plan import ResourceSnapshot
from msinterface.prof_parallel_plan import SystemResourceProbe
from msinterface.prof_parallel_plan import _prepare_parallel_tasks

# 每槽位预留 socketpair 两端和 fork 的四个 pipe FD，并留两个余量。
# 初始检查预留全部槽位的上界，包含已运行 worker 常驻和新建瞬时 FD。
_PARENT_FDS_PER_WORKER = 8
_KILL_SIGNAL = getattr(signal, 'SIGKILL', 9)


class ChannelProtocolError(Exception):
    """
    通道协议故障：超长、乱序、重复、非法 JSON 或校验失败。
    """


class _TaskTreeProbeError(RuntimeError):
    """
    任务树的归属或存活状态无法确认，不能视作清理成功。
    """


# ----------------------------------------------------------------------
# fork 保护：fork 出的子进程关闭本进程登记的调度通道端点，
# 使业务后代不继承外层调度协议端点；本进程自身持有不受影响。
# ----------------------------------------------------------------------
_FORK_GUARD_ENDPOINTS = set()


def _after_fork_close_inherited_endpoints():
    for endpoint in list(_FORK_GUARD_ENDPOINTS):
        endpoint.close()
    _FORK_GUARD_ENDPOINTS.clear()


if hasattr(os, 'register_at_fork'):
    os.register_at_fork(after_in_child=_after_fork_close_inherited_endpoints)


def _poll_fds(fds, events, timeout):
    """等待 Linux FD；毫秒超时向上取整，避免 select 的 FD 编号上限。"""
    poller = select.poll()  # pylint: disable=no-member
    for fd in fds:
        poller.register(fd, events)
    poller.poll(None if timeout is None else max(0, math.ceil(timeout * 1000)))


class _ProfWorkerChannel:
    """单个双向 packet 端点：保留消息边界，非阻塞接收、限时发送。"""

    VERSION = 1
    MAX_FRAME_BYTES = 65536

    def __init__(self, conn):
        conn.setblocking(False)
        self._conn, self._eof = conn, False
        _FORK_GUARD_ENDPOINTS.add(conn)

    @property
    def closed(self):
        return self._conn.fileno() < 0

    def fileno(self):
        return self._conn.fileno()

    def send_message(self, message, timeout=2.0):
        payload = json.dumps(message, ensure_ascii=False, default=str).encode('utf-8')
        if len(payload) > self.MAX_FRAME_BYTES:
            raise ChannelProtocolError('message exceeds maximum length')
        deadline = time.monotonic() + timeout
        while True:
            try:
                if self._conn.send(payload) != len(payload):
                    raise ChannelProtocolError('incomplete packet send')
                return
            except (BlockingIOError, InterruptedError):
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise ChannelProtocolError('channel send blocked')
                _poll_fds([self._conn], select.POLLOUT, remaining)  # pylint: disable=no-member
            except OSError as error:
                raise ChannelProtocolError('channel send failed: {}'.format(error)) from error

    def pump(self):
        messages = []
        while not self._eof:
            try:
                payload, _ancillary, flags, _address = self._conn.recvmsg(self.MAX_FRAME_BYTES)
            except BlockingIOError:
                break
            except InterruptedError:
                continue
            except OSError as error:
                raise ChannelProtocolError('channel receive failed: {}'.format(error)) from error
            if flags & (socket.MSG_TRUNC | socket.MSG_CTRUNC):
                raise ChannelProtocolError('truncated packet')
            if not payload:
                self._eof = True
                break
            try:
                message = json.loads(payload.decode('utf-8'))
            except (ValueError, UnicodeDecodeError) as error:
                raise ChannelProtocolError('invalid json packet') from error
            if not isinstance(message, dict):
                raise ChannelProtocolError('packet is not an object')
            messages.append(message)
        return messages

    def recv_blocking(self, timeout=None):
        deadline = None if timeout is None else time.monotonic() + timeout
        while True:
            messages = self.pump()
            if messages:
                return messages[0]
            if self._eof:
                raise EOFError('channel closed')
            remaining = None if deadline is None else max(0, deadline - time.monotonic())
            if remaining == 0:
                return None
            _poll_fds([self._conn], select.POLLIN, remaining)  # pylint: disable=no-member

    def close(self):
        _FORK_GUARD_ENDPOINTS.discard(self._conn)
        self._conn.close()


class _OwnedTask:
    """父进程侧一个已启动 worker 的生命周期记录。"""

    def __init__(self: any, task: ProfTask, deadline: float) -> None:
        self.task = task
        self.state = 'STARTING'  # STARTING / READY / RUNNING
        self.process = None
        self.channel = None
        self.run_attempted = False
        self.ready_pid = None
        self.pgid = None
        self.terminal = None
        self.reclaimed = False
        self.cancelled = False
        self.cancel_deadline = None
        self.start_failed = False
        self.worker_endpoint = None
        self.leader_identity = None
        self.cleanup_started = False
        self.cleanup_killed = False
        self.bootstrap_deadline = deadline


class _LoopState:
    """一次并行运行的调度状态。"""

    def __init__(self: any) -> None:
        self.first_exception = None  # (task_id, exception)
        self.peak_workers = 0


class ProfTaskScheduler:
    """按执行计划进行有界调度；不导入命令类，串行回调由入口注入。"""

    FILE_NAME = os.path.basename(__file__)
    STARTUP_TIMEOUT_SECONDS = 300.0
    _RUN_SEND_TIMEOUT_SECONDS = 1.0
    CANCEL_GRACE_SECONDS = 10.0
    _WAIT_SLICE_SECONDS = 0.5
    _IDLE_SLEEP_SECONDS = 0.005
    _CLEANUP_POLL_SECONDS = 0.05

    def __init__(self: any, process_factory=None, channel_factory=None, time_fn=None, sleep_fn=None) -> None:
        self._process_factory = process_factory or self._default_process_factory
        self._channel_factory = channel_factory or self._default_channel_factory
        self._time_fn = time_fn or time.monotonic
        self._sleep_fn = sleep_fn or time.sleep
        self._interrupted = False
        self._interrupt_signal = None
        self._group_snapshot = None
        self._snapshot_groups = set()
        self._seen_groups = set()
        self._unreclaimed = []

    @staticmethod
    def _default_process_factory(target, args, daemon):
        return multiprocessing.get_context('fork').Process(target=target, args=args, daemon=daemon)

    @staticmethod
    def _default_channel_factory():
        parent, worker = socket.socketpair(socket.AF_UNIX, socket.SOCK_SEQPACKET)  # pylint: disable=no-member
        try:
            return _ProfWorkerChannel(parent), worker
        except BaseException:
            parent.close()
            worker.close()
            raise

    def run(
        self: any,
        tasks: list,
        options: CommandOptions,
        plan: ExecutionPlan,
        serial_runner,
        valid_count: int = None,
        pending_count: int = None,
    ) -> None:
        """按计划执行任务；串行回调在父进程内调用原单目录执行链。"""
        task_count = len(tasks)
        if valid_count is None:
            valid_count = task_count
        if pending_count is None:
            pending_count = task_count
        print_info(self.FILE_NAME, 'PROF discovery finished: valid=%d, pending=%d.' % (valid_count, pending_count))
        if not tasks or plan.mode == 'EMPTY':
            print_info(self.FILE_NAME, 'PROF execution mode: mode=EMPTY, workers=0.')
            return
        if plan.mode != 'PARALLEL':
            reason = ','.join(plan.reason_codes) if plan.reason_codes else 'SERIAL'
            print_info(self.FILE_NAME, 'PROF execution mode: mode=SERIAL, workers=1, reason=%s.' % reason)
            for task in tasks:
                serial_runner(task)
            return
        print_info(
            self.FILE_NAME,
            'PROF multiprocessing mode enabled: mode=PARALLEL, workers=%d, max_workers=%d, '
            'memory_factor=%d, reason=%s.'
            % (
                plan.max_workers,
                MAX_PROF_PROCESSES,
                PROF_MEMORY_FACTOR,
                ','.join(plan.reason_codes) or 'RESOURCE_BUDGET',
            ),
        )
        from msinterface.prof_task_executor import run_prof_task

        self._run_parallel(tasks, options, plan, run_prof_task)

    # ------------------------------------------------------------------
    # 并行事件循环
    # ------------------------------------------------------------------
    def _run_parallel(self: any, tasks: list, options: CommandOptions, plan: ExecutionPlan, worker_callable) -> None:
        parent_identity = (os.getpid(), _process_start_time(os.getpid()))
        pending = deque(tasks)
        owned = {}
        loop = _LoopState()
        old_handlers = self._install_signal_handlers()
        try:
            while owned or pending:
                self._begin_group_probe(owned)
                progress = self._pump_channels(owned, loop)
                progress += self._reap_exited(owned, loop)
                progress += self._check_bootstrap_deadlines(owned, loop)
                # 中断和首个异常是停止派发的唯一依据，不另行维护循环模式。
                if self._interrupted or loop.first_exception is not None:
                    if self._interrupted:
                        for record in list(owned.values()):
                            self._cancel_task_tree(record)
                    else:
                        self._cancel_bootstrap_tasks(owned)
                    if owned:
                        if not progress:
                            self._wait_slice(owned)
                        continue
                    if self._interrupted:
                        raise KeyboardInterrupt(
                            'prof parallel scheduling cancelled by signal {}'.format(self._interrupt_signal)
                        )
                    if loop.first_exception is not None:
                        raise loop.first_exception[1]
                progress += self._dispatch_run(owned, loop)
                progress += self._spawn_pending(pending, owned, plan, options, worker_callable, parent_identity, loop)
                if not progress:
                    self._wait_slice(owned)
            if loop.first_exception is not None:
                raise loop.first_exception[1]
            print_info(self.FILE_NAME, 'PROF scheduling finished: peak_workers=%d.' % loop.peak_workers)
        finally:
            active_exception = sys.exc_info()[1]
            try:
                if owned:
                    try:
                        self._cancel_all_owned(owned)
                    except _TaskTreeProbeError:
                        if active_exception is None:
                            raise
                        logging.error('PROF task tree cleanup failed; preserving original exception.', exc_info=True)
            finally:
                self._restore_signal_handlers(old_handlers)
        if self._interrupted:
            raise KeyboardInterrupt('prof parallel scheduling cancelled by signal {}'.format(self._interrupt_signal))

    def _pump_channels(self: any, owned: dict, loop: _LoopState) -> int:
        progress = 0
        for record in list(owned.values()):
            if record.reclaimed or record.channel is None or record.channel.closed:
                continue
            try:
                messages = record.channel.pump()
            except ChannelProtocolError as err:
                self._record_failure(record, err, loop)
                self._cancel_task_tree(record)
                continue
            for message in messages:
                progress += 1
                try:
                    self._handle_message(record, message, loop)
                except (ChannelProtocolError, OSError) as err:
                    self._record_failure(record, err, loop)
                    self._cancel_task_tree(record)
                    break
            if (
                getattr(record.channel, '_eof', False)
                and record.terminal is None
                and not record.cancelled
                and not record.cleanup_started
            ):
                self._record_failure(
                    record,
                    RuntimeError(
                        'prof worker channel closed without terminal message for task {}'.format(record.task.task_id)
                    ),
                    loop,
                )
                self._cancel_task_tree(record)
        return progress

    def _handle_message(self: any, record: _OwnedTask, message: dict, loop: _LoopState) -> None:
        if message.get('version') != _ProfWorkerChannel.VERSION:
            raise ChannelProtocolError('unexpected protocol version')
        if message.get('task_id') != record.task.task_id:
            raise ChannelProtocolError('unexpected task id')
        message_type = message.get('type')
        if message_type == 'READY':
            if record.state != 'STARTING' or record.ready_pid is not None:
                raise ChannelProtocolError('duplicate or misplaced READY')
            ready_pid = message.get('pid')
            process_pid = getattr(record.process, 'pid', None)
            if ready_pid is None or (process_pid and ready_pid != process_pid):
                raise ChannelProtocolError('READY pid mismatch')
            pgid = message.get('pgid')
            if self._real_linux_process(record):
                if (
                    pgid != ready_pid
                    or os.getpgid(ready_pid) != ready_pid  # pylint: disable=no-member
                    or os.getsid(ready_pid) != ready_pid  # pylint: disable=no-member
                ):
                    raise ChannelProtocolError('worker has no independent session/group')
                record.leader_identity = _process_start_time(ready_pid)
                if record.leader_identity is None:
                    raise ChannelProtocolError('worker identity unavailable')
            record.ready_pid = ready_pid
            record.pgid = pgid
            record.state = 'READY'
            return
        if message_type in ('RETURNED', 'RAISED'):
            if not record.run_attempted:
                raise ChannelProtocolError('terminal before RUN')
            if record.terminal is not None:
                raise ChannelProtocolError('duplicate terminal message')
            record.terminal = message
            if message_type == 'RAISED':
                self._record_exception(record, message.get('exception') or {}, loop)
                self._cancel_task_tree(record)
            return
        raise ChannelProtocolError('unknown message type')

    def _reap_exited(self, owned, loop):
        progress = 0
        for record in list(owned.values()):
            if record.reclaimed:
                continue
            if self._has_no_child(record):
                self._reclaim(record, owned)
                progress += 1
                continue
            exit_code = record.process.exitcode
            if exit_code is None:
                continue
            if record.channel is not None and not record.channel.closed:
                try:
                    for message in record.channel.pump():
                        self._handle_message(record, message, loop)
                except (ChannelProtocolError, OSError) as error:
                    self._record_failure(record, error, loop)
            if not record.cancelled and not record.start_failed:
                if not record.run_attempted:
                    self._record_failure(
                        record,
                        RuntimeError('prof parallel worker for task {} exited before RUN'.format(record.task.task_id)),
                        loop,
                    )
                elif record.terminal is None or exit_code != 0:
                    self._record_failure(
                        record,
                        RuntimeError(
                            'prof parallel worker for task {} exited with code {} without terminal message '
                            'or with abnormal exit'.format(record.task.task_id, exit_code)
                        ),
                        loop,
                    )
            # The leader can exit before its descendants. Keep the reservation until
            # the owned group is empty; never wait for a pipe EOF held by descendants.
            if self._group_alive(record):
                self._cancel_task_tree(record)
                continue
            self._reclaim(record, owned)
            progress += 1
        return progress

    @staticmethod
    def _has_no_child(record):
        return record.process is None or (record.start_failed and not getattr(record.process, 'pid', None))

    def _check_bootstrap_deadlines(self: any, owned: dict, loop: _LoopState) -> int:
        progress = 0
        now = self._time_fn()
        for record in list(owned.values()):
            if record.cleanup_started:
                self._cancel_task_tree(record)
            if record.reclaimed or record.state != 'STARTING' or record.run_attempted:
                continue
            if record.bootstrap_deadline is not None and now > record.bootstrap_deadline:
                self._record_failure(
                    record,
                    RuntimeError('prof parallel bootstrap timeout for task {}'.format(record.task.task_id)),
                    loop,
                )
                self._cancel_task_tree(record)
                progress += 1
        return progress

    def _dispatch_run(self: any, owned: dict, loop: _LoopState) -> int:
        progress = 0
        for record in list(owned.values()):
            if self._interrupted or loop.first_exception is not None:
                return progress
            if record.state != 'READY' or record.run_attempted:
                continue
            record.run_attempted = True
            try:
                record.channel.send_message(
                    {
                        'type': 'RUN',
                        'version': _ProfWorkerChannel.VERSION,
                        'task_id': record.task.task_id,
                    },
                    # 仅重试尚未发送的 packet；短暂阻塞不直接判为通道故障。
                    timeout=self._RUN_SEND_TIMEOUT_SECONDS,
                )
            except (ChannelProtocolError, OSError) as err:
                self._record_failure(
                    record, RuntimeError('failed to send RUN for task {}: {}'.format(record.task.task_id, err)), loop
                )
                self._cancel_task_tree(record)
                return progress
            record.state = 'RUNNING'
            loop.peak_workers = max(loop.peak_workers, self._running_count(owned))
            progress += 1
        return progress

    def _spawn_pending(
        self: any,
        pending: deque,
        owned: dict,
        plan: ExecutionPlan,
        options: CommandOptions,
        worker_callable,
        parent_identity,
        loop: _LoopState,
    ) -> int:
        progress = 0
        while not self._interrupted and loop.first_exception is None and pending and len(owned) < plan.max_workers:
            candidate = pending.popleft()
            record = self._spawn(candidate, options, worker_callable, parent_identity, loop, owned)
            if record is None:
                break
            progress += 1
        return progress

    def _spawn(
        self: any,
        task: ProfTask,
        options: CommandOptions,
        worker_callable,
        parent_identity,
        loop: _LoopState,
        owned=None,
    ):
        record = _OwnedTask(task, self._time_fn() + self.STARTUP_TIMEOUT_SECONDS)
        if owned is not None:
            owned[task.task_id] = record
        try:
            record.channel, record.worker_endpoint = self._channel_factory()
            record.process = self._process_factory(
                target=worker_callable,
                args=(task, options, record.worker_endpoint, parent_identity),
                daemon=False,
            )
            record.process.start()
        except BaseException as err:  # pylint: disable=broad-except
            self._cleanup_spawn_failure(record, loop, err)
            return None
        record.worker_endpoint.close()
        record.worker_endpoint = None
        return record

    def _cleanup_spawn_failure(self: any, record: _OwnedTask, loop: _LoopState, err: BaseException) -> None:
        record.start_failed = True
        if record.worker_endpoint is not None:
            record.worker_endpoint.close()
            record.worker_endpoint = None
        self._cancel_task_tree(record)
        if record.channel is not None:
            record.channel.close()
        self._record_failure(
            record,
            RuntimeError('failed to start prof parallel worker for task {}: {}'.format(record.task.task_id, err)),
            loop,
        )

    def _cancel_bootstrap_tasks(self: any, owned: dict) -> None:
        """停止派发后，取消尚未获得 RUN 的任务（bootstrap 取消）。

        先关闭控制通道让 worker 在等 RUN 时观察到 EOF 自行退出；宽限期内
        未退出的进程再按登记的直接子进程强制回收，保证排空必然收敛。
        """
        now = self._time_fn()
        for record in list(owned.values()):
            if record.reclaimed or record.run_attempted:
                continue
            if not record.cancelled:
                record.cancelled = True
                record.cancel_deadline = now + self.CANCEL_GRACE_SECONDS
                if record.channel is not None and not record.channel.closed:
                    record.channel.close()
                continue
            if (
                record.process is not None
                and record.process.exitcode is None
                and record.cancel_deadline is not None
                and now > record.cancel_deadline
            ):
                self._cancel_task_tree(record)

    @staticmethod
    def _real_linux_process(record):
        return os.name == 'posix' and is_linux() and isinstance(record.process, multiprocessing.process.BaseProcess)

    def _begin_group_probe(self, owned):
        """一轮内共享新鲜快照，正常 RUNNING 路径不触发扫描。"""
        self._group_snapshot = None
        self._snapshot_groups = {
            record.pgid for record in owned.values() if record.pgid is not None and self._real_linux_process(record)
        }
        self._seen_groups = set()

    @staticmethod
    def _verify_group_identity(record):
        try:
            current_identity = _process_start_time(record.pgid)
        except (OSError, ValueError, IndexError) as error:
            raise _TaskTreeProbeError('worker group identity unavailable: {}'.format(record.pgid)) from error
        if current_identity is not None and record.leader_identity != current_identity:
            raise _TaskTreeProbeError('prof worker group identity changed: {}'.format(record.pgid))

    def _scan_task_groups(self, groups):
        """True/False/None 分别表示活动、仅已退出成员、状态未知。"""
        states = {pgid: False for pgid in groups}
        self._seen_groups = set()
        try:
            names = os.listdir('/proc')
        except OSError:
            return {pgid: None for pgid in groups}
        unassigned_unknown = False
        for name in names:
            if not name.isdigit():
                continue
            try:
                pgid = os.getpgid(int(name))  # pylint: disable=no-member
            except ProcessLookupError:
                continue
            except OSError:
                unassigned_unknown = True
                continue
            if pgid not in groups:
                continue  # 无关进程无需读取 stat，避免 hidepid 等权限限制。
            try:
                info = _process_info(int(name))
            except (ProcessLookupError, FileNotFoundError):
                continue
            except (OSError, ValueError, IndexError):
                if states[pgid] is not True:
                    states[pgid] = None
                continue
            if info is None:
                continue
            if info[1] != pgid or info[2] != pgid:
                if states[pgid] is not True:
                    states[pgid] = None
                continue
            self._seen_groups.add(pgid)
            if info[0] != 'Z':
                states[pgid] = True
        if unassigned_unknown:
            states = {pgid: True if value is True else None for pgid, value in states.items()}
        return states

    def _group_alive(self, record):
        if not self._real_linux_process(record) or record.pgid is None:
            return False
        self._verify_group_identity(record)
        try:
            os.killpg(record.pgid, 0)  # pylint: disable=no-member
        except ProcessLookupError:
            return False  # 进程组不存在，独立于快照可直接确认。
        except OSError as error:
            raise _TaskTreeProbeError('worker group inaccessible: {}'.format(record.pgid)) from error
        if self._group_snapshot is None or record.pgid not in self._snapshot_groups:
            self._snapshot_groups.add(record.pgid)
            self._group_snapshot = self._scan_task_groups(self._snapshot_groups)
        alive = self._group_snapshot[record.pgid]
        if alive is None or (not alive and record.pgid not in self._seen_groups):
            try:
                os.killpg(record.pgid, 0)  # pylint: disable=no-member
            except ProcessLookupError:
                return False  # 扫描期间最后一个成员可能刚退出，重新确认整组已消失。
            # 组仍存在却没有可确认的成员，不能把隐藏/变化中的成员当成空树。
            raise _TaskTreeProbeError('worker group members unknown: {}'.format(record.pgid))
        return alive

    def _cancel_all_owned(self, owned):
        """一个记录的探测失败不能阻止其他记录清理；未知状态不释放预算。

        本循环不新增 owned，成功回收才移除；cleanup_started/killed 只从 False 变 True。
        failures 反映本轮探测失败，因此 unresolved 不保证单调。全部剩余记录均已尝试
        KILL 且仍无法确认状态时，移交 _unreclaimed 并抛错，不宣称清理成功。
        可确认仍活跃的任务则继续等待退出；该循环不保证内核进程在固定时间内退出。
        """
        failures = {}
        while owned:
            self._begin_group_probe(owned)
            for record in list(owned.values()):
                if self._has_no_child(record):
                    self._reclaim(record, owned)
                    continue
                failures.pop(record.task.task_id, None)
                try:
                    self._cancel_task_tree(record)
                    if record.process.exitcode is not None and not self._group_alive(record):
                        self._reclaim(record, owned)
                        failures.pop(record.task.task_id, None)
                except (OSError, _TaskTreeProbeError) as error:
                    failures[record.task.task_id] = error
                    # 直接子进程由 Process 对象持有，不依赖全机 proc 可见性。
                    if record.process.exitcode is None:
                        try:
                            if record.cleanup_killed:
                                record.process.kill()
                            else:
                                record.process.terminate()
                        except (OSError, AssertionError) as direct_error:
                            failures[record.task.task_id] = direct_error
            unresolved = [
                record for record in owned.values() if record.task.task_id in failures and record.cleanup_killed
            ]
            if owned and len(unresolved) == len(owned):
                self._unreclaimed.extend(unresolved)
                for record in unresolved:
                    if record.channel is not None:
                        record.channel.close()
                    # 后代状态未知不等于直接子进程仍活着；已退出组长的句柄仍可关闭。
                    if record.process.exitcode is not None:
                        try:
                            record.process.join()
                            record.process.close()
                        except (OSError, ValueError, AssertionError):
                            pass
                raise _TaskTreeProbeError(
                    'PROF cleanup could not verify tasks {}: {}'.format(
                        sorted(owned), '; '.join(str(failures[key]) for key in sorted(owned))
                    )
                )
            if owned:
                self._sleep_fn(self._CLEANUP_POLL_SECONDS)

    def _cancel_task_tree(self, record):
        """非阻塞推进取消；组长退出不能作为后代全部退出的依据。"""
        if self._has_no_child(record):
            return
        now = self._time_fn()
        if not record.cleanup_started:
            record.cleanup_started = True
            record.cancel_deadline = now + self.CANCEL_GRACE_SECONDS
            first_signal = self._interrupt_signal if self._interrupted else signal.SIGTERM
            self._signal_owned(record, first_signal or signal.SIGTERM)
        elif now >= record.cancel_deadline and not record.cleanup_killed:
            record.cleanup_killed = True
            self._signal_owned(record, _KILL_SIGNAL)

    def _signal_owned(self, record, signum):
        if self._real_linux_process(record) and record.pgid is not None:
            # READY 已验证归属；发送信号只需复核身份，不必先全量扫描存活成员。
            self._verify_group_identity(record)
            try:
                os.killpg(record.pgid, signum)  # pylint: disable=no-member
            except ProcessLookupError:
                pass
            except OSError as error:
                raise _TaskTreeProbeError('failed to signal worker group {}'.format(record.pgid)) from error
        elif record.process is not None and record.process.exitcode is None:
            try:
                if signum == signal.SIGINT and self._real_linux_process(record):
                    os.kill(record.process.pid, signum)
                elif signum == signal.SIGTERM:
                    record.process.terminate()
                else:
                    record.process.kill()
            except (AttributeError, ProcessLookupError, AssertionError):
                pass

    def _reclaim(self: any, record: _OwnedTask, owned: dict) -> None:
        record.reclaimed = True
        owned.pop(record.task.task_id, None)
        if record.process is not None:
            try:
                if not self._has_no_child(record):
                    record.process.join()
            except Exception:  # pylint: disable=broad-except
                logging.warning('Failed to join PROF worker %s.', record.task.task_id, exc_info=True)
            close = getattr(record.process, 'close', None)
            if close is not None:
                try:
                    close()
                except Exception:  # pylint: disable=broad-except
                    logging.warning('Failed to close PROF worker %s.', record.task.task_id, exc_info=True)
        if record.channel is not None:
            record.channel.close()

    @staticmethod
    def _running_count(owned: dict) -> int:
        return sum(1 for record in owned.values() if record.run_attempted and not record.reclaimed)

    def _record_exception(self: any, record: _OwnedTask, payload: dict, loop: _LoopState) -> None:
        from msinterface.prof_task_executor import rebuild_exception

        exception = rebuild_exception(payload)
        self._record_failure(record, exception, loop)

    def _record_failure(self: any, record: _OwnedTask, error: BaseException, loop: _LoopState) -> None:
        if loop.first_exception is None or record.task.task_id < loop.first_exception[0]:
            loop.first_exception = (record.task.task_id, error)

    def _wait_slice(self: any, owned: dict) -> None:
        fds = []
        for record in owned.values():
            if record.reclaimed:
                continue
            if self._has_no_child(record):
                continue
            if record.channel is not None and not record.channel.closed:
                try:
                    # EOF 已消费、或组长已退出后，不让永久可读端点驱动忙循环。
                    if record.process.exitcode is None and not getattr(record.channel, '_eof', False):
                        fds.append(record.channel.fileno())
                except (AttributeError, OSError):
                    pass
            sentinel = getattr(record.process, 'sentinel', None) if record.process.exitcode is None else None
            if sentinel is not None:
                fds.append(sentinel)
        if not fds:
            self._sleep_fn(self._CLEANUP_POLL_SECONDS)
            return
        timeout = self._WAIT_SLICE_SECONDS
        now = self._time_fn()
        for record in owned.values():
            if record.reclaimed or record.run_attempted or record.bootstrap_deadline is None:
                continue
            timeout = min(timeout, max(0.0, record.bootstrap_deadline - now))
        try:
            _poll_fds(fds, select.POLLIN, timeout)  # pylint: disable=no-member
        except (OSError, ValueError):
            self._sleep_fn(self._IDLE_SLEEP_SECONDS)

    # ------------------------------------------------------------------
    # 中断：父进程捕获后停止派发并按组转发，worker 不永久忽略取消信号。
    # ------------------------------------------------------------------
    def _install_signal_handlers(self: any) -> dict:
        installed = {}
        for sig in (signal.SIGINT, signal.SIGTERM):
            try:
                installed[sig] = signal.signal(sig, self._on_signal)
            except (ValueError, OSError):
                continue
        return installed

    def _restore_signal_handlers(self: any, installed: dict) -> None:
        for sig, handler in installed.items():
            try:
                signal.signal(sig, handler)
            except (ValueError, OSError):
                continue

    def _on_signal(self: any, signum, _frame) -> None:
        self._interrupted = True
        self._interrupt_signal = signum


def _process_info(pid):
    try:
        with open('/proc/{}/stat'.format(pid), 'r', encoding='utf-8') as reader:
            fields = reader.read().rsplit(')', 1)[1].split()
        return fields[0], int(fields[2]), int(fields[3]), int(fields[19])
    except (FileNotFoundError, ProcessLookupError):
        return None
    except (ValueError, IndexError) as error:
        raise OSError('Malformed /proc/{}/stat'.format(pid)) from error


def _process_start_time(pid):
    if os.name != 'posix' or not is_linux():
        return None
    info = _process_info(pid)
    return info[3] if info else None


def _context_eligibility():
    # 显式 fork 外层 worker；标准库在子进程 bootstrap 时同步其默认上下文为 fork，
    # 使内部进程继承 worker 的 session，不改变父进程的 forkserver/spawn 默认值。
    if os.name != 'posix' or not is_linux():
        return ('PROCESS_CONTEXT_UNSUPPORTED',)
    if 'fork' not in multiprocessing.get_all_start_methods():
        return ('PROCESS_CONTEXT_UNSUPPORTED',)
    if 'msprof_analysis' in sys.modules or len(os.listdir('/proc/self/task')) != 1:
        return ('FORK_CONTEXT_UNSAFE',)
    try:
        parent, worker = socket.socketpair(socket.AF_UNIX, socket.SOCK_SEQPACKET)  # pylint: disable=no-member
        with parent, worker:
            parent.setblocking(False)
            worker.setblocking(False)
    except (AttributeError, OSError):
        return ('IPC_UNAVAILABLE',)
    return ()


def _check_fd_headroom(workers):
    """初始准入覆盖父进程全部槽位的常驻及创建峰值 FD；不复核运行期资源。"""
    try:
        import resource

        open_fds = len(os.listdir('/proc/self/fd'))
        soft_limit = resource.getrlimit(resource.RLIMIT_NOFILE)[0]
    except (ImportError, OSError, ValueError):
        return False, 'FD_LIMIT_UNKNOWN'
    if soft_limit not in (resource.RLIM_INFINITY, -1) and open_fds + workers * _PARENT_FDS_PER_WORKER > soft_limit:
        return False, 'FD_HEADROOM_INSUFFICIENT'
    return True, None


def schedule_prof_tasks(
    tasks: list, options: CommandOptions, serial_runner, valid_count: int = None, pending_count: int = None
) -> None:
    """
    父进程编排：一次资源预筛 → 按需统计输入大小 → 固定计划及 FD 准入 → 调度。

    单任务直接走串行，不做资源探测；串行回调继续使用父进程的原命令实例。
    """
    if not tasks:
        return
    eligibility_reasons = ()
    snapshot = ResourceSnapshot(None, None, ())
    if len(tasks) > 1:
        try:
            eligibility_reasons = _context_eligibility()
        except OSError:
            eligibility_reasons = ('PROCESS_CONTEXT_UNKNOWN',)
        if not eligibility_reasons:
            snapshot = SystemResourceProbe().probe()
            if (
                not snapshot.unknown_reasons
                and snapshot.available_ram_bytes is not None
                and snapshot.effective_cpu is not None
            ):
                if snapshot.effective_cpu < 2 * PROF_CPU_CORES:
                    eligibility_reasons = (REASON_PARALLEL_BUDGET_LT_2,)
                else:
                    tasks = _prepare_parallel_tasks(tasks)
    plan = plan_prof_tasks(tasks, options, snapshot, eligibility_reasons)
    if plan.mode == 'PARALLEL':
        allowed, reason = _check_fd_headroom(plan.max_workers)
        if not allowed:
            plan = ExecutionPlan('SERIAL', 1, (reason,))
    ProfTaskScheduler().run(tasks, options, plan, serial_runner, valid_count=valid_count, pending_count=pending_count)
