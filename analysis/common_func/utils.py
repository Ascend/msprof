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
from typing import Callable
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from msmodel.interface.base_model import BaseModel


class Utils:
    """
    utils class
    """

    def __init__(self: any) -> any:
        pass

    @staticmethod
    def is_step_scene(result_dir: str) -> bool:
        """
        check the scene of step info or not
        :param result_dir: project path
        :return: True or False
        """
        _model = BaseModel(result_dir, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA])
        if not _model.check_table():
            return False

        sql = "select {col} from {table_name} where {col} != '{item}'".format(  # nosec B608
            table_name=DBNameConstant.TABLE_STEP_TRACE_DATA,
            col='model_id',
            item=Constant.GE_OP_MODEL_ID,
        )
        data = DBManager.fetch_one_data(_model.cur, sql)
        _model.finalize()
        return len(data) != 0

    @staticmethod
    def is_single_op_scene(result_dir: str) -> bool:
        """
        check the scene of single_op or not
        :param result_dir: project path
        :return: True or False
        """
        return not Utils.is_step_scene(result_dir)

    @staticmethod
    def is_single_op_graph_mix(result_dir: str) -> bool:
        """
        check the scene of single op graph mix or not
        :param result_dir: project path
        :return: True or False
        """

        _model = BaseModel(result_dir, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA])
        if not _model.check_table():
            return False
        sql = "select model_id from {}".format(DBNameConstant.TABLE_STEP_TRACE_DATA)  # nosec B608
        model_ids = DBManager.fetch_all_data(_model.cur, sql)
        _model.finalize()
        model_id_set = set(model_id[0] for model_id in model_ids)
        if len(model_id_set) == 1:
            return False
        if Constant.GE_OP_MODEL_ID in model_id_set:
            return True
        return False

    @staticmethod
    def get_scene(result_dir: str) -> str:
        """
        get the current scene
        :param result_dir:  project path
        :return: scene
        """
        if Utils.is_single_op_graph_mix(result_dir):
            return Constant.MIX_OP_AND_GRAPH
        if Utils.is_step_scene(result_dir):
            return Constant.STEP_INFO
        return Constant.SINGLE_OP

    @staticmethod
    def generator_to_list(gen: any) -> list:
        """
        convert generator to list
        :param gen : generator
        :return:
        """
        result = []
        try:
            for data in gen:
                result.append(data)
            return result
        except (TypeError,) as err:
            logging.error("Failed to convert generator to list. %s", err, exc_info=Constant.TRACE_BACK_SWITCH)
            return []

    @staticmethod
    def chunks(all_log_bytes: bytes, struct_size: int) -> any:
        """
        Yield successive n-sized chunks from all_log_bytes.
        :param all_log_bytes:
        :param struct_size:
        :return: generator
        """
        for i in range(0, len(all_log_bytes), struct_size):
            yield all_log_bytes[i : i + struct_size]

    @staticmethod
    def obj_list_to_list(data_list: any) -> list:
        """
        :return:
        """
        res = []
        for data in data_list:
            temp = []
            for value in data.__dict__.values():
                temp.append(value)
            res.append(temp)
        return res

    @staticmethod
    def dict_copy(ori_data: dict) -> dict:
        """
        ori_data copy
        """
        copy_dict = {}
        for key, val in ori_data.items():
            copy_dict[key] = val
        return copy_dict

    @staticmethod
    def get_json_data(info_json_path: str) -> list:
        """
        read json data from file
        :param info_json_path:info json path
        :return:
        """
        if not info_json_path or not os.path.exists(info_json_path) or not os.path.isfile(info_json_path):
            return []
        if os.path.getsize(info_json_path) > 1024 * 1024 * 1024:
            return []
        with FileOpen(info_json_path, "r", 1024 * 1024 * 1024) as json_reader:
            json_data = json_reader.file_reader.read()
            json_data = json.loads(json_data)
            return json_data

    @staticmethod
    def is_valid_num(data: float) -> bool:
        """check if a num is valid"""
        if data is None or (not isinstance(data, (int, float)) and not data.isdigit()):
            return False
        return True

    @staticmethod
    def cal_total_time(total_cycle: int, freq: int, block_num: int, core_num: int) -> float:
        if not all([block_num, core_num, freq]):
            return 0
        total_time = (
            total_cycle * 1000 * NumberConstant.NS_TO_US / freq / block_num * ((block_num + core_num - 1) // core_num)
        )
        return round(total_time, NumberConstant.ROUND_TWO_DECIMAL)

    @classmethod
    def need_all_model_in_one_iter(cls: any, result_dir: str, model_id: int) -> bool:
        """
        check the scene of need export all model in one iter or not
        :param result_dir:
        :param model_id:
        :return:
        """
        if cls.is_single_op_scene(result_dir):
            return True
        if cls.is_single_op_graph_mix(result_dir) and model_id == Constant.GE_OP_MODEL_ID:
            return True
        return False

    @classmethod
    def get_func_type(cls: any, header: int) -> str:
        """
        func type is the lower 6 bits of the first byte, used to distinguish different data types.
        keep 6 lower bits which represent func type
        :param header:
        :return: func_type.For example: 000011
        """
        return bin(header & 63)[2:].zfill(6)

    @classmethod
    def get_stream_id(cls: any, stream_id: int) -> int:
        """
        To get stream id, Highest bit needed to be masked
        stream id max_value 2047 = 0b11111111111
        :param stream_id:
        :return: Highest bit mask stream id .For example: 160 -> 32
        """
        return stream_id % (1 << 11)

    @classmethod
    def get_cnt(cls: any, header: int) -> str:
        """
        cnt is the 7 - 10 bits of the first byte.
        keep 7 - 10 bits which represent cnt
        :param header:
        :return: func_type.For example: 1111
        """
        return bin(header & 960)[2:].zfill(6)

    @classmethod
    def get_aicore_type(cls: any, sample_config: dict) -> str:
        """
        get ai core type
        :return:
        """
        return sample_config.get(StrConstant.AICORE_PROFILING_MODE)

    @classmethod
    def percentile(cls: any, data_list: list, percent: float, key=int) -> float:
        """
        Find the percentile of a list of values.
        param data_list - is a list of values. Note data_list MUST BE already sorted ascending.
        param percent - a float value from 0.0 to 1.0.
        param key - optional key function to compute value from each element of N.

        return - the percentile of the values
        """
        if not data_list:
            return 0
        if percent < 0 or percent > 1:
            return 0
        rank = (len(data_list) - 1) * percent
        return data_list[key(rank)]

    @classmethod
    def filter_data_by_start_time_condition(
        cls: any, data_list: list, start_time: str, data_time_fetcher: Callable
    ) -> list:
        """
        filter datd by start time condition
        :param data_list:
        :param start_time:
        :param data_time_fetcher:  (task_start_time, task_end_time)
        :return:
        """
        filtered_data = [0] * len(data_list)
        _index = 0
        for item in data_list:
            task_start, task_end = data_time_fetcher(item)
            if task_start > start_time or task_start < start_time < task_end:
                filtered_data[_index] = item
                _index += 1
        filtered_data = filtered_data[:_index]
        return filtered_data

    @classmethod
    def get_valid_int_data(cls: any, int_data: int) -> int:
        return NumberConstant.INT64_MAX if int_data > NumberConstant.INT64_MAX else int_data
