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
import json
import multiprocessing
import os
import signal
import socket
import sys
import tempfile
import time
import unittest
from collections import deque
from dataclasses import replace
from fractions import Fraction
from unittest import mock

from common_func.msprof_exception import ProfException
from msinterface.prof_parallel_plan import (
    CommandOptions, ExecutionPlan, ProfTask, ResourceSnapshot,
)
from msinterface.prof_parallel_scheduler import (
    ChannelProtocolError,
    ProfTaskScheduler,
    _FORK_GUARD_ENDPOINTS,
    _OwnedTask,
    _ProfWorkerChannel,
    _check_fd_headroom,
    _context_eligibility,
    schedule_prof_tasks,
)

NAMESPACE = 'msinterface.prof_parallel_scheduler'


def build_options():
    return CommandOptions(
        input_root='/root', command='import', command_type=None, cluster_flag=False,
        export_mode=None, model_id=None, iteration_id=None, iteration_count=1,
        export_format=None, reports_path='', clear_mode=False,
    )


def make_task(task_id):
    return ProfTask(
        task_id=task_id,
        prof_path='/root/PROF_%d' % task_id,
        host_path='',
        device_paths=('d0',),
        all_result_paths=('d0',),
        raw_bytes=100,
        is_cluster=True,
    )


def make_plan(mode='PARALLEL', workers=2):
    return ExecutionPlan(mode, workers, ())


def ready(task_id, pid):
    return {'type': 'READY', 'version': 1, 'task_id': task_id, 'pid': pid, 'pgid': None}


def returned(task_id, _pid):
    return {'type': 'RETURNED', 'version': 1, 'task_id': task_id}


def raised(task_id, _pid, code):
    return {'type': 'RAISED', 'version': 1, 'task_id': task_id,
            'exception': {'kind': 'ProfException', 'code': code, 'message': 'x', 'callback': 'error'}}


def exit_with(messages, process):
    """脚本步进：发送指定消息并让进程随后退出。"""

    def _step():
        process.exitcode = 0
        return list(messages)

    return _step


class FakeChannel:
    """队列驱动的通道替身：按脚本产出消息，记录发送内容。"""

    def __init__(self, script=(), on_close=None):
        self._script = list(script)
        self.sent = []
        self._closed = False
        self.on_close = on_close

    def pump(self):
        if self._script:
            item = self._script.pop(0)
            return item() if callable(item) else list(item)
        return []

    def send_message(self, message, timeout=0.0):
        if self._closed:
            raise ChannelProtocolError('fake channel closed')
        self.sent.append(message)

    @property
    def closed(self):
        return self._closed

    def close(self):
        if not self._closed:
            self._closed = True
            if self.on_close is not None:
                self.on_close()

    def fileno(self):
        raise OSError('fake channel has no fileno')


class FakeProcess:
    """脚本化进程替身：信号调用记录与退出状态。"""

    _next_pid = [40000]

    def __init__(self):
        FakeProcess._next_pid[0] += 1
        self.pid = FakeProcess._next_pid[0]
        self.exitcode = None
        self.sentinel = None
        self.daemon = None
        self.terminated = self.killed = self.joined = self.close_count = 0
        self.spawn_args = None

    def start(self):
        return None

    def terminate(self):
        self.terminated += 1
        self.exitcode = 0

    def kill(self):
        self.killed += 1
        self.exitcode = -9

    def join(self, timeout=None):
        self.joined += 1

    def close(self):
        self.close_count += 1


