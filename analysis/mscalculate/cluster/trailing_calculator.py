# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2025 Huawei Technologies Co.,Ltd.
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

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene


class TrailingCalculator:
    """
    calculate slow node message.
    """

    SLOW_THRESHOLD = 20

    def __init__(self: any, path_list: list) -> None:
        self._cluster_list = path_list
        self.trailing_dict = {}

    @staticmethod
    def get_step_data(data_path: str) -> list:
        sql_path = PathManager.get_db_path(data_path, DBNameConstant.DB_TRACE)
        conn, curs = DBManager.check_connect_db_path(sql_path)
        avg_time = []
        if DBManager.check_tables_in_db(sql_path, DBNameConstant.TABLE_TRAINING_TRACE):
            avg_time = DBManager.fetch_all_data(
                curs, "select avg(data_aug_bound) from {}".format(DBNameConstant.TABLE_TRAINING_TRACE)
            )
        DBManager().destroy_db_connect(conn, curs)
        return avg_time

    def run(self: any) -> dict:
        """
        entrance for calculating slow node
        :return: slow node list
        """
        for data_path in self._cluster_list:
            ProfilingScene().init(data_path)
            if ProfilingScene().is_operator():
                continue
            self.calculate(data_path)
        return self.calculate_slow_node()

    def calculate_slow_node(self: any) -> dict:
        slow_node_dict = {'Slow Node': []}
        if not self.trailing_dict:
            return slow_node_dict
        try:
            sum_time = sum(self.trailing_dict.values())
        except TypeError as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return slow_node_dict
        if not sum_time:
            return slow_node_dict
        try:
            avg_bound = round(sum_time / len(self.trailing_dict.values()), NumberConstant.ROUND_TWO_DECIMAL)
        except (ZeroDivisionError, TypeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return slow_node_dict
        try:
            for key, value in self.trailing_dict.items():
                ratio = round(
                    NumberConstant.PERCENTAGE * (value - avg_bound) / avg_bound, NumberConstant.ROUND_TWO_DECIMAL
                )
                if ratio > self.SLOW_THRESHOLD:
                    slow_node = (
                        'Node: {0}, with a data augmentation bound duration of {1} ns on average, '
                        'which is {2}% higher than the average duration consumed by nodes.\t'.format(
                            os.path.basename(key), str(round(value, NumberConstant.ROUND_TWO_DECIMAL)), str(ratio)
                        )
                    )
                    slow_node_dict.setdefault('Slow Node', []).append(slow_node)
        except (ZeroDivisionError, TypeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return slow_node_dict
        return slow_node_dict

    def calculate(self: any, data_path) -> None:
        """
        get and update data
        :return: None
        """
        time = self.get_step_data(data_path)
        if time:
            self.trailing_dict[data_path] = time[0][0]
