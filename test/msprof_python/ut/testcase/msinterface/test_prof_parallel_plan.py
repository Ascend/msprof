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
import os
import tempfile
import unittest
from dataclasses import replace
from fractions import Fraction
from unittest import mock

from msinterface.prof_parallel_plan import (
    MAX_PROF_PROCESSES,
    CommandOptions,
    plan_prof_tasks,
    ProfTask,
    ProfTaskCollector,
    ResourceSnapshot,
    SystemResourceProbe,
    _parallel_task_supported,
    _ResourceProbeError,
    _get_raw_data_bytes,
)

NAMESPACE = 'msinterface.prof_parallel_plan'


def workspace_temp_dir():
    return tempfile.TemporaryDirectory(dir=os.path.dirname(__file__))


def build_options(command='import', **kwargs):
    return replace(CommandOptions(
        input_root='/root', command=command, command_type=None, cluster_flag=False,
        export_mode=None, model_id=None, iteration_id=None, iteration_count=1,
        export_format=None, reports_path='', clear_mode=False), **kwargs)


def build_task(task_id, prof_path, raw_bytes=100, is_cluster=True):
    return ProfTask(
        task_id=task_id,
        prof_path=prof_path,
        host_path='',
        device_paths=('device_0',),
        all_result_paths=('device_0',),
        raw_bytes=raw_bytes,
        is_cluster=is_cluster,
    )


class TestProfTaskCollector(unittest.TestCase):
    def test_collect_should_build_root_task_when_input_root_has_valid_members(self):
        options = build_options()
        with mock.patch(NAMESPACE + '.DataCheckManager.iter_valid_profiling_sub_paths',
                        return_value=[('device_0', '/root/device_0', True), ('host', '/root/host', True)]) as scan, \
                mock.patch(NAMESPACE + '._get_raw_data_bytes', return_value=None) as summarize:
            result = ProfTaskCollector('test_file').collect(options)

        summarize.assert_not_called()
        scan.assert_called_once_with('/root', 'test_file', skip_invalid=False)
        self.assertEqual(len(result.tasks), 1)
        task = result.tasks[0]
        self.assertEqual(task.prof_path, '/root')
        self.assertFalse(task.is_cluster)
        self.assertEqual(task.to_path_table('import')['host'], '/root/host')
        self.assertEqual(task.to_path_table('import')['device'], ['/root/device_0'])
        self.assertFalse(task.to_path_table('import')['is_cluster'])
        task.to_path_table('import')['device'].clear()
        self.assertEqual(task.device_paths, ('/root/device_0',))
        self.assertNotIn('is_cluster', task.to_path_table('export'))
        self.assertEqual(result.valid_prof_count, 1)
        self.assertEqual(result.valid_data_count, 2)

    def test_collect_should_accept_any_name_at_current_level_only(self):
        options = build_options()

        def scan(path, file_name, skip_invalid=False):
            if path == '/root':
                # 保留公共迭代器顺序；普通名称也可解析，深一层的采集根不递归发现。
                return [('ordinary_dir', '/root/ordinary_dir', False),
                        ('nested', '/root/nested', False),
                        ('PROF_2', '/root/PROF_2', False),
                        ('PROF_1', '/root/PROF_1', False)]
            if path == '/root/ordinary_dir':
                return [('device_0', '/root/ordinary_dir/device_0', True),
                        ('invalid', '/root/ordinary_dir/invalid', False)]
            if path == '/root/nested':
                return [('capture', '/root/nested/capture', False)]
            if path == '/root/PROF_1':
                return [('device_0', '/root/PROF_1/device_0', True)]
            if path == '/root/PROF_2':
                return [('host', '/root/PROF_2/host', True)]
            return []

        with mock.patch(NAMESPACE + '.DataCheckManager.iter_valid_profiling_sub_paths', side_effect=scan), \
                mock.patch(NAMESPACE + '.warn') as warning:
            result = ProfTaskCollector('test_file').collect(options)

        warning.assert_any_call('test_file', ProfTaskCollector._INVALID_DIR_TEMPLATE % '/root/ordinary_dir')
        self.assertEqual([os.path.basename(task.prof_path) for task in result.tasks], ['ordinary_dir', 'PROF_2', 'PROF_1'])
        self.assertTrue(all(task.is_cluster for task in result.tasks))
        self.assertTrue(all(task.to_path_table('import')['is_cluster'] for task in result.tasks))
        self.assertEqual(result.valid_prof_count, 3)
        self.assertEqual(result.valid_data_count, 3)
        self.assertEqual(result.tasks[0].to_path_table('import')['collection_path'], '/root/ordinary_dir')

    def test_collect_should_warn_and_skip_prof_without_valid_members(self):
        options = build_options()

        def scan(path, file_name, skip_invalid=False):
            if path == '/root':
                return [('PROF_1', '/root/PROF_1', False)]
            # 成员可访问但不含 info.json：沿用原“Invalid parsing dir”告警。
            return [('device_0', '/root/PROF_1/device_0', False)]

        with mock.patch(NAMESPACE + '.DataCheckManager.iter_valid_profiling_sub_paths', side_effect=scan), \
                mock.patch(NAMESPACE + '.warn') as warning:
            result = ProfTaskCollector('test_file').collect(options)

        self.assertEqual(result.tasks, [])
        self.assertEqual(result.valid_prof_count, 0)
        self.assertEqual(result.valid_data_count, 0)
        warning.assert_any_call(
            'test_file',
            'Invalid parsing dir(%s), -dir must be profiling data dir, such as PROF_XXX_XXX_XXX' % '/root/PROF_1')


