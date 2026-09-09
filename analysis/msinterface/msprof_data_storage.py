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

import json
import logging
import math
import os
import re

from common_func.common import warn
from common_func.constant import Constant
from common_func.file_manager import FdOpen
from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msvp_common import create_json
from common_func.file_manager import check_file_writable
from common_func.file_slice_helper import FileSliceHelper
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from profiling_bean.prof_enum.timeline_slice_strategy import LoadingTimeLevel
from profiling_bean.prof_enum.timeline_slice_strategy import TimeLineSliceStrategy


class MsprofDataStorage:
    """
    This class is used to slicing a timeline json file.
    """

    SLICE_CONFIG_PATH = FileSliceHelper.SLICE_CONFIG_PATH
    DATA_TO_FILE = 1024 * 1024
    DEFAULT_SETTING = FileSliceHelper.DEFAULT_SLICE_SETTING
    SETTING = None
    DEFAULT_SLICE_SIZE = 200 * 1024 * 1024  # MB
    MAX_JSON_FILE_SIZE = 20 * 1024 * 1024 * 1024  # 20G

    def __init__(self: any) -> None:
        self.tid_set = set()
        self.slice_config = None
        self.timeline_head = []
        self.slice_data = []
        self.data_list = None

    @staticmethod
    def slice_msprof_json_for_so(trace_file: str, params: dict) -> None:
        if not trace_file:
            return
        setting = MsprofDataStorage.SETTING
        if not setting:
            setting = MsprofDataStorage().read_slice_config()
            MsprofDataStorage.SETTING = setting
        slice_switch, limit_size, method = setting
        data_size = os.path.getsize(trace_file)
        if slice_switch == 'off':
            return
        if data_size < limit_size or data_size < MsprofDataStorage.DEFAULT_SLICE_SIZE:
            warn(
                MsProfCommonConstant.COMMON_FILE_NAME,
                "Data can be sliced only when the data size "
                "exceeds 200 MB. The current data size is less than 200 MB or the threshold you set in the "
                "configuration file, won't be sliced.",
            )
            return
        device_count = PathManager.get_device_count(params.get(StrConstant.PARAM_EXPORT_DUMP_FOLDER))
        with FileOpen(trace_file, 'r', MsprofDataStorage.MAX_JSON_FILE_SIZE) as fr:
            data = json.load(fr.file_reader)
        sliced_timeline_data = MsprofDataStorage().slice_data_list(data, device_count, data_size)
        MsprofDataStorage.write_json_files(sliced_timeline_data, params, False)
        os.remove(trace_file)

    @staticmethod
    def export_timeline_data_to_json(timeline_data: json, params: dict) -> str:
        """
        export data to json file
        :param timeline_data: export result
        :param params: params
        :return: result
        """
        if not timeline_data:
            return json.dumps(
                {
                    "status": NumberConstant.WARN,
                    "info": "Unable to get %s data. Maybe the data is not "
                    "collected, or the data may fail to be analyzed." % params.get(StrConstant.PARAM_DATA_TYPE),
                }
            )

        if 'status' in timeline_data:
            return json.dumps(timeline_data)
        device_count = PathManager.get_device_count(params.get(StrConstant.PARAM_RESULT_DIR))
        sliced_timeline_data = MsprofDataStorage().slice_data_list(timeline_data, device_count)
        error_code, data_path = MsprofDataStorage.write_json_files(sliced_timeline_data, params)
        if error_code:
            return json.dumps({"status": NumberConstant.ERROR, "info": "message error: %s" % data_path})
        return json.dumps({'status': NumberConstant.SUCCESS, 'data': data_path})

    @staticmethod
    def write_json_files(json_data: tuple, params: dict, is_clear: bool = True) -> tuple:
        """
        write json data  to file
        :param json_data:
        :param params:
        :param is_clear: whether clear timeline dir
        :return:
        """
        if is_clear:
            MsprofDataStorage.clear_timeline_dir(params)
        data_path = []
        for slice_time in range(len(json_data[1])):
            timeline_file_path = FileSliceHelper.make_export_file_name(params, slice_time, json_data[0])
            if os.path.exists(timeline_file_path):
                os.remove(timeline_file_path)
            try:
                with FdOpen(timeline_file_path) as trace_file:
                    trace_file.write(json.dumps(json_data[1][slice_time]))
                    data_path.append(timeline_file_path)
            except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
                logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
                return NumberConstant.ERROR, err
        return NumberConstant.SUCCESS, data_path

    @staticmethod
    def export_summary_data(headers: list, data: list, params: dict) -> any:
        """
        export data to csv file
        :param headers: header
        :param data: data
        :param params: params
        :return:
        """
        if headers and data:
            summary_file_path = FileSliceHelper.make_export_file_name(params)
            check_file_writable(summary_file_path)
            helper = FileSliceHelper(params, headers, data)
            if params.get(StrConstant.PARAM_EXPORT_FORMAT) == StrConstant.EXPORT_CSV:
                return helper.slice_and_dump_summary_data_as_csv()
            if params.get(StrConstant.PARAM_EXPORT_FORMAT) == StrConstant.EXPORT_JSON:
                return create_json(summary_file_path, headers, data, save_old_file=False)
        if data:
            return data
        return json.dumps(
            {
                "status": NumberConstant.WARN,
                "info": "Unable to get %s data. Maybe the data is not "
                "collected, or the data may fail to be analyzed." % params.get(StrConstant.PARAM_DATA_TYPE),
            }
        )

    @staticmethod
    def clear_timeline_dir(params: dict) -> None:
        timeline_dir = PathManager.get_timeline_dir(params.get(StrConstant.PARAM_RESULT_DIR))
        for file in os.listdir(timeline_dir):
            file_suffix = ''
            if params.get(StrConstant.PARAM_DEVICE_ID) is not None:
                file_suffix += "_" + str(params.get(StrConstant.PARAM_DEVICE_ID))
                if ProfilingScene().is_graph_export():
                    file_suffix += "_" + str(params.get(StrConstant.PARAM_MODEL_ID))
                if params.get(StrConstant.PARAM_ITER_ID) is not None:
                    file_suffix += "_" + str(params.get(StrConstant.PARAM_ITER_ID))
            if re.match(
                r'^{0}{1}(_slice_\d+)?.json'.format(params.get(StrConstant.PARAM_DATA_TYPE), file_suffix), file
            ):
                check_file_writable(os.path.join(timeline_dir, file))
                os.remove(os.path.join(timeline_dir, file))

    @staticmethod
    def _calculate_loading_time(row_line_level: int, count_line_level: int) -> float:
        """
        Calculate the approximate time
        :return:approximate time
        The basis of this formula:
        The number of rows is proportional to the time,
        And the number of data records is proportional to the slope.
        And the start value is proportional to the total number of records.
        """
        return (7 * count_line_level / 36000000 + 5.1) * row_line_level / 1000 + 9 * count_line_level / 2000000

    @staticmethod
    def _get_time_level(line_level: float) -> int:
        """
        judge loading time level
        :return: slice times in range [1,10,30]
        """
        time_level_list = [i.value for i in LoadingTimeLevel]
        for index, level in enumerate(time_level_list):
            if line_level < level:
                return time_level_list[index - 1] if index > 0 else time_level_list[index]
        return LoadingTimeLevel.BAD_LEVEL.value

    def init_params(self: any, data_list: list) -> None:
        """
        init data params
        :return: None
        """
        self.data_list = data_list
        self.data_list.sort(key=lambda x: float(x.get('ts', 0)))
        self.set_tid()
        self._update_timeline_head()

    def slice_data_list(self: any, data_list: list, device_count: int = 1, data_size: int = 0) -> tuple:
        """
        split data to slices
        return: tuple (slice count, slice data)
        """
        try:
            self.init_params(data_list)
        except (TypeError, ValueError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return False, [self.timeline_head + data_list]
        setting = MsprofDataStorage.SETTING
        if not setting:
            setting = self.read_slice_config()
            MsprofDataStorage.SETTING = setting
        slice_switch, limit_size, method = setting
        if slice_switch == 'off':
            return False, [self.timeline_head + data_list]
        slice_count = self.get_slice_times(limit_size, method, device_count, data_size)
        if not slice_count:
            return False, [self.timeline_head + data_list]
        slice_point = len(data_list) // slice_count
        slice_data = []
        for i in range(slice_count):
            slice_data.append(self.timeline_head + data_list[i * slice_point : (i + 1) * slice_point])
        return True, slice_data

    def set_tid(self: any):
        for data in self.data_list:
            pid = str(data.get(StrConstant.TRACE_HEADER_PID, ''))
            tid = str(data.get(StrConstant.TRACE_HEADER_TID, ''))
            if pid and tid:
                self.tid_set.add('{}-{}'.format(pid, tid))

    def read_slice_config(self: any) -> tuple:
        """
        read the configuration file
        :return: tuple
        :return: params: slice_switch slice switch
        :return: params: limit_size specifies the size of the split file. (MB)
        :return: params: slice_switch priority (0: granularity first,1: loading time first)
        """
        return FileSliceHelper.read_slice_config(self.SLICE_CONFIG_PATH)

    def get_slice_times(self: any, limit_size: int = 0, method: int = 0, device_count: int = 1, data_size: int = 0):
        """
        read the configuration file
        :return: slice times: int
        """
        list_length = len(self.data_list)
        str_length = data_size if data_size else len(json.dumps(self.data_list))
        # str_length / (list_length * 80) simplify formula
        coefficient = math.ceil(str_length / list_length / 80) if list_length else 0
        # If an exception occurs, continue the calculation logic.
        try:
            if isinstance(limit_size, int) and limit_size >= 200:
                str_size_of_mb = str_length // self.DATA_TO_FILE
                return 1 + str_size_of_mb // limit_size if str_size_of_mb > limit_size else 0
        except (TypeError, ValueError) as err:
            logging.warning(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        row_line_level = len(self.tid_set)
        formula = MsprofDataStorage._calculate_loading_time(row_line_level, list_length * coefficient)
        # if only host or one device dir
        time_level = self._get_time_level(formula * (1 + device_count // 2))
        slice_time = 2
        slice_method = LoadingTimeLevel.BAD_LEVEL.value
        if method == TimeLineSliceStrategy.LOADING_TIME_PRIORITY.value:
            slice_method = LoadingTimeLevel.FINE_LEVEL.value
        try:
            while time_level >= slice_method:
                if slice_time > list_length:
                    return 0
                slice_length = math.ceil(list_length / slice_time)
                formula = MsprofDataStorage._calculate_loading_time(row_line_level, slice_length * coefficient)
                time_level = self._get_time_level(formula * (1 + device_count // 2))
                slice_time += 1
        except ZeroDivisionError as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return 0
        return slice_time - 1 if slice_time > 2 else 0

    def _update_timeline_head(self: any) -> None:
        while self.data_list:
            if self.data_list[0].get('ph', '') == "M":
                self.timeline_head.append(self.data_list.pop(0))
            else:
                break
