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
import sqlite3
from typing import List, Any

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from mscalculate.aic.aic_utils import AicPmuUtils
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.pmu_block_dto import PmuBlockDto
from viewer.calculate_rts_data import get_metrics_from_sample_config


class FftsPmuModel(ParserModel):
    """
    ffts pmu model.
    """

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.events_name_list = []
        self.profiling_events = {'aic': [], 'aiv': []}

    def create_table(self: any) -> None:
        """
        create aic and aiv table by sample.json
        :return:
        """
        # aic metrics table
        column_list = []
        self.profiling_events['aiv'] = get_metrics_from_sample_config(
            self.result_dir, StrConstant.AIV_PROFILING_METRICS
        )
        self.profiling_events['aic'] = get_metrics_from_sample_config(self.result_dir)
        self.update_pmu_list(column_list)
        self._create_metric_table_by_head(column_list, DBNameConstant.TABLE_METRIC_SUMMARY)

    def update_pmu_list(self: any, column_list: list) -> None:
        for core_type, pmu_list in self.profiling_events.items():
            core_column_list = AicPmuUtils.remove_unused_column(pmu_list)
            for column in core_column_list:
                column_list.append('{0}_{1}'.format(core_type, column))

    def flush(self: any, data_list: list) -> None:
        """
        insert data into database
        :param data_list: ffts pmu data list
        :return: None
        """
        if not data_list:
            logging.warning("ffts pmu data is empty, no data found.")
            return
        self.insert_data_to_db(DBNameConstant.TABLE_METRIC_SUMMARY, data_list)

    def _create_metric_table_by_head(self: any, metrics: list, table_name: str) -> None:
        """
        insert event value into metric op_summary
        """
        sql = 'CREATE TABLE IF NOT EXISTS {name}({column})'.format(
            column=','.join(metric.replace('(ms)', '').replace('(GB/s)', '') + ' numeric' for metric in metrics)
            + ', task_id INT, '
            'stream_id INT,'
            'subtask_id INT,'
            'task_type INT, '
            'start_time INT,'
            'end_time INT, '
            'ffts_type INT,'
            'core_type INT,'
            'batch_id INT',
            name=table_name,
        )
        try:
            DBManager.execute_sql(self.conn, sql)
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)


class V6PmuModel(FftsPmuModel):
    def __init__(
        self, result_dir: str, db_name: str = DBNameConstant.DB_METRICS_SUMMARY, table_list: List = None
    ) -> None:
        if table_list is None:
            table_list = []
        super().__init__(result_dir, db_name, table_list)

    def create_table(self: Any) -> None:
        super().create_table()
        self._create_block_pmu_time_table(DBNameConstant.TABLE_V6_BLOCK_PMU)

    def block_flush(self: Any, data_list: List) -> None:
        if not data_list:
            logging.warning("ffts pmu block data is empty, no data found.")
            return
        self.insert_data_to_db(DBNameConstant.TABLE_V6_BLOCK_PMU, data_list)

    def _create_block_pmu_time_table(self: Any, table_name: str) -> None:
        sql = (
            f'CREATE TABLE IF NOT EXISTS {table_name} (stream_id INT, task_id INT, '
            f'subtask_id INT, batch_id INT, start_time INT, duration INT, core_type INT, core_id INT)'
        )
        try:
            DBManager.execute_sql(self.conn, sql)
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)


class V6PmuViewModel(ViewModel):
    def __init__(self: Any, result_dir: str, *args, **kwargs) -> None:
        super().__init__(result_dir, DBNameConstant.DB_METRICS_SUMMARY, [DBNameConstant.TABLE_V6_BLOCK_PMU])

    def get_timeline_data(self: Any) -> list:
        sql = (
            f"SELECT stream_id, task_id, subtask_id, batch_id, start_time, duration, core_type, core_id "
            f"FROM {DBNameConstant.TABLE_V6_BLOCK_PMU}"
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=PmuBlockDto)