@unittest.skipUnless(sys.platform.startswith('linux'), '生产 IPC 仅 Linux')
class TestProfWorkerChannel(unittest.TestCase):
    """通过真实 packet 验证传输边界，不扩展生产平台。"""

    def setUp(self):
        parent, worker = socket.socketpair(socket.AF_UNIX, socket.SOCK_SEQPACKET)
        self.parent = _ProfWorkerChannel(parent)
        self.worker = _ProfWorkerChannel(worker)
        self.addCleanup(self.parent.close)
        self.addCleanup(self.worker.close)

    def test_pump_should_reject_invalid_packets(self):
        for payload in (b'\xff', b'{', b'[]', b'x' * 65537):
            with self.subTest(payload_size=len(payload)):
                self.worker._conn.send(payload)
                with self.assertRaises(ChannelProtocolError):
                    self.parent.pump()

    def test_packet_limit_eof_and_bounded_send_failures(self):
        message = {'x': 'x' * (65536 - len(json.dumps({'x': ''}).encode()))}
        self.worker.send_message(message)
        self.assertEqual(self.parent.pump(), [message])
        self.assertEqual(self.parent.pump(), [])
        with self.assertRaises(ChannelProtocolError):
            self.worker.send_message({'x': message['x'] + 'x'})
        self.worker.close()
        with self.assertRaises(EOFError):
            self.parent.recv_blocking(timeout=0.1)
        endpoint = mock.Mock()
        channel = _ProfWorkerChannel(endpoint)
        try:
            for error in (BlockingIOError(), OSError('send failed'), None):
                endpoint.send.side_effect, endpoint.send.return_value = error, 0
                with self.assertRaises(ChannelProtocolError):
                    channel.send_message({}, timeout=0)
            endpoint.fileno.return_value = self.parent.fileno()
            endpoint.send.side_effect = [BlockingIOError(), 2]
            channel.send_message({}, timeout=0.1)
        finally:
            channel.close()

    def test_run_dispatch_recovers_from_transient_send_block(self):
        from msinterface.prof_parallel_scheduler import _LoopState
        record = _OwnedTask(make_task(0), 300)
        record.state, record.channel = 'READY', self.parent
        endpoint = mock.Mock(wraps=self.parent._conn)
        real_send = self.parent._conn.send

        def send(payload):
            if endpoint.send.call_count == 1:
                raise BlockingIOError()
            return real_send(payload)

        endpoint.send.side_effect = send
        loop = _LoopState()
        with mock.patch.object(self.parent, '_conn', endpoint):
            self.assertEqual(ProfTaskScheduler()._dispatch_run({0: record}, loop), 1)
        self.assertEqual(self.worker.pump(), [{'type': 'RUN', 'version': 1, 'task_id': 0}])
        self.assertEqual(record.state, 'RUNNING')
        self.assertFalse(record.cleanup_started)
        self.assertIsNone(loop.first_exception)

    def test_factory_failure_closes_both_endpoints(self):
        parent, worker = socket.socketpair(socket.AF_UNIX, socket.SOCK_SEQPACKET)
        with mock.patch(NAMESPACE + '.socket.socketpair', return_value=(parent, worker)), \
                mock.patch(NAMESPACE + '._ProfWorkerChannel', side_effect=OSError('cannot configure')):
            with self.assertRaises(OSError):
                ProfTaskScheduler._default_channel_factory()
        self.assertEqual((parent.fileno(), worker.fileno()), (-1, -1))

    def test_bootstrap_parent_close_exits_real_worker_without_run(self):
        from msinterface.prof_task_executor import run_prof_task
        parent, worker = ProfTaskScheduler._default_channel_factory()
        process = multiprocessing.Process(target=run_prof_task, args=(make_task(0), build_options(), worker))
        try:
            process.start()
            worker.close()
            self.assertEqual(parent.recv_blocking(timeout=3)['type'], 'READY')
            parent.close()
            process.join(timeout=3)
            self.assertEqual(process.exitcode, 0)
        finally:
            parent.close()
            worker.close()
            if process.is_alive():
                process.kill()
                process.join()
            process.close()

    def test_high_fd_worker_handshake_and_scheduler_wait(self):
        import fcntl
        import resource
        from msinterface.prof_task_executor import run_prof_task
        soft_limit = resource.getrlimit(resource.RLIMIT_NOFILE)[0]
        if soft_limit != resource.RLIM_INFINITY and soft_limit < 1040:
            self.skipTest('高 FD 回归需要 RLIMIT_NOFILE >=1040')
        endpoints = socket.socketpair(socket.AF_UNIX, socket.SOCK_SEQPACKET)
        try:
            parent_socket, worker = [socket.socket(fileno=fcntl.fcntl(
                endpoint.fileno(), fcntl.F_DUPFD_CLOEXEC, 1024)) for endpoint in endpoints]
        finally:
            for endpoint in endpoints:
                endpoint.close()
        parent = _ProfWorkerChannel(parent_socket)
        with mock.patch('msinterface.prof_task_executor.execute_prof_task'):
            process = multiprocessing.get_context('fork').Process(
                target=run_prof_task, args=(make_task(0), build_options(), worker))
            try:
                process.start()
                worker.close()
                self.assertEqual(parent.recv_blocking(timeout=3)['type'], 'READY')
                record = _OwnedTask(make_task(0), time.monotonic() + 3)
                record.process, record.channel = process, parent
                scheduler = ProfTaskScheduler(sleep_fn=mock.Mock(side_effect=AssertionError('等待不应退化轮询')))
                scheduler._WAIT_SLICE_SECONDS = 0.01
                scheduler._wait_slice({0: record})
                parent.send_message({'type': 'RUN', 'task_id': 0, 'version': 1}, timeout=0)
                self.assertEqual(parent.recv_blocking(timeout=3)['type'], 'RETURNED')
                process.join(timeout=3)
                self.assertEqual(process.exitcode, 0)
            finally:
                parent.close()
                worker.close()
                if process.is_alive():
                    process.kill()
                    process.join()
                process.close()


