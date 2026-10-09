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

import argparse
import os
import logging
import subprocess  # nosec B404
import signal
import time
import platform

logging.basicConfig(level=logging.DEBUG, format='[%(asctime)s] [%(levelname)s]:%(message)s')


def gil_trace_record_start(pid_list=None, duration=-1):
    if platform.system() != 'Linux':
        logging.critical('Please run this script on Linux')
        return False

    if os.getuid() != 0:
        logging.critical('Please run this script as root')
        return False

    if not isinstance(duration, int):
        logging.warning("duration must be an integer, reset to default value -1")
        duration = -1

    if not isinstance(pid_list, list):
        pid_list = [pid_list]

    if not all(isinstance(pid, int) for pid in pid_list):
        logging.warning("pid_list must be a list of integers, reset to default value None")
        pid_list = None

    GilTraceRecord.start(pid_list, duration)
    logging.info("Start gil trace record for pid %s", pid_list)
    return True


def gil_trace_record_stop():
    GilTraceRecord.stop()
    logging.info("End gil trace record")


def on_exit(signal_num, frame):
    GilTraceRecord.stop()


class GilTraceRecord:
    @classmethod
    def start(cls, pid_list=None, duration=-1):
        pid_arg = f'pid={",".join(map(str, pid_list))}' if pid_list is not None else ''
        duration_arg = f'duration={duration}' if duration > 0 else ''
        start_command = ['sysTrace_cli', 'enable', 'GIL', duration_arg, pid_arg]

        try:
            logging.info("Starting sysTrace_cli record process%s", " ".join(start_command))
            result = subprocess.run(  # nosec B603
                start_command,
                capture_output=True,
                text=True,
                check=False,
                env={'LD_PRELOAD': '', 'PATH': os.getenv('PATH', '')},
            )
            logging.info("sysTrace_cli record process return code: %s", result.returncode)
            logging.info("sysTrace_cli record process stderr: %s", result.stderr)
        except Exception as e:
            logging.error("Failed to start sysTrace_cli record: %s", e)

    @classmethod
    def stop(cls):
        stop_command = ['sysTrace_cli', 'disable', 'GIL']
        try:
            result = subprocess.run(  # nosec B603
                stop_command,
                capture_output=True,
                text=True,
                check=False,
                env={'LD_PRELOAD': '', 'PATH': os.getenv('PATH', '')},
            )
            logging.info("sysTrace_cli disable GIL return code: %s", result.returncode)
            logging.info("sysTrace_cli disable GIL stderr: %s", result.stderr)
        except Exception as e:
            logging.error("Failed to stop sysTrace_cli record: %s", e)


def main(cmd_args):
    signal.signal(signal.SIGTERM, on_exit)
    try:
        gil_trace_record_start(cmd_args.pid, cmd_args.duration)
    except KeyboardInterrupt:
        gil_trace_record_stop()
        return
    if cmd_args.duration <= 0:
        logging.warning('Duration equals -1, start long term record')
        default_sleep_time = 1
        while True:
            try:
                time.sleep(default_sleep_time)
            except KeyboardInterrupt:
                break
    else:
        time.sleep(cmd_args.duration)
    gil_trace_record_stop()


def parse_pid_list(pid_str):
    if pid_str is None:
        return None
    try:
        pid_list = [int(pid.strip()) for pid in pid_str.split(',') if pid.strip().isdigit()]
    except ValueError:
        logging.error("Invalid PID format: %s", pid_str)
        return None
    return pid_list


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--pid',
        type=parse_pid_list,
        default=None,
        help='Specify the PID of the process to trace. Default: trace all processes running on NPU.',
    )
    parser.add_argument(
        '--duration',
        type=int,
        default=-1,
        help='Specify the duration of the trace in seconds. Default: -1 (long term record).',
    )

    args = parser.parse_args()
    main(args)