class TestGetRawDataBytes(unittest.TestCase):
    def test_raw_bytes_sum_direct_files_across_members(self):
        with workspace_temp_dir() as root:
            members = [os.path.join(root, name) for name in ('host', 'device_0')]
            for member, size in zip(members, (100, 50)):
                data_dir = os.path.join(member, 'data')
                os.makedirs(data_dir)
                with open(os.path.join(data_dir, 'input.bin'), 'wb') as writer:
                    writer.write(b'x' * size)
            self.assertEqual(_get_raw_data_bytes(tuple(members)), 150)

    def test_read_error_returns_unknown_instead_of_partial_sum(self):
        with mock.patch(NAMESPACE + '.get_data_file_memory', side_effect=[1.0, PermissionError('denied')]):
            self.assertIsNone(_get_raw_data_bytes(('/root/host', '/root/device_0')))


class TestSystemResourceProbe(unittest.TestCase):
    def setUp(self):
        patcher = mock.patch('os.sched_getaffinity', create=True, return_value=set(range(64)))
        patcher.start()
        self.addCleanup(patcher.stop)

    V2_FILES = {
        '/proc/meminfo': 'MemTotal:       4000000 kB\nMemAvailable:   2097152 kB\n',
        '/proc/self/cgroup': '0::/slice/app\n',
        '/proc/self/mountinfo': '30 25 0:29 / /sys/fs/cgroup rw,nosuid - cgroup2 cgroup2 rw\n',
        '/sys/fs/cgroup/slice/app/memory.max': '1048576\n',
        '/sys/fs/cgroup/slice/app/memory.current': '262144\n',
        '/sys/fs/cgroup/slice/memory.max': '2097152\n',
        '/sys/fs/cgroup/slice/memory.current': '524288\n',
        '/sys/fs/cgroup/memory.max': 'max\n',
        '/sys/fs/cgroup/memory.current': '1024\n',
        '/sys/fs/cgroup/slice/app/cpu.max': '200000 100000\n',
        '/sys/fs/cgroup/slice/cpu.max': 'max 100000\n',
        '/sys/fs/cgroup/cpu.max': 'max 100000\n',
    }


    def probe_files(self, files):
        with mock.patch('os.name', 'posix'), mock.patch(NAMESPACE + '.is_linux', return_value=True):
            return SystemResourceProbe(
                read_file=lambda path: files.get(path.replace(os.sep, '/')), os_cpu_count=lambda: 64).probe()

    def test_probe_should_honor_cgroup_v2_limits(self):
        snapshot = self.probe_files(self.V2_FILES)
        self.assertEqual(snapshot.unknown_reasons, ())
        # min(MemAvailable=2GiB, cgroup 余量 768KiB)
        self.assertEqual(snapshot.available_ram_bytes, 786432)
        # min(cpu_count=64, quota=2)
        self.assertEqual(snapshot.effective_cpu, Fraction(2))


    def test_pid_controller_is_not_probed(self):
        reads = []
        files = dict(self.V2_FILES)
        def read(path):
            reads.append(path)
            return files.get(path.replace(os.sep, '/'))
        with mock.patch('os.name', 'posix'), mock.patch(NAMESPACE + '.is_linux', return_value=True):
            probe = SystemResourceProbe(read_file=read)
            snapshot = probe.probe()
        self.assertEqual(snapshot.unknown_reasons, ())
        self.assertFalse(any('pids.' in path for path in reads))
        for path in ('/proc/self/cgroup', '/proc/self/mountinfo'):
            self.assertEqual(reads.count(path), 1)
        files['/sys/fs/cgroup/slice/app/cpu.max'] = '100000 100000'
        with mock.patch('os.name', 'posix'), mock.patch(NAMESPACE + '.is_linux', return_value=True):
            self.assertEqual(probe.probe().effective_cpu, Fraction(1))
        self.assertEqual(reads.count('/proc/self/cgroup'), 2)
        self.assertEqual(reads.count('/proc/self/mountinfo'), 2)

    def test_probe_should_honor_cgroup_v1_limits(self):
        files = {
            '/proc/meminfo': 'MemAvailable:   1000000 kB\n',
            '/proc/self/cgroup': '8:memory:/slice/app\n9:cpu:/slice/app\n',
            '/proc/self/mountinfo': (
                '31 25 0:30 / /sys/fs/cgroup/memory rw - cgroup cgroup rw,memory\n'
                '32 25 0:31 / /sys/fs/cgroup/cpu rw - cgroup cgroup rw,cpu\n'
            ),
            '/sys/fs/cgroup/memory/slice/app/memory.limit_in_bytes': '524288\n',
            '/sys/fs/cgroup/memory/slice/app/memory.usage_in_bytes': '262144\n',
            '/sys/fs/cgroup/cpu/slice/app/cpu.cfs_quota_us': '150000\n',
            '/sys/fs/cgroup/cpu/slice/app/cpu.cfs_period_us': '100000\n',
        }
        for parent in ('/slice', ''):
            base = '/sys/fs/cgroup'
            files[base + '/memory' + parent + '/memory.limit_in_bytes'] = str(1 << 62)
            files[base + '/memory' + parent + '/memory.usage_in_bytes'] = '0'
            files[base + '/cpu' + parent + '/cpu.cfs_quota_us'] = '-1'
            files[base + '/cpu' + parent + '/cpu.cfs_period_us'] = '100000'
        snapshot = self.probe_files(files)
        self.assertEqual(snapshot.unknown_reasons, ())
        self.assertEqual(snapshot.available_ram_bytes, 262144)
        self.assertEqual(snapshot.effective_cpu, Fraction(3, 2))


    def snapshot_for(self, changes):
        return self.probe_files(dict(self.V2_FILES, **changes))

    def test_unreadable_finite_memory_is_unknown(self):
        snapshot = self.snapshot_for({'/sys/fs/cgroup/slice/app/memory.current': None})
        self.assertIsNone(snapshot.available_ram_bytes)
        self.assertTrue(snapshot.unknown_reasons)


    def test_ancestor_cpu_quota_restricts_unlimited_leaf(self):
        snapshot = self.snapshot_for({'/sys/fs/cgroup/slice/app/cpu.max': 'max 100000',
                                      '/sys/fs/cgroup/slice/cpu.max': '150000 100000'})
        self.assertEqual(snapshot.unknown_reasons, ())
        self.assertEqual(snapshot.effective_cpu, Fraction(3, 2))


    def test_missing_v2_limits_require_confirmed_disabled_controller(self):
        from io import StringIO
        for controller, finite in (('cpu', Fraction(3, 2)), ('memory', 1572864)):
            other = 'memory' if controller == 'cpu' else 'cpu'
            for name, control, limit, expected in (
                ('disabled', other + ' pids', None, finite),
                ('enabled', 'cpu memory pids', None, None),
                ('controller_unreadable', PermissionError('denied'), None, None),
                ('controller_missing', None, None, None),
                ('limit_unreadable', other + ' pids', PermissionError('denied'), None),
            ):
                with self.subTest(controller=controller, case=name):
                    files = dict(self.V2_FILES)
                    files['/sys/fs/cgroup/slice/app/' + controller + '.max'] = limit
                    files['/sys/fs/cgroup/slice/cgroup.subtree_control'] = control
                    files['/sys/fs/cgroup/' + controller + '.max'] = None
                    if controller == 'cpu':
                        files['/sys/fs/cgroup/slice/cpu.max'] = '150000 100000'
                    else:
                        files['/sys/fs/cgroup/slice/app/memory.current'] = None
                    def read_file(path, *_args, **_kwargs):
                        value = files.get(path.replace(os.sep, '/'))
                        if value is None:
                            raise FileNotFoundError(path)
                        if isinstance(value, Exception):
                            raise value
                        return StringIO(value)
                    with mock.patch('builtins.open', side_effect=read_file), mock.patch('os.name', 'posix'), \
                            mock.patch(NAMESPACE + '.is_linux', return_value=True):
                        snapshot = SystemResourceProbe(os_cpu_count=lambda: 64).probe()
                    actual = snapshot.effective_cpu if controller == 'cpu' else snapshot.available_ram_bytes
                    self.assertEqual(actual, expected)
                    self.assertEqual(bool(snapshot.unknown_reasons), expected is None)

    def test_subtree_only_mount_is_unknown_for_v1_and_v2(self):
        for v2 in (True, False):
            with self.subTest(v2=v2):
                files = {
                    '/proc/self/cgroup': '0::/slice/app\n' if v2 else '8:memory:/slice/app\n',
                    '/proc/self/mountinfo': '30 25 0:29 /slice/app /cg rw - {} cgroup rw{}\n'.format(
                        'cgroup2' if v2 else 'cgroup', '' if v2 else ',memory'),
                }
                probe = SystemResourceProbe(read_file=files.get)
                with self.assertRaises(_ResourceProbeError):
                    list(probe._cgroup_levels('memory'))