class TestProfTaskScheduler(unittest.TestCase):
    @unittest.skipUnless(sys.platform.startswith('linux'), '父进程 FD 预留仅 Linux')
    def test_initial_fd_headroom_covers_all_worker_slots(self):
        for slots, limit, allowed in ((2, 12, False), (2, 20, True), (8, 67, False), (8, 68, True)):
            with self.subTest(slots=slots, limit=limit), \
                    mock.patch(NAMESPACE + '.os.listdir', return_value=['0', '1', '2', '3']), \
                    mock.patch('resource.getrlimit', return_value=(limit, 1024)):
                self.assertEqual(_check_fd_headroom(slots),
                                 (True, None) if allowed else (False, 'FD_HEADROOM_INSUFFICIENT'))

    def test_process_info_distinguishes_malformed_unreadable_and_gone(self):
        from io import StringIO
        from msinterface.prof_parallel_scheduler import _process_info
        fields = ['S', '1', '50', '50'] + ['0'] * 15 + ['100']
        valid = '50 (command with ) bracket) ' + ' '.join(fields)
        with mock.patch('builtins.open', return_value=StringIO(valid)):
            self.assertEqual(_process_info(50), ('S', 50, 50, 100))
        for content in ('', 'bad', '50 (name) S 1', valid.rsplit(' ', 1)[0] + ' invalid'):
            with self.subTest(content=content), mock.patch('builtins.open', return_value=StringIO(content)):
                with self.assertRaisesRegex(OSError, 'Malformed'):
                    _process_info(50)
        with mock.patch('builtins.open', side_effect=PermissionError('denied')):
            with self.assertRaises(PermissionError):
                _process_info(50)
        with mock.patch('builtins.open', side_effect=FileNotFoundError()):
            self.assertIsNone(_process_info(50))

    def test_run_dispatch_and_refill_stop_on_cancellation_or_failure(self):
        from msinterface.prof_parallel_scheduler import _LoopState
        for outcome in ('cancelled', 'existing_failure', 'send_failure'):
            with self.subTest(outcome=outcome):
                record = _OwnedTask(make_task(0), 300)
                record.state, record.process, record.channel = 'READY', FakeProcess(), FakeChannel()
                scheduler, loop = ProfTaskScheduler(), _LoopState()
                if outcome == 'cancelled':
                    scheduler._on_signal(signal.SIGINT, None)
                elif outcome == 'existing_failure':
                    loop.first_exception = (0, RuntimeError('earlier worker failed'))
                else:
                    record.channel.send_message = mock.Mock(side_effect=ChannelProtocolError('send failed'))
                scheduler._dispatch_run({0: record}, loop)
                self.assertEqual(record.run_attempted, outcome == 'send_failure')
                self.assertEqual(record.cleanup_started, outcome == 'send_failure')
                self.assertEqual(record.channel.sent, [])
                if outcome != 'cancelled':
                    self.assertIsNotNone(loop.first_exception)
                pending = deque([make_task(1)])
                with mock.patch.object(scheduler, '_spawn') as spawn:
                    scheduler._spawn_pending(pending, {0: record}, make_plan(), build_options(), None, None, loop)
                spawn.assert_not_called()
                self.assertEqual(len(pending), 1)

    def make_scheduler(self, processes, channels, sleep_fn=None):
        pending_processes = list(processes)
        pending_channels = list(channels)

        def process_factory(target, args, daemon):
            process = pending_processes.pop(0)
            process.spawn_args = args
            process.daemon = daemon
            return process

        return ProfTaskScheduler(
            process_factory=process_factory,
            channel_factory=lambda: (pending_channels.pop(0), mock.Mock(name='worker_end')),
            sleep_fn=sleep_fn)

    def test_raised_task_with_smallest_id_should_win_even_when_later(self):
        proc0, proc1 = FakeProcess(), FakeProcess()
        result0 = FakeChannel(script=[[ready(0, proc0.pid)], [],
                                      exit_with([raised(0, proc0.pid, 7)], proc0)])
        result1 = FakeChannel(script=[[ready(1, proc1.pid)],
                                      exit_with([raised(1, proc1.pid, 5)], proc1)])
        scheduler = self.make_scheduler([proc0, proc1], [result0, result1])
        with self.assertRaises(ProfException) as context:
            scheduler.run([make_task(0), make_task(1)], build_options(), make_plan(), mock.Mock())
        # task 1 的异常先到，但 task 0 编号更小，应抛出 task 0 的异常。
        self.assertEqual(context.exception.code, 7)

    def test_eof_without_terminal_cleans_alive_worker_but_not_cancel_or_success(self):
        from msinterface.prof_parallel_scheduler import _LoopState
        for outcome in ('STARTING', 'RUNNING', 'cancelled', 'returned'):
            with self.subTest(outcome=outcome):
                record = _OwnedTask(make_task(0), 300)
                record.process = FakeProcess()
                record.run_attempted = outcome != 'STARTING'
                record.cancelled = outcome == 'cancelled'
                record.channel = FakeChannel(
                    [[returned(0, record.process.pid)]] if outcome == 'returned' else [])
                record.channel._eof = True
                scheduler, loop = ProfTaskScheduler(), _LoopState()
                scheduler._pump_channels({0: record}, loop)
                failed = outcome in ('STARTING', 'RUNNING')
                self.assertEqual(record.cleanup_started, failed)
                self.assertEqual(loop.first_exception is not None, failed)
                self.assertEqual(record.process.terminated, int(failed))


    def test_fixed_slots_refill_without_runtime_resource_probe(self):
        processes = [FakeProcess() for _ in range(5)]
        channels = [FakeChannel([[ready(i, p.pid)], exit_with([returned(i, p.pid)], p)])
                    for i, p in enumerate(processes)]
        scheduler = self.make_scheduler(processes, channels)
        factory = scheduler._process_factory
        def tracked_factory(**kwargs):
            process = factory(**kwargs)
            self.assertLessEqual(sum(p.spawn_args is not None and not p.close_count for p in processes), 2)
            return process
        scheduler._process_factory = tracked_factory
        runner = mock.Mock()
        with mock.patch(NAMESPACE + '.SystemResourceProbe', side_effect=AssertionError('运行期间不应探测资源')):
            scheduler.run([make_task(i) for i in range(5)], build_options(), make_plan(), runner)
        runner.assert_not_called()
        self.assertEqual([p.close_count for p in processes], [1] * 5)
        self.assertEqual([[m['type'] for m in channel.sent] for channel in channels], [['RUN']] * 5)

    def test_last_refill_start_failure_cannot_return_success(self):
        class FailingProcess(FakeProcess):
            def start(self):
                raise OSError('last refill failed')
        processes = [FakeProcess(), FakeProcess(), FailingProcess()]
        pairs = [FakeChannel([[ready(i, p.pid)], exit_with([returned(i, p.pid)], p)])
                 for i, p in enumerate(processes[:2])]
        pairs.append(FakeChannel())
        scheduler = self.make_scheduler(processes, pairs)
        with self.assertRaisesRegex(RuntimeError, 'last refill failed'):
            scheduler.run([make_task(i) for i in range(3)], build_options(), make_plan(), mock.Mock())
        self.assertTrue(pairs[-1].closed)


    def test_cancel_running_workers_does_not_wait_for_normal_completion(self):
        import signal
        p0, p1 = FakeProcess(), FakeProcess()
        holder = []
        def interrupt():
            holder[0]._on_signal(signal.SIGTERM, None)
            return []
        pairs = [FakeChannel([[ready(0, p0.pid)], interrupt]), FakeChannel([[ready(1, p1.pid)]])]
        scheduler = self.make_scheduler([p0, p1], pairs)
        holder.append(scheduler)
        waits = [0]
        def watchdog(_owned):
            waits[0] += 1
            if waits[0] > 10:
                raise AssertionError('cancellation did not finish')
        with mock.patch.object(scheduler, '_wait_slice', side_effect=watchdog):
            with self.assertRaises(KeyboardInterrupt):
                scheduler.run([make_task(0), make_task(1)], build_options(), make_plan(), mock.Mock())
        self.assertEqual([p0.terminated, p1.terminated], [1, 1])


    def test_real_unstarted_process_failure_finishes_cleanup(self):
        process = multiprocessing.get_context().Process(target=int)
        channel = FakeChannel()
        waits = []
        def watchdog(_delay):
            waits.append(1)
            if len(waits) > 2:
                raise AssertionError('cleanup cannot reclaim unstarted Process')
        scheduler = ProfTaskScheduler(
            process_factory=lambda **_kwargs: process,
            channel_factory=lambda: (channel, mock.Mock()),
            sleep_fn=watchdog,
        )
        with mock.patch.object(process, 'start', side_effect=OSError('fork unavailable')):
            with self.assertRaisesRegex(RuntimeError, 'failed to start'):
                scheduler._run_parallel([make_task(0)], build_options(), make_plan(), int)
        self.assertTrue(channel.closed)
        self.assertTrue(process._closed)


    @staticmethod
    def make_owned_record(task_id, pgid):
        record = _OwnedTask(make_task(task_id), 300)
        record.pgid = pgid
        record.leader_identity = 100
        return record

    def test_one_failed_cleanup_does_not_skip_other_owned_tasks(self):
        first, second = self.make_owned_record(0, 50), self.make_owned_record(1, 60)
        first.process, second.process = FakeProcess(), FakeProcess()
        owned = {0: first, 1: second}
        clock = [0]
        scheduler = ProfTaskScheduler(time_fn=lambda: clock[0],
                                      sleep_fn=lambda _delay: clock.__setitem__(0, clock[0] + 1))
        scheduler.CANCEL_GRACE_SECONDS = 1
        from msinterface.prof_parallel_scheduler import _TaskTreeProbeError
        def probe(record):
            if record is first:
                raise _TaskTreeProbeError('owned group unreadable')
            return False
        with mock.patch.object(scheduler, '_group_alive', side_effect=probe):
            with self.assertRaisesRegex(_TaskTreeProbeError, 'could not verify tasks'):
                scheduler._cancel_all_owned(owned)
        self.assertTrue(second.reclaimed)
        self.assertEqual(second.process.joined, 1)
        self.assertFalse(first.reclaimed)
        self.assertEqual(list(owned), [0])
        self.assertEqual(first.process.close_count, 1)

    def test_running_tasks_do_not_scan_proc(self):
        from msinterface.prof_parallel_scheduler import _LoopState
        record = self.make_owned_record(0, 50)
        record.process = FakeProcess()
        record.run_attempted = True
        scheduler = ProfTaskScheduler()
        with mock.patch.object(scheduler, '_scan_task_groups') as scan:
            scheduler._reap_exited({0: record}, _LoopState())
        scan.assert_not_called()

    @unittest.skipUnless(sys.platform.startswith('linux'), 'proc 语义仅 Linux')
    def test_multiple_groups_share_one_scan_per_cleanup_round(self):
        first, second = self.make_owned_record(0, 50), self.make_owned_record(1, 60)
        scheduler = ProfTaskScheduler()
        with mock.patch.object(scheduler, '_real_linux_process', return_value=True), \
                mock.patch(NAMESPACE + '._process_start_time', return_value=100), \
                mock.patch(NAMESPACE + '.os.listdir', return_value=['50', '60']) as listing, \
                mock.patch(NAMESPACE + '.os.getpgid', side_effect=lambda pid: pid), \
                mock.patch(NAMESPACE + '.os.killpg'), \
                mock.patch(NAMESPACE + '._process_info', side_effect=lambda pid: ('Z', pid, pid, 100)):
            scheduler._begin_group_probe({0: first, 1: second})
            self.assertFalse(scheduler._group_alive(first))
            self.assertFalse(scheduler._group_alive(second))
            listing.assert_called_once_with('/proc')
            scheduler._begin_group_probe({0: first, 1: second})
            self.assertFalse(scheduler._group_alive(first))
            self.assertEqual(listing.call_count, 2)

    @unittest.skipUnless(sys.platform.startswith('linux'), 'proc 语义仅 Linux')
    def test_owned_member_permission_error_is_unknown_not_empty(self):
        from msinterface.prof_parallel_scheduler import _TaskTreeProbeError
        record = self.make_owned_record(0, 50)
        scheduler = ProfTaskScheduler()
        with mock.patch.object(scheduler, '_real_linux_process', return_value=True), \
                mock.patch(NAMESPACE + '._process_start_time', return_value=100), \
                mock.patch(NAMESPACE + '.os.listdir', return_value=['50']), \
                mock.patch(NAMESPACE + '.os.getpgid', return_value=50), \
                mock.patch(NAMESPACE + '.os.killpg') as killpg, \
                mock.patch(NAMESPACE + '._process_info', side_effect=PermissionError('owned stat unreadable')):
            with self.assertRaisesRegex(_TaskTreeProbeError, 'members unknown'):
                scheduler._group_alive(record)
            killpg.side_effect = [None, ProcessLookupError()]
            self.assertFalse(scheduler._group_alive(record))


    @unittest.skipUnless(sys.platform.startswith('linux'), 'proc 语义仅 Linux')
    def test_unrelated_permission_error_does_not_block_group_probe(self):
        record = _OwnedTask(make_task(0), 300)
        record.pgid = 50
        record.leader_identity = 100
        scheduler = ProfTaskScheduler()
        def info(pid):
            if pid == 1:
                raise PermissionError('unrelated /proc/1/stat')
            return ('Z', 50, 50, 100)
        with mock.patch.object(scheduler, '_real_linux_process', return_value=True), \
                mock.patch(NAMESPACE + '._process_start_time', return_value=100), \
                mock.patch(NAMESPACE + '.os.listdir', return_value=['1', '50']), \
                mock.patch(NAMESPACE + '.os.getpgid', side_effect=lambda pid: pid), \
                mock.patch(NAMESPACE + '.os.killpg'), \
                mock.patch(NAMESPACE + '._process_info', side_effect=info):
            self.assertFalse(scheduler._group_alive(record))


