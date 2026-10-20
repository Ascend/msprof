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
import logging
import os
import tempfile
import unittest
from dataclasses import replace
from unittest import mock

from common_func.common import LogFactory
from common_func.msprof_exception import ProfException
from common_func.profiling_scene import ExportMode
from msinterface.prof_parallel_plan import CommandOptions, build_prof_task
from msinterface.prof_task_executor import (
    execute_prof_task,
    rebuild_exception,
    run_prof_task,
    _reset_inherited_logging,
)

NAMESPACE = 'msinterface.prof_task_executor'


def build_options(command='import', **kwargs):
    return replace(CommandOptions(
        input_root='/root', command=command, command_type=None, cluster_flag=False,
        export_mode=None, model_id=None, iteration_id=None, iteration_count=1,
        export_format=None, reports_path='', clear_mode=False), **kwargs)


class TestProfTaskExecutor(unittest.TestCase):
    def test_execute_should_rebuild_import_command_context(self):
        options = build_options(command='import', cluster_flag=True)
        task = build_prof_task(0, '/root/PROF_1', '', ['d0'], ('d0',), True)
        with mock.patch('msinterface.msprof_import.ImportCommand') as command_class:
            instance = command_class.return_value
            execute_prof_task(task, options)
        args = command_class.call_args.args[0]
        self.assertEqual(args.collection_path, '/root/PROF_1')
        self.assertTrue(args.cluster_flag)
        instance._execute_prof_task.assert_called_once_with(task.to_path_table(options.command))

    def test_execute_should_rebuild_export_command_context(self):
        options = build_options(command='export', command_type='summary', export_mode=1, model_id=2,
                                iteration_id=3, iteration_count=4, export_format='csv',
                                reports_path='/reports.json', clear_mode=True)
        task = build_prof_task(0, '/root/PROF_1', '/root/PROF_1/host', [],
                               ('/root/PROF_1/host',), True)
        with mock.patch('msinterface.msprof_export.ExportCommand') as command_class, \
                mock.patch(NAMESPACE + '.ProfilingScene') as scene:
            command_class.return_value._cluster_params = {}
            execute_prof_task(task, options)
        self.assertEqual(command_class.call_args.args[0], 'summary')
        args = command_class.call_args.args[1]
        self.assertEqual(args.collection_path, '/root/PROF_1')
        self.assertEqual(args.model_id, 2)
        self.assertEqual(args.iteration_id, 3)
        self.assertEqual(args.iteration_count, 4)
        self.assertEqual(args.export_format, 'csv')
        self.assertEqual(args.reports_path, '/reports.json')
        self.assertTrue(args.clear_mode)
        scene.return_value.set_mode.assert_called_once_with(ExportMode(1))
        # worker 内按任务恢复场景上下文，不依赖父进程残留。
        self.assertTrue(command_class.return_value._cluster_params['is_cluster_scene'])
        command_class.return_value._execute_prof_task.assert_called_once_with(task.to_path_table(options.command))


class TestRunProfTask(unittest.TestCase):
    """验证真实 worker 入口的握手顺序、取消与异常回传；传输由通道 UT 覆盖。"""

    def setUp(self):
        self.options = build_options()
        self.task = build_prof_task(7, '/root/PROF_7', '', ['d0'], ('d0',), True)

    def _run_worker(self, execute_error=None, cancel=False):
        result = mock.Mock()
        with mock.patch(NAMESPACE + '._ProfWorkerChannel', return_value=result) as channel, \
                mock.patch(NAMESPACE + '.execute_prof_task') as execute, \
                mock.patch(NAMESPACE + '._reset_inherited_logging') as reset_logs, \
                mock.patch(NAMESPACE + '._detach_session'), \
                mock.patch(NAMESPACE + '._reset_worker_signals'):
            channel.VERSION = 1
            execute.side_effect = execute_error

            def receive_run(**_kwargs):
                self.assertEqual(result.send_message.call_args.args[0]['type'], 'READY')
                execute.assert_not_called()
                return {'type': 'RUN', 'version': 1, 'task_id': self.task.task_id}

            result.recv_blocking.side_effect = EOFError if cancel else receive_run
            run_prof_task(self.task, self.options, result)
            if cancel:
                execute.assert_not_called()
                reset_logs.assert_not_called()
            else:
                execute.assert_called_once_with(self.task, self.options)
                reset_logs.assert_called_once_with()
        result.close.assert_called_once_with()
        return [call.args[0] for call in result.send_message.call_args_list]

    def test_worker_should_returned_after_run_handshake(self):
        messages = self._run_worker()
        self.assertEqual([message['type'] for message in messages], ['READY', 'RETURNED'])
        for message in messages:
            self.assertEqual(message['task_id'], self.task.task_id)
            self.assertEqual(message['version'], 1)

    def test_worker_should_report_raised_prof_exception(self):
        messages = self._run_worker(ProfException(ProfException.PROF_INVALID_DATA_ERROR, 'invalid data'))
        self.assertEqual([message['type'] for message in messages], ['READY', 'RAISED'])
        self.assertEqual(messages[-1]['exception']['kind'], 'ProfException')
        self.assertEqual(messages[-1]['exception']['code'], ProfException.PROF_INVALID_DATA_ERROR)
        self.assertEqual(messages[-1]['exception']['message'], 'invalid data')

    def test_worker_should_exit_silently_when_control_closes_before_run(self):
        self.assertEqual([message['type'] for message in self._run_worker(cancel=True)], ['READY'])

    def test_channel_protocol_failure_closes_worker_without_escaping(self):
        from msinterface.prof_parallel_scheduler import ChannelProtocolError
        with mock.patch(NAMESPACE + '._ProfWorkerChannel') as channel, \
                mock.patch(NAMESPACE + '.execute_prof_task') as execute, \
                mock.patch(NAMESPACE + '._detach_session'), \
                mock.patch(NAMESPACE + '._reset_worker_signals'), \
                mock.patch(NAMESPACE + '.logging.error') as log_error:
            channel.return_value.send_message.side_effect = ChannelProtocolError('oversize')
            run_prof_task(self.task, self.options, mock.Mock())
            execute.assert_not_called()
            channel.return_value.close.assert_called_once_with()
            log_error.assert_called_once()


class TestExceptionEncoding(unittest.TestCase):
    def test_rebuild_generic_exception_without_dynamic_import(self):
        exception = rebuild_exception({'kind': 'Exception', 'type': 'ValueError', 'message': 'boom'})
        self.assertIsInstance(exception, RuntimeError)
        self.assertIn('ValueError', str(exception))
        self.assertIn('boom', str(exception))


class TestResetInheritedLogging(unittest.TestCase):
    def test_reset_should_remove_file_handlers_and_clear_cache(self):
        with tempfile.TemporaryDirectory(dir=os.path.dirname(__file__)) as temp_dir:
            loggers = (logging.getLogger(), logging.getLogger('ut_prof_parallel_executor_child'))
            handlers = [logging.FileHandler(os.path.join(temp_dir, '%d.log' % i)) for i in range(2)]
            try:
                for logger, handler in zip(loggers, handlers):
                    logger.addHandler(handler)
                with mock.patch.object(LogFactory, 'loggers', {'stale': object()}):
                    _reset_inherited_logging()
                    self.assertEqual(LogFactory.loggers, {})
                for logger, handler in zip(loggers, handlers):
                    self.assertNotIn(handler, logger.handlers)
                    self.assertIsNone(handler.stream)
            finally:
                for logger, handler in zip(loggers, handlers):
                    logger.removeHandler(handler)
                    handler.close()


if __name__ == '__main__':
    unittest.main()