class TestParallelTaskEligibility(unittest.TestCase):
    def test_parallel_scope_depends_on_command_not_cluster_context(self):
        task = build_task(0, '/root/capture')
        for command in ('import', 'export'):
            options = build_options(command=command, command_type='db')
            self.assertTrue(_parallel_task_supported(task, options))
            self.assertTrue(_parallel_task_supported(replace(task, is_cluster=False), options))
            self.assertFalse(_parallel_task_supported(task, replace(options, command='query')))
            if command == 'export':
                self.assertFalse(_parallel_task_supported(task, replace(options, command_type='unknown')))


class TestProfResourcePlanner(unittest.TestCase):
    def setUp(self):
        self.options = build_options()

    @staticmethod
    def snapshot(ram=1 << 40, cpu=Fraction(256), reasons=()):
        return ResourceSnapshot(ram, cpu, reasons)

    def test_unknown_resources_input_or_unsupported_task_fall_back_to_serial(self):
        for case in ('resource', 'input', 'unsupported'):
            with self.subTest(case=case):
                tasks = [build_task(i, '/root/PROF_%d' % i,
                                    raw_bytes=None if case == 'input' else 100) for i in range(2)]
                snapshot = (self.snapshot(ram=None, reasons=('MEMORY_UNKNOWN:MEMINFO_UNREADABLE',))
                            if case == 'resource' else self.snapshot())
                with mock.patch(NAMESPACE + '._parallel_task_supported', return_value=case != 'unsupported'):
                    plan = plan_prof_tasks(tasks, self.options, snapshot)
                self.assertEqual(plan.mode, 'SERIAL')
                self.assertTrue(plan.reason_codes)
                if case == 'resource':
                    self.assertEqual(plan.reason_codes, ('RESOURCE_UNKNOWN', 'MEMORY_UNKNOWN:MEMINFO_UNREADABLE'))

    def test_initial_memory_limit_uses_largest_prof_size(self):
        tasks = [build_task(i, '/root/PROF_%d' % i, raw_bytes=100 if i else 200) for i in range(10)]
        for ram, workers in ((3000, 1), (9000, 3)):
            with self.subTest(ram=ram), mock.patch(NAMESPACE + '._parallel_task_supported', return_value=True):
                plan = plan_prof_tasks(tasks, self.options, self.snapshot(ram=ram))
                self.assertEqual(plan.mode, 'SERIAL' if workers == 1 else 'PARALLEL')
                self.assertEqual(plan.max_workers, workers)

    def test_initial_cpu_slots_and_internal_cap_bound_workers(self):
        tasks = [build_task(index, '/root/PROF_%d' % index) for index in range(20)]
        for cpu, workers in ((32, 1), (Fraction(127, 2), 1), (64, 2), (128, 4), (256, 8), (512, 8)):
            with self.subTest(cpu=cpu), mock.patch(NAMESPACE + '._parallel_task_supported', return_value=True):
                snapshot = self.snapshot(cpu=Fraction(cpu))
                plan = plan_prof_tasks(tasks, self.options, snapshot)
                self.assertEqual(plan.mode, 'SERIAL' if workers == 1 else 'PARALLEL')
                self.assertEqual(plan.max_workers, workers)
                self.assertLessEqual(plan.max_workers, MAX_PROF_PROCESSES)

    def test_import_and_export_use_fixed_cpu_budget(self):
        tasks = [build_task(i, '/root/capture_%d' % i, is_cluster=False) for i in range(2)]
        for command, kind in (('import', None), ('export', 'summary'), ('export', 'timeline'), ('export', 'db')):
            with self.subTest(command=command, kind=kind):
                options = build_options(command=command, command_type=kind)
                plan = plan_prof_tasks(tasks, options, self.snapshot(cpu=Fraction(64)))
                self.assertEqual(plan.mode, 'PARALLEL')
                self.assertEqual(plan.max_workers, 2)


if __name__ == '__main__':
    unittest.main()