class TestScheduleProfTasksFacade(unittest.TestCase):
    def test_serial_preflight_skips_parallel_preparation(self):
        for count, context, snapshot in (
                (1, (), ResourceSnapshot(None, None)),
                (2, ('PROCESS_CONTEXT_UNSUPPORTED',), ResourceSnapshot(None, None)),
                (2, (), ResourceSnapshot(None, None, ('MEMORY_UNKNOWN',))),
                (2, (), ResourceSnapshot(1 << 40, Fraction(127, 2)))):
            with self.subTest(count=count, context=context, snapshot=snapshot):
                tasks, runner = [make_task(i) for i in range(count)], mock.Mock()
                with mock.patch(NAMESPACE + '._context_eligibility', return_value=context), \
                        mock.patch(NAMESPACE + '.SystemResourceProbe') as probe, \
                        mock.patch(NAMESPACE + '._prepare_parallel_tasks') as prepare:
                    probe.return_value.probe.return_value = snapshot
                    schedule_prof_tasks(tasks, build_options(), runner)
                prepare.assert_not_called()
                self.assertEqual(probe.call_count, int(count > 1 and not context))
                self.assertEqual(runner.call_args_list, [mock.call(task) for task in tasks])

    def test_initial_fd_failure_uses_one_snapshot_and_original_serial_order(self):
        tasks, runner = [make_task(i) for i in range(3)], mock.Mock()
        with mock.patch(NAMESPACE + '._context_eligibility', return_value=()), \
                mock.patch(NAMESPACE + '.SystemResourceProbe') as probe, \
                mock.patch(NAMESPACE + '._prepare_parallel_tasks', return_value=tasks), \
                mock.patch(NAMESPACE + '.plan_prof_tasks', return_value=make_plan()), \
                mock.patch(NAMESPACE + '._check_fd_headroom', return_value=(False, 'FD_LIMIT_UNKNOWN')) as fd:
            probe.return_value.probe.return_value = ResourceSnapshot(1 << 40, Fraction(256))
            schedule_prof_tasks(tasks, build_options(), runner)
        probe.return_value.probe.assert_called_once_with()
        fd.assert_called_once_with(2)
        self.assertEqual(runner.call_args_list, [mock.call(task) for task in tasks])


