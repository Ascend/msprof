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

import logging

from itertools import groupby
from operator import attrgetter
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from msconfig.config_manager import ConfigManager
from msmodel.interface.parser_model import ParserModel
from profiling_bean.db_dto.msproftx_dto import MsprofTxDto, MsprofTxExDto
from profiling_bean.db_dto.step_trace_dto import MsproftxMarkDto


class MsprofTxModel(ParserModel):
    """
    db operator for msproftx parser
    """

    TABLES_PATH = ConfigManager.TABLES

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:  # pylint: disable=W0246
        super().__init__(result_dir, db_name, table_list)

    def flush(self: any, data_list: list) -> None:
        """
        flush msproftx data to db
        :param data_list:msproftx data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_MSPROFTX, data_list)

    def get_timeline_data(self: any) -> list:
        """
        get timeline data
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_MSPROFTX):
            return []
        all_data_sql = (
            "select category, pid, tid, start_time, (end_time-start_time) as dur_time, payload_type, "
            "payload_value, message_type, message, event_type from `{table}`"
        ).format(table=DBNameConstant.TABLE_MSPROFTX)  # nosec B608
        return DBManager.fetch_all_data(self.cur, all_data_sql, dto_class=MsprofTxDto)

    def get_summary_data(self: any) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_MSPROFTX):
            return []
        all_data_sql = (
            "select pid, tid, category, event_type, payload_type, payload_value, start_time, "
            "end_time, message_type, message from `{table}`"
        ).format(table=DBNameConstant.TABLE_MSPROFTX)  # nosec B608
        return DBManager.fetch_all_data(self.cur, all_data_sql)


class MsprofTxExModel(ParserModel):
    """
    db operator for msproftx ex parser
    """

    def __init__(self: any, result_dir: str, db_name: str, table_list: list):
        super().__init__(result_dir, db_name, table_list)
        self.default_task_duration = 0

    def flush(self: any, data_list: list) -> None:
        """
        flush msproftx ex data to db
        :param data_list: msproftx ex data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_MSPROFTX_EX, data_list)

    def get_timeline_data(self) -> list:
        """
        get timeline data
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_MSPROFTX_EX):
            return []
        all_data_sql = (
            "select pid, tid, event_type, start_time, (end_time-start_time) as dur_time, "
            "mark_id, message, domain from `{table}`"
        ).format(table=DBNameConstant.TABLE_MSPROFTX_EX)  # nosec B608
        return DBManager.fetch_all_data(self.cur, all_data_sql, dto_class=MsprofTxExDto)

    def get_summary_data(self) -> list:
        """
        get timeline data
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_MSPROFTX_EX):
            return []
        all_data_sql = (
            "select pid, tid, event_type, start_time, end_time, message, domain, mark_id from `{table}`"
        ).format(table=DBNameConstant.TABLE_MSPROFTX_EX)  # nosec B608
        return DBManager.fetch_all_data(self.cur, all_data_sql)

    def get_device_data(self) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_STEP_TRACE):
            return []
        all_data_sql = (
            'select index_id, timestamp, stream_id, task_id, tag_id from `{table}` where tag_id = 11 or tag_id = 12'
        ).format(table=DBNameConstant.TABLE_STEP_TRACE)  # nosec B608
        task_list = DBManager.fetch_all_data(self.cur, all_data_sql, dto_class=MsproftxMarkDto)
        if not task_list:
            return []
        res_task_data = []

        major_version, minor_version = InfoConfReader().get_cann_version()
        # for cann version >= 9.1.0, tag id 11 is used for mark data, and tag id 12 is used for range data
        # so we handle these data differently according to cann version
        if (major_version, minor_version) >= (9, 1):
            range_data_list = []
            for task in task_list:
                if task.tag_id == 11:
                    res_task_data.append([task.index_id, task.timestamp, task.stream_id, task.task_id, 0])
                else:
                    range_data_list.append(task)
            range_data_list.sort(key=lambda x: (x.index_id, x.timestamp))
            for index_id, group in groupby(range_data_list, key=attrgetter('index_id')):
                iterms = list(group)
                for i in range(0, len(iterms) - 1, 2):
                    start_time = iterms[i].timestamp
                    end_time = iterms[i + 1].timestamp
                    duration = end_time - start_time
                    res_task_data.append([index_id, start_time, iterms[i].stream_id, iterms[i].task_id, duration])
                if len(iterms) % 2 != 0:
                    logging.warning("Unpaired range data with index_id %d, odd count %d.", index_id, len(iterms))
        else:
            task_list.sort(key=lambda x: (x.index_id, x.timestamp))
            res_task_data.append(
                [
                    task_list[0].index_id,
                    task_list[0].timestamp,
                    task_list[0].stream_id,
                    task_list[0].task_id,
                    self.default_task_duration,
                ]
            )
            for i in range(1, len(task_list)):
                if task_list[i].index_id == task_list[i - 1].index_id:
                    # set range data duration
                    res_task_data[-1][4] = task_list[i].timestamp - res_task_data[-1][1]
                else:
                    res_task_data.append(
                        [
                            task_list[i].index_id,
                            task_list[i].timestamp,
                            task_list[i].stream_id,
                            task_list[i].task_id,
                            self.default_task_duration,
                        ]
                    )

        return res_task_data
