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
import os
from datetime import datetime
from datetime import timezone

from common_func.constant import Constant
from common_func.file_manager import FdOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msvp_common import create_csv
from common_func.ms_constant.number_constant import NumberConstant
from common_func.profiling_scene import ProfilingScene


class FileSliceHelper:
    """
    This class is used to slicing file.
    """

    # csv count limit, less than 1000000
    CSV_LIMIT = 1000000
    # json file size less than 200M, 300 means a record is 300 Bytes
    JSON_LIMIT = 200 * 1024 * 1024 // 300
    COUNT_INIT = 0

    def __init__(self: any, params: dict, header: list, data_list: list) -> None:
        """
        target_dir: file target_dir
        export_type: summary or timeline
        """
        self.params = params
        self.header = header
        self.data_list = data_list
        # self.file_name_slice used to get the right file name
        self.file_name_slice = self.COUNT_INIT
        self.connection_id_set = set()

    @staticmethod
    def get_current_time_str() -> str:
        utc_time = datetime.now(tz=timezone.utc)
        current_time = utc_time.replace(tzinfo=timezone.utc).astimezone(tz=None)
        return current_time.strftime('%Y%m%d%H%M%S')

    @staticmethod
    def get_export_prefix_file_name(params: dict, slice_times: int = 0, slice_switch=False) -> str:
        file_name = params.get(StrConstant.PARAM_DATA_TYPE)
        if (
            params.get(StrConstant.PARAM_DEVICE_ID) is not None
            and params.get(StrConstant.PARAM_DEVICE_ID) != NumberConstant.HOST_ID
        ):
            file_name += "_" + str(params.get(StrConstant.PARAM_DEVICE_ID))
            if ProfilingScene().is_graph_export():
                file_name += "_" + str(params.get(StrConstant.PARAM_MODEL_ID))
            if params.get(StrConstant.PARAM_ITER_ID) is not None:
                file_name += "_" + str(params.get(StrConstant.PARAM_ITER_ID))
        if slice_switch:
            file_name += "_slice_{}".format(str(slice_times))
        date_str = FileSliceHelper.get_current_time_str()
        file_name += "_" + date_str
        return file_name

    @staticmethod
    def make_export_file_name(params: dict, slice_times: int = 0, slice_switch=False) -> str:
        result_path = params.get(StrConstant.PARAM_RESULT_DIR)
        dump_folder = params.get(StrConstant.PARAM_EXPORT_DUMP_FOLDER)
        file_name = FileSliceHelper.get_export_prefix_file_name(params, slice_times, slice_switch)
        file_suffix = ""

        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
            file_suffix = StrConstant.FILE_SUFFIX_CSV
            if params.get(StrConstant.PARAM_EXPORT_FORMAT) == StrConstant.EXPORT_JSON:
                file_suffix = StrConstant.FILE_SUFFIX_JSON
        elif params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            file_suffix = StrConstant.FILE_SUFFIX_JSON

        return os.path.join(result_path, dump_folder, file_name + file_suffix)

    def slice_and_dump_summary_data_as_csv(self):
        # to count slice in data_list
        slice_count = 0
        while len(self.data_list) >= ((slice_count + 1) * self.CSV_LIMIT):
            slice_switch = self.file_name_slice != self.COUNT_INIT
            csv_file = FileSliceHelper.make_export_file_name(self.params, self.file_name_slice, slice_switch)
            result_json = json.loads(
                create_csv(
                    csv_file,
                    self.header,
                    self.data_list[slice_count * self.CSV_LIMIT : (slice_count + 1) * self.CSV_LIMIT],
                    save_old_file=False,
                )
            )
            if result_json.get('status', NumberConstant.EXCEPTION) == NumberConstant.ERROR:
                return result_json
            slice_count += 1
            self.file_name_slice += 1

        self.data_list = self.data_list[slice_count * self.CSV_LIMIT :]
        csv_file = FileSliceHelper.make_export_file_name(
            self.params, self.file_name_slice, slice_switch=(self.file_name_slice != self.COUNT_INIT)
        )
        return create_csv(csv_file, self.header, self.data_list, save_old_file=False)

    def set_header(self, header: list):
        if not self.header:
            self.header = header

    def check_header_is_empty(self) -> bool:
        return not self.header

    def insert_data(self: any, data_list: list):
        if not data_list:
            return
        if self.params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
            self.data_list.extend(data_list)
            self.dump_csv_data()
        elif self.params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            self._pretreat_json_data(data_list)

    def dump_json_data(self, slice_index: int, is_need_slice: bool = False):
        if self.data_list:
            json_file = FileSliceHelper.make_export_file_name(self.params, slice_index, slice_switch=is_need_slice)
            self._create_json(json_file)

    def dump_csv_data(self, force: bool = False):
        if force and self.data_list:
            csv_file = FileSliceHelper.make_export_file_name(
                self.params, self.file_name_slice, slice_switch=(self.file_name_slice != self.COUNT_INIT)
            )
            create_csv(csv_file, self.header, self.data_list, use_dict=True)
            self.data_list = []
            return

        # to count slice in data_list
        slice_count = 0
        while len(self.data_list) >= ((slice_count + 1) * self.CSV_LIMIT):
            csv_file = FileSliceHelper.make_export_file_name(self.params, self.file_name_slice, slice_switch=True)
            create_csv(
                csv_file,
                self.header,
                self.data_list[slice_count * self.CSV_LIMIT : (slice_count + 1) * self.CSV_LIMIT],
                use_dict=True,
            )
            slice_count += 1
            self.file_name_slice += 1
        # clear used data to avoid oom
        self.data_list = self.data_list[slice_count * self.CSV_LIMIT :]

    def _pretreat_json_data(self, json_data: list):
        """
        data deduplication in cann level and get timeline header
        filter cann data by connection id
        """
        for data in json_data:
            if data.get('ph', '') == "M":
                self.header.append(data)
                continue
            if "@" in data.get("name"):
                connection_id = data.get("args", {"connection_id": None}).get("connection_id")
                if connection_id and connection_id in self.connection_id_set:
                    continue
                self.connection_id_set.add(connection_id)
            self.data_list.append(data)

    def _create_json(self, filename: str):
        try:
            with FdOpen(filename) as trace_file:
                trace_file.write(json.dumps(self.header + self.data_list))
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