IS_LINUX = os.name == 'posix' and sys.platform.startswith('linux')

# fork 继承的通信全局量：测试主进程在 run() 前设置。
_MARKER_DIR = None
_OVERLAP_BARRIER = None
_IGNORE_RAISED_CHILD_TERM = False
_FAIL_TERMINAL = False
_FAILURE_EVENTS = None
_CHILD_PID = None


def _raised_worker_child():
    if _IGNORE_RAISED_CHILD_TERM:
        signal.signal(signal.SIGTERM, signal.SIG_IGN)
    _CHILD_PID.value = os.getpid()
    _FAILURE_EVENTS[0].set()
    while True:
        time.sleep(0.05)


def _execute_raised_and_healthy(task, _options):
    if task.task_id == 0:
        child = multiprocessing.Process(target=_raised_worker_child)
        child.start()
        if not _FAILURE_EVENTS[0].wait(5) or not _FAILURE_EVENTS[1].wait(5):
            raise AssertionError('real failure test did not start both branches')
        if _FAIL_TERMINAL:
            raise SystemExit('x' * 70000)
        raise ProfException(7, 'original worker exception')
    _FAILURE_EVENTS[1].set()
    if not _FAILURE_EVENTS[2].wait(5):
        raise AssertionError('failed tree was not reclaimed')
    _FAILURE_EVENTS[3].set()


