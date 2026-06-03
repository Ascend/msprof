# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
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

from collections import OrderedDict

from common_func.db_manager import DBManager
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import is_number
from common_func.trace_view_header_constant import TraceViewHeaderConstant


class TraceViewManager:
    """
    Trace view Manager object
    """

    PID_OFFSET = 10
    INDEX_OFFSET = 5
    HOST_ID_FOR_PID = 31

    @staticmethod
    def column_graph_trace(trace_header: list, trace_data: list) -> list:
        """
        Format column graph
        """
        result_data = [0] * len(trace_data)
        if not trace_data:
            return result_data
        try:
            for index, item_data in enumerate(trace_data):
                # name, ts, pid, args is required
                result_data_part = OrderedDict(list(zip(trace_header, item_data)))
                result_data_part['ph'] = 'C'
                result_data[index] = result_data_part
            return result_data
        except (OSError, SystemError, ValueError, TypeError, RuntimeError):
            return result_data
        finally:
            pass

    @staticmethod
    def time_graph_trace(trace_header: list, trace_data: list) -> list:
        """
        Format sequence diagram
        """
        result_data = [0] * len(trace_data)
        if not trace_data:
            return result_data
        try:
            for index, item_data in enumerate(trace_data):
                # name, pid, tid, ts, duration is required
                result_data_part = OrderedDict(list(zip(trace_header, item_data)))
                result_data_part['ph'] = 'X'
                result_data[index] = result_data_part
            return result_data
        except (OSError, SystemError, ValueError, TypeError, RuntimeError):
            return result_data
        finally:
            pass

    @staticmethod
    def metadata_event(meta_data: any) -> list:
        """
        Format metadata event
        """
        if not meta_data:
            return []
        result_data = [0] * len(meta_data)
        try:
            for index, item_data in enumerate(meta_data):
                item_data_list = list(item_data)
                # name, pid, tid, args is required
                if item_data_list[0] in ["process_sort_index", "thread_sort_index"]:
                    item_data_list[3] = OrderedDict([("sort_index", item_data_list[3])])
                elif item_data_list[0] in ["process_labels"]:
                    item_data_list[3] = OrderedDict([("labels", item_data_list[3])])
                else:
                    item_data_list[3] = OrderedDict([("name", str(item_data_list[3]))])
                result_data_part = OrderedDict(list(zip(TraceViewHeaderConstant.METADATA_HEAD, item_data_list)))
                result_data_part['ph'] = 'M'
                result_data[index] = result_data_part
            return result_data
        except (OSError, SystemError, ValueError, TypeError, RuntimeError):
            return result_data
        finally:
            pass

    @staticmethod
    def add_connect_start_point(data_dict: dict, data_list: list) -> list:
        """
        add connect start point
        :param data_dict: json_data_dict
        :param data_list: ge_data_list
        :return: None
        """
        connect_list = []
        start_time = float(data_dict.get('ts', '0'))
        end_time = start_time + float(data_dict.get('dur', '0'))
        while data_list:
            ts = float(InfoConfReader().time_from_host_syscnt(data_list[0].get('timestamp', 0)) / DBManager.NSTOUS)
            if start_time <= ts <= end_time:
                connect_dict = {
                    'name': 'acl_to_npu',
                    'ph': 's',
                    'cat': StrConstant.ASYNC_ACL_NPU,
                    'id': TraceViewManager.get_line_format_pid(
                        data_list[0].get('stream_id'), data_list[0].get('task_id'), data_list[0].get('batch_id')
                    ),
                    'pid': data_dict.get('pid'),
                    'tid': data_dict.get('tid'),
                    'ts': start_time,
                }
                connect_list.append(connect_dict)
            elif ts > end_time:
                break
            data_list.pop(0)
        return connect_list

    @staticmethod
    def add_connect_end_point(json_list: list) -> list:
        """
        add connect end point
        :param json_list: json_data_dict
        :return: None
        """
        if isinstance(json_list, list):
            for data_dict in json_list:
                args = data_dict.get('args', {})
                if not all(str(args.get(id, '')) for id in ('Stream Id', 'Task Id', 'Batch Id')):
                    continue
                connect_dict = {
                    'name': 'acl_to_npu',
                    'ph': 'f',
                    'id': TraceViewManager.get_line_format_pid(
                        args.get('Stream Id'), args.get('Task Id'), args.get('Batch Id')
                    ),
                    'cat': StrConstant.ASYNC_ACL_NPU,
                    'pid': data_dict.get('pid'),
                    'tid': data_dict.get('tid'),
                    'ts': data_dict.get('ts'),
                    'bp': 'e',
                }
                json_list.append(connect_dict)
        return json_list

    @staticmethod
    def get_format_pid(pid: int, layer_info: TraceViewHeaderConstant.LayerInfo) -> int:
        """
        get format_pid
        :param pid: int,
        :param layer_info: TraceViewHeaderConstant.LayerInfo
        :return: format_pid: Uint32: pid use high 22bit, index_id use middle 5bit, device_id use low 5bit
        ps: pid_max is 10^22 - 1
        """
        if layer_info.general_layer == TraceViewHeaderConstant.GENERAL_LAYER_DPU:
            device_id = pid
            pid = InfoConfReader().get_json_pid_data()
        elif layer_info.general_layer == TraceViewHeaderConstant.GENERAL_LAYER_CPU or not is_number(
            InfoConfReader().get_device_id()
        ):
            # host device_id is 31, we cannot use NumberConstant.HOST_ID,
            # cause this value is also been used in record time.
            device_id = TraceViewManager.HOST_ID_FOR_PID
        else:
            device_id = int(InfoConfReader().get_device_id())

        format_pid = (
            (pid << TraceViewManager.PID_OFFSET) | (layer_info.sort_index << TraceViewManager.INDEX_OFFSET) | device_id
        )
        return format_pid

    @staticmethod
    def get_device_id_from_format_pid(pid: int) -> int:
        """
        get device_id
        :param pid: int
        :return: device_id
        反向从pid中获取对应的device_id
        """
        return pid & ((1 << TraceViewManager.INDEX_OFFSET) - 1)

    @staticmethod
    def get_line_format_pid(stream_id: int, task_id: int, batch_id: int) -> int:
        """
        get format_pid
        :param stream_id: int, task_id: int, batch_id: int
        :return: format_pid: int
        """
        stream_id_pos = 32
        task_id_pos = 16
        format_pid = (stream_id << stream_id_pos) + (task_id << task_id_pos) + batch_id
        return format_pid
