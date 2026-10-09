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
import logging
import sys
from pathlib import Path
from collections import defaultdict
from file_manager import FileManager


logging.basicConfig(level=logging.DEBUG, format='[%(asctime)s] [%(levelname)s]:%(message)s')

REQUIRED_FIELDS = ['name', 'start', 'duration', 'rankId', 'pid', 'tid']
NUMERIC_FIELDS = ['rankId', 'pid', 'tid']


def parse_line(line: str) -> dict:
    original_line = line.strip()
    if not original_line:
        return {}

    result = {}
    for item in (part.strip() for part in original_line.split() if ':' in part):
        key, value = item.split(':', 1)
        result[key.strip()] = value.strip()

    if not all(field in result for field in REQUIRED_FIELDS):
        logging.error("Line does not contain all required fields: %s", original_line)
        return {}

    for field in NUMERIC_FIELDS:
        try:
            result[field] = int(result[field])
        except ValueError:
            logging.error("Failed to convert field %s to int: %s in line: %s", field, result[field], original_line)
            return {}

    return result


def get_unique_id(rank_id: int, pid: int) -> str:
    return f"{rank_id}_{pid}"


def parse_extra_info(extra_info: str) -> dict:
    result = {}
    for item in (part.strip() for part in extra_info.split(';') if ':' in part):
        key, value = item.split(':', 1)
        result[key.strip()] = value.strip()
    return result


def convert_to_trace(log_lines) -> dict:
    trace_data = {
        'traceEvents': [],
        'displayTimeUnit': 'us',
    }

    process_name_map = {}
    thread_map = defaultdict(set)

    for line in log_lines:
        if not line:
            continue
        rank_id = line.get('rankId', 'unknown_rank')
        pid = line.get('pid', 0)
        unique_pid = get_unique_id(rank_id, pid)
        process_name = line.get('pname', f'process_{unique_pid}')
        tid = line.get('tid', 0)
        thread_name = line.get('tname', f'thread_{tid}')

        name = line.get('name', 'unknown_func')
        start_time = line.get('start', 0)
        duration = line.get('duration', 0)

        if unique_pid not in process_name_map and process_name.lower() != 'none':
            process_name_map[unique_pid] = process_name

        if thread_name.lower() != 'unknown':
            thread_map[unique_pid].add((tid, thread_name))

        args = {'rankId': rank_id}
        extra_info = line.get('extraInfo', '')
        if extra_info:
            args.update(parse_extra_info(extra_info))

        trace_data['traceEvents'].append(
            {
                'ph': 'X',
                'pid': unique_pid,
                'tid': tid,
                'name': name,
                'ts': start_time,
                'dur': duration,
                'cat': 'function',
                'args': args,
            }
        )

    for unique_pid, process_name in process_name_map.items():
        trace_data['traceEvents'].append(
            {'ph': 'M', 'pid': unique_pid, 'tid': 0, 'name': 'process_name', 'args': {'name': process_name}}
        )
        for tid, thread_name in thread_map[unique_pid]:
            trace_data['traceEvents'].append(
                {
                    'ph': 'M',
                    'pid': unique_pid,
                    'tid': tid,
                    'name': 'thread_name',
                    'args': {'name': f"Thread {tid} ({thread_name})"},
                }
            )

    return trace_data


def main():
    parser = argparse.ArgumentParser(description='Convert function monitor log to trace format')
    parser.add_argument('--input', type=str, required=True, help='Path to the input log file')
    parser.add_argument('--output', type=str, required=False, help='Path to the output trace file')
    args = parser.parse_args()

    log_lines = []
    total_lines = 0
    valid_lines = 0

    try:
        FileManager.check_file_readable(args.input)

        with open(args.input, 'r', encoding='utf-8') as f:
            for line in f:
                total_lines += 1
                parsed_line = parse_line(line)
                if parsed_line:
                    log_lines.append(parsed_line)
                    valid_lines += 1

    except Exception as e:
        logging.error("Failed to read input file: %s, error: %s", args.input, e)
        sys.exit(1)

    trace_data = convert_to_trace(log_lines)
    try:
        if not args.output:
            args.output = Path(args.input).stem + '_trace.json'
        if not args.output.endswith('.json'):
            args.output += '.json'
        FileManager.create_json_file(trace_data, args.output)
    except Exception as e:
        logging.error("Failed to write output file: %s, error: %s", args.output, e)
        sys.exit(1)

    logging.info("Processed %s lines, valid %s lines, output %s", total_lines, valid_lines, args.output)


if __name__ == '__main__':
    main()