def _inner_child(inner_dir, task_id, inherited, expected_pgid):
    assert os.getpgid(0) == expected_pgid
    assert not _FORK_GUARD_ENDPOINTS, '业务后代继承了协议端点'
    fd = os.open(os.devnull, os.O_RDONLY)
    try:
        for endpoint in inherited:
            assert endpoint.fileno() == -1
            endpoint.close()
        os.fstat(fd)  # 重复关闭继承对象不得误关复用的 FD。
    finally:
        os.close(fd)
    with open(os.path.join(inner_dir, 'inner_%d' % task_id), 'w') as handler:
        handler.write('ok')


def _execute_nested(task, _options):
    """只替换业务执行，握手和异常回传继续使用生产 worker。"""
    assert multiprocessing.get_start_method() == 'fork'
    inner = multiprocessing.Process(
        target=_inner_child, args=(_MARKER_DIR, task.task_id, tuple(_FORK_GUARD_ENDPOINTS), os.getpgid(0)))
    inner.start()
    inner.join(timeout=10)
    if inner.is_alive():
        inner.kill()
        inner.join()
        raise AssertionError('nested child did not finish')
    if inner.exitcode != 0:
        raise AssertionError('nested child failed')
    inner.close()
    _OVERLAP_BARRIER.wait(timeout=10)


