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

# pylint: disable=duplicate-code

import logging
from collections import OrderedDict

from common_func.config_mgr import ConfigMgr
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import read_cpu_cfg
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from common_func.utils import Utils
from mscalculate.aic.aic_utils import AicPmuUtils
from mscalculate.aic.pmu_calculator import PmuCalculator
from viewer.calculate_rts_data import get_metrics_from_sample_config, get_limit_and_offset, create_metric_table


class MiniAicCalculator(PmuCalculator, MsMultiProcess):
    """
    class used to calculator aic pmu of mini
    """

    AI_CORE_TYPE = "ai_core"

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_range = sample_config.get(StrConstant.PARAM_ITER_ID)
        self.events_name_list = []
        self.conn = None
        self.cur = None
        self.metrics_head = []
        self.metrics_datas = []

    def calculate(self: any) -> None:
        """
        calculate for ai core
        :return:
        """
        self._parse_ai_core_pmu_event()
        freq = InfoConfReader().get_freq(StrConstant.AIC)
        self.metrics_head = get_metrics_from_sample_config(self._project_path)
        self.metrics_datas = self.get_metric_summary_data(freq)
        if not self.metrics_datas or not self.metrics_head:
            logging.warning("metrics data or head is empty, please check the query conditions.")

    def save(self: any) -> None:
        """
        save data for aic core
        :return:
        """
        try:
            self.create_metrics_summary_db()
            create_metric_table(self.conn, self.metrics_head, DBNameConstant.TABLE_METRIC_SUMMARY)
            DBManager.insert_data_into_table(self.conn, DBNameConstant.TABLE_METRIC_SUMMARY, self.metrics_datas)
        except ValueError:
            # 目前看只有create阶段会抛异常
            logging.warning("create metrics summary db failed")
        finally:
            DBManager().destroy_db_connect(self.conn, self.cur)

    def ms_run(self: any) -> None:
        """
        entrance for calculate ai core
        :return: None
        """
        if not DBManager.check_tables_in_db(
            PathManager.get_db_path(self._project_path, DBNameConstant.DB_RUNTIME), DBNameConstant.TABLE_EVENT_COUNTER
        ):
            logging.warning("unable to create metrics db, because it can't find the event_counter table")
            return
        config = ConfigMgr.read_sample_config(self._project_path)
        if config.get('ai_core_profiling_mode') == StrConstant.AIC_SAMPLE_BASED_MODE:
            return
        db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_METRICS_SUMMARY)
        if ProfilingScene().is_all_export() and DBManager.check_tables_in_db(
            db_path, DBNameConstant.TABLE_METRIC_SUMMARY
        ):
            logging.info(
                "The Table %s already exists in the %s, and won't be calculate again.",
                DBNameConstant.TABLE_METRIC_SUMMARY,
                DBNameConstant.DB_METRICS_SUMMARY,
            )
            return
        self.calculate()
        self.save()

    def create_metrics_summary_db(self: any) -> None:
        self.conn, self.cur = DBManager.create_connect_db(
            PathManager.get_db_path(self._project_path, DBNameConstant.DB_METRICS_SUMMARY)
        )
        if not self.conn or not self.cur:
            raise ValueError

    def get_metric_summary_data(self, freq: float) -> []:
        """
        get_metric_summary_data
        :return: []
        """
        have_step_info = not ProfilingScene().is_all_export()
        conn, curs = DBManager.check_connect_db_path(
            PathManager.get_db_path(self._project_path, DBNameConstant.DB_RUNTIME)
        )
        try:
            if not conn or not curs or not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_EVENT_COUNT):
                logging.warning("unable to get metrics data, because it can't find the event_count table")
                return []

            sql = self.get_metric_summary_sql(freq, have_step_info)
            device_id = InfoConfReader().get_device_id()
            if have_step_info:
                limit_and_offset = get_limit_and_offset(self._project_path, self._iter_range)
                if not limit_and_offset:
                    return []
                metric_results = DBManager.fetch_all_data(
                    curs, sql, (device_id, limit_and_offset[0], limit_and_offset[1])
                )
            else:
                metric_results = DBManager.fetch_all_data(curs, sql, (device_id,))
            return metric_results
        finally:
            DBManager.destroy_db_connect(conn, curs)

    def get_metric_summary_sql(self, freq: float, have_step_info: bool) -> str:
        algos = []
        field_dict = read_cpu_cfg(MiniAicCalculator.AI_CORE_TYPE, 'metrics')
        if field_dict is None:
            return ''
        metrics_res = []
        for metric in self.metrics_head:
            replaced_metric = metric.replace("(GB/s)", "(gb/s)")
            replaced_field = field_dict.get(replaced_metric, replaced_metric).replace("freq", str(freq))
            metrics_res.append((metric, replaced_field))
        field_dict = OrderedDict(metrics_res)
        for field in field_dict:
            algo = field_dict[field]
            algos.append(algo)
        algo_lst = Utils.generator_to_list("cast(" + algo + " as decimal(8,2)) " for algo in algos)
        sql = (
            "SELECT "
            + ",".join(algo_lst)
            + ", task_id, stream_id, '0', -1 as batch_id FROM EventCount where device_id=?"
        )
        if have_step_info:
            sql = (
                "SELECT " + ",".join(algo_lst) + ", task_id, stream_id, '0', -1 as batch_id "
                "FROM EventCount where device_id=? limit ? offset ?"
            )
        return sql

    def _parse_ai_core_pmu_event(self: any) -> None:
        """
        get ai_core_profiling_events from sample.json
        :return:None
        """
        config = ConfigMgr.read_sample_config(self._project_path)
        ai_core_profiling_events = config.get('ai_core_profiling_events', '')
        if ai_core_profiling_events:
            self.events_name_list = Utils.generator_to_list(
                AicPmuUtils.get_pmu_event_name(pmu_event) for pmu_event in ai_core_profiling_events.split(",")
            )
