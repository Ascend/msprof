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

"""子进程任务执行适配：重建命令上下文并执行单个 PROF 任务。

``run_prof_task`` 是模块级 worker 入口，承担启动握手与异常编码；只有获得
RUN 后才调用 ``execute_prof_task``。命令类在本模块内局部导入，避免
循环依赖；规划/调度模块不导入命令类。
"""

import logging
import os
import signal
import time
from argparse import Namespace

from common_func.common import error, warn
from common_func.msprof_exception import ProfException
from common_func.profiling_scene import ExportMode, ProfilingScene
from msinterface.prof_parallel_scheduler import ChannelProtocolError, _ProfWorkerChannel, _process_start_time

TERMINAL_MESSAGE_LIMIT = 1024


def execute_prof_task(task: any, options: any) -> None:
    """重建命令上下文并执行单个任务，不重扫目录或重复集群后处理。"""
    args = Namespace(
        collection_path=task.prof_path,
        cluster_flag=options.cluster_flag,
        model_id=options.model_id,
        iteration_id=options.iteration_id,
        iteration_count=options.iteration_count,
        export_format=options.export_format,
        reports_path=options.reports_path,
        clear_mode=options.clear_mode,
    )
    if options.command == 'import':
        from msinterface.msprof_import import ImportCommand

        command = ImportCommand(args)
    else:
        from msinterface.msprof_export import ExportCommand

        command = ExportCommand(options.command_type, args)
        ProfilingScene().set_mode(ExportMode(options.export_mode))
        command._cluster_params['is_cluster_scene'] = task.is_cluster
    command._execute_prof_task(task.to_path_table(options.command))


def run_prof_task(task: any, options: any, worker_conn: any, parent_identity: any = None) -> None:
    """外层 worker 入口：握手、执行、回交一次终结通知。

    READY 前建立独立 session/process group（POSIX）；等待 RUN 期间控制通道
    关闭（父进程取消 bootstrap 或消失）时静默退出。RUN 后才初始化日志并进入
    解析；终结消息仅含必要描述，完整堆栈留原日志。
    """
    channel = None
    try:
        _detach_session()
        _reset_worker_signals()
        channel = _ProfWorkerChannel(worker_conn)
        channel.send_message(
            {
                'type': 'READY',
                'version': _ProfWorkerChannel.VERSION,
                'task_id': task.task_id,
                'pid': os.getpid(),
                'pgid': _current_pgid(),
            }
        )
        deadline = time.monotonic() + 300.0
        message = None
        # RUN/EOF 会立即唤醒；0.5 秒仅限制 bootstrap 身份复核间隔，不轮询业务执行。
        while message is None and time.monotonic() < deadline:
            if parent_identity is not None and _process_start_time(parent_identity[0]) != parent_identity[1]:
                return
            message = channel.recv_blocking(timeout=0.5)
        if (
            not isinstance(message, dict)
            or message.get('type') != 'RUN'
            or message.get('task_id') != task.task_id
            or message.get('version') != _ProfWorkerChannel.VERSION
        ):
            return
        _reset_inherited_logging()
        try:
            execute_prof_task(task, options)
        except ProfException as err:
            channel.send_message(_encode_prof_exception(task, err))
        except SystemExit as err:
            channel.send_message(
                {
                    'type': 'RAISED',
                    'version': _ProfWorkerChannel.VERSION,
                    'task_id': task.task_id,
                    'exception': {'kind': 'SystemExit', 'code': err.code},
                }
            )
        except BaseException as err:  # pylint: disable=broad-except
            channel.send_message(_encode_generic_exception(task, err))
        else:
            channel.send_message(
                {
                    'type': 'RETURNED',
                    'version': _ProfWorkerChannel.VERSION,
                    'task_id': task.task_id,
                }
            )
    except ChannelProtocolError:
        logging.error('PROF worker channel protocol failed.', exc_info=True)
    except (EOFError, OSError):
        # 控制端关闭（bootstrap 取消/父进程退出）或结果端失效：按预期退出。
        return
    finally:
        if channel is not None:
            channel.close()
        else:
            worker_conn.close()


def _reset_worker_signals():
    signal.signal(signal.SIGINT, signal.default_int_handler)
    signal.signal(signal.SIGTERM, signal.SIG_DFL)


def _detach_session() -> None:
    """POSIX 下在 READY 前建立独立 session/process group；失败时不得进入 READY。"""
    if os.name != 'posix':
        return
    os.setsid()  # pylint: disable=no-member  # POSIX API，Windows 静态检查不可见。


def _current_pgid() -> any:
    if os.name != 'posix':
        return None
    try:
        return os.getpgid(0)  # pylint: disable=no-member  # POSIX API，Windows 静态检查不可见。
    except OSError:
        return os.getpid()


def _reset_inherited_logging() -> None:
    """清理 fork 继承的 profiling 文件 handler；只改本 worker 的副本。

    先复制 handler 列表再逐一解除并关闭，不关闭父进程或无关日志器的
    handler；``LogFactory`` 缓存同样在本 worker 副本中重建。随后由原
    ``prepare_and_load_info`` 链路按本任务路径重新初始化日志。
    """
    root_logger = logging.getLogger()
    for handler in list(root_logger.handlers):
        root_logger.removeHandler(handler)
        _close_file_handler(handler)
    for _name, logger in list(logging.Logger.manager.loggerDict.items()):
        if not isinstance(logger, logging.Logger):
            continue
        for handler in list(logger.handlers):
            logger.removeHandler(handler)
            _close_file_handler(handler)
    try:
        from common_func.common import LogFactory

        LogFactory.loggers = {}
    except (ImportError, AttributeError):
        pass


def _close_file_handler(handler: any) -> None:
    if isinstance(handler, logging.FileHandler):
        try:
            handler.close()
        except OSError:
            pass


def _encode_prof_exception(task: any, err: ProfException) -> dict:
    callback = getattr(err.callback, '__name__', '') or ''
    return {
        'type': 'RAISED',
        'version': _ProfWorkerChannel.VERSION,
        'task_id': task.task_id,
        'exception': {
            'kind': 'ProfException',
            'code': err.code,
            'message': _truncate(err.message),
            'callback': callback,
        },
    }


def _encode_generic_exception(task: any, err: BaseException) -> dict:
    logging.error(str(err), exc_info=True)
    return {
        'type': 'RAISED',
        'version': _ProfWorkerChannel.VERSION,
        'task_id': task.task_id,
        'exception': {
            'kind': 'Exception',
            'type': type(err).__name__,
            'message': _truncate(str(err)),
        },
    }


def _truncate(message: any) -> str:
    """异常消息过长时保留类别并带截断标识，不发送完整堆栈。"""
    text = '' if message is None else str(message)
    if len(text) <= TERMINAL_MESSAGE_LIMIT:
        return text
    return text[:TERMINAL_MESSAGE_LIMIT] + '...[truncated]'


def rebuild_exception(payload: dict) -> BaseException:
    """父进程按固定映射重建异常；不根据消息内容动态 import/执行。"""
    kind = payload.get('kind')
    if kind == 'ProfException':
        callback = {'error': error, 'warn': warn}.get(payload.get('callback'), error)
        return ProfException(int(payload.get('code', 0)), str(payload.get('message', '')), callback)
    if kind == 'SystemExit':
        return SystemExit(payload.get('code'))
    exception_type = str(payload.get('type', 'Exception'))
    message = str(payload.get('message', ''))
    return RuntimeError('prof parallel worker failed: {}: {}'.format(exception_type, message))