@unittest.skipUnless(IS_LINUX, '真实进程 IPC 测试仅 Linux（生产并行平台）开放')
class TestProfParallelRealProcessIpc(unittest.TestCase):
    TEST_WATCHDOG_SECONDS = 240

    def setUp(self):
        global _MARKER_DIR
        self.assertFalse(_FORK_GUARD_ENDPOINTS, '前一用例未关闭调度端点')
        temp_dir = tempfile.TemporaryDirectory(dir=os.path.dirname(__file__))
        self.addCleanup(temp_dir.cleanup)
        self.root = temp_dir.name
        _MARKER_DIR = os.path.join(self.root, 'markers')
        os.makedirs(_MARKER_DIR)
        self.serial_calls = []
        self._old_alarm_handler = signal.signal(
            signal.SIGALRM,
            lambda *_args: (_ for _ in ()).throw(TimeoutError('IPC 看门狗超时：调度疑似卡死')))
        signal.alarm(self.TEST_WATCHDOG_SECONDS)

    def tearDown(self):
        signal.alarm(0)
        signal.signal(signal.SIGALRM, self._old_alarm_handler)
        self.assertFalse(_FORK_GUARD_ENDPOINTS, '调度端点未回收')

    def _run(self, tasks, max_workers=None):
        plan = make_plan(workers=max_workers or len(tasks))
        scheduler = ProfTaskScheduler()
        scheduler.CANCEL_GRACE_SECONDS = 0.2
        scheduler.run(
            tasks, replace(build_options(), input_root=self.root), plan,
            lambda task: self.serial_calls.append(task.task_id),
            valid_count=len(tasks), pending_count=len(tasks),
        )

    def assert_not_running(self, pid):
        try:
            with open('/proc/%d/stat' % pid) as reader:
                self.assertEqual(reader.read().rsplit(')', 1)[1].split()[0], 'Z')
        except FileNotFoundError:
            pass


    def test_real_raised_reclaims_children_and_allows_healthy_peer_to_finish(self):
        global _IGNORE_RAISED_CHILD_TERM, _FAIL_TERMINAL, _FAILURE_EVENTS, _CHILD_PID
        from msinterface.prof_task_executor import run_prof_task
        for index, (ignore_term, fail_terminal) in enumerate(((False, False), (True, False), (False, True))):
            with self.subTest(ignore_term=ignore_term, fail_terminal=fail_terminal):
                _IGNORE_RAISED_CHILD_TERM, _FAIL_TERMINAL = ignore_term, fail_terminal
                _FAILURE_EVENTS = tuple(multiprocessing.Event() for _ in range(4))
                _CHILD_PID = multiprocessing.Value('i', 0)
                tasks = [make_task(i) for i in (10 + index * 2, 11 + index * 2)]
                # 执行分支按 task_id 0/1 选择；无需创建无关的采集目录。
                tasks = [replace(task, task_id=i) for i, task in enumerate(tasks)]
                scheduler = ProfTaskScheduler()
                scheduler.CANCEL_GRACE_SECONDS = 0.2
                original_reclaim = scheduler._reclaim
                def reclaim(record, owned):
                    original_reclaim(record, owned)
                    if record.task.task_id == 0:
                        _FAILURE_EVENTS[2].set()
                signal.alarm(10)
                try:
                    with mock.patch('msinterface.prof_task_executor.execute_prof_task', _execute_raised_and_healthy), \
                            mock.patch.object(scheduler, '_reclaim', side_effect=reclaim):
                        with self.assertRaises(RuntimeError if fail_terminal else ProfException) as error:
                            scheduler._run_parallel(
                                tasks, replace(build_options(), input_root=self.root), make_plan(workers=2),
                                run_prof_task)
                    if fail_terminal:
                        self.assertIn('without terminal message', str(error.exception))
                    else:
                        self.assertEqual(error.exception.code, 7)
                        self.assertEqual(error.exception.message, 'original worker exception')
                    self.assertTrue(_FAILURE_EVENTS[3].is_set())
                    self.assert_not_running(_CHILD_PID.value)
                finally:
                    _IGNORE_RAISED_CHILD_TERM = _FAIL_TERMINAL = False
                    signal.alarm(self.TEST_WATCHDOG_SECONDS)


    # ------------------------------------------------------------------
    # IPC 集成测试
    # ------------------------------------------------------------------


    def test_real_nested_workers_spawn_inner_children_and_overlap(self):
        global _OVERLAP_BARRIER
        original_method = multiprocessing.get_start_method(allow_none=True)
        try:
            for index, method in enumerate(('fork', 'forkserver', 'spawn')):
                with self.subTest(parent_default=method):
                    multiprocessing.set_start_method(method, force=True)
                    # barrier 本身使用 fork 上下文，避免为测试新增 resource tracker。
                    _OVERLAP_BARRIER = multiprocessing.get_context('fork').Barrier(2)
                    with mock.patch.dict(sys.modules), mock.patch(NAMESPACE + '.os.listdir', return_value=['0']):
                        sys.modules.pop('msprof_analysis', None)
                        self.assertEqual(_context_eligibility(), ())
                    tasks = [make_task(i) for i in range(index * 4, index * 4 + 4)]
                    with mock.patch('msinterface.prof_task_executor.execute_prof_task', _execute_nested), \
                            mock.patch('msinterface.prof_parallel_plan._get_raw_data_bytes') as scan:
                        self._run(tasks, max_workers=2)
                    scan.assert_not_called()
                    self.assertEqual(multiprocessing.get_start_method(), method)
                    for task in tasks:
                        self.assertTrue(os.path.exists(os.path.join(_MARKER_DIR, 'inner_%d' % task.task_id)))
        finally:
            _OVERLAP_BARRIER = None
            multiprocessing.set_start_method(original_method, force=True)
        self.assertEqual(self.serial_calls, [])


if __name__ == '__main__':
    unittest.main()
