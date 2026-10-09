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
from collections import defaultdict
from decimal import Decimal
from file_manager import FileManager

logging.basicConfig(level=logging.DEBUG, format='[%(asctime)s] [%(levelname)s]:%(message)s')

TAKE_GIL_EVENT_NAME = 'take_gil'
DROP_GIL_EVENT_NAME = 'drop_gil'
HOLD_GIL_EVENT_NAME = 'hold_gil'


def parse_args():
    parser = argparse.ArgumentParser(description='Convert GIL trace data')
    parser.add_argument('--input', type=str, required=True, help='Input GIL trace file')
    parser.add_argument('--output', type=str, required=True, help='Output JSON file')
    return parser.parse_args()


def create_x_event(name: str, ts: float, dur: float, pid: int, tid: int) -> dict:
    return {'name': name, 'ph': 'X', 'ts': ts, 'dur': dur, 'pid': pid, 'tid': tid}


def convert_gil_trace(trace_data: list) -> list:
    if not trace_data:
        return []
    if not isinstance(trace_data, list):
        msg = f"Invalid trace data format, expected list, got {type(trace_data)}"
        raise ValueError(msg)

    events_by_thread = defaultdict(list)
    pid_tid_map = defaultdict(set)
    for event in trace_data:
        events_by_thread[(event['pid'], event['tid'])].append(event)
        pid_tid_map[event['pid']].add(event['tid'])

    converted_events = []
    for (pid, tid), events in events_by_thread.items():
        events.sort(key=lambda x: x['ts'])

        i = 0
        while i < len(events):
            event = events[i]

            if event['name'] == TAKE_GIL_EVENT_NAME:
                j = i + 1
                while j < len(events) and not events[j]['name'] == DROP_GIL_EVENT_NAME:
                    j += 1

                if j < len(events):
                    event['ts'] = str(event['ts'])
                    events[j]['ts'] = str(events[j]['ts'])
                    hold_gil_ts = Decimal(event['ts']) + Decimal(event['dur'])
                    hold_gil_dur = Decimal(events[j]['ts']) - hold_gil_ts
                    hold_gil_x_event = create_x_event(
                        HOLD_GIL_EVENT_NAME, str(hold_gil_ts), float(hold_gil_dur), pid, tid
                    )
                    converted_events.append(event)
                    converted_events.append(hold_gil_x_event)
                    converted_events.append(events[j])
                    i = j + 1
                else:
                    i += 1

            else:
                i += 1

    for pid, tids in pid_tid_map.items():
        m_events = [{'name': 'process_name', 'ph': 'M', 'pid': pid, 'tid': 0, 'args': {'name': "GIL Trace"}}]
        m_events.extend(
            [
                {'name': 'thread_name', 'ph': 'M', 'pid': pid, 'tid': tid, 'args': {'name': f"Thread {tid}"}}
                for tid in tids
            ]
        )
        converted_events.extend(m_events)
    return converted_events


def main():
    args = parse_args()

    logging.info("Start to convert GIL trace file: %s", args.input)

    try:
        trace_data = FileManager.read_json_file(args.input)  # pylint: disable=no-member
    except Exception as e:
        logging.error("%s", e)
        sys.exit(1)

    try:
        converted_events = convert_gil_trace(trace_data)
    except Exception as e:
        logging.error("Failed to convert GIL trace file: %s", e)
        sys.exit(1)

    try:
        if not args.output.endswith('.json'):
            args.output += '.json'
        FileManager.create_json_file(converted_events, args.output)
    except Exception as e:
        logging.error("%s", e)
        sys.exit(1)

    logging.info("Finish to convert GIL trace file: %s to %s", args.input, args.output)


if __name__ == '__main__':
    main()
