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
import os
from collections import OrderedDict

from common_func.platform.ai_core_metrics_manager import AiCoreMetricsManager
from common_func.config_mgr import ConfigMgr
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.msvp_common import config_file_obj
from common_func.msvp_common import error
from common_func.msvp_common import read_cpu_cfg
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.utils import Utils
from mscalculate.aic.aic_utils import AicPmuUtils
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
from msmodel.interface.base_model import BaseModel
from viewer.calculate_rts_data import judge_custom_pmu_scene


class AiCoreSampleModel(BaseModel):
    """
    ffts pmu model.
    """

    FILE_NAME = os.path.basename(__file__)
    TYPE = "ai_core"
    PMU_LENGTH = 8
    CHIPV6_PMU_LENGTH = 10

    def __init__(self: any, result_dir: str, db_name: str, table_list: list, metric_type: str) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.metrics_data = []
        self.sample_config = ConfigMgr.read_sample_config(result_dir)
        self.metrics_type = metric_type
        self.conn = None
        self.cur = None

    @staticmethod
    def get_ai_core_event_chunk(event: list) -> list:
        """
        get ai core event chunk
        :param event: ai core event
        :return:
        """
        pmu_length = AiCoreSampleModel.CHIPV6_PMU_LENGTH if ChipManager().is_chip_v6() else AiCoreSampleModel.PMU_LENGTH
        ai_core_events = Utils.generator_to_list(event[i : i + pmu_length] for i in range(0, len(event), pmu_length))
        return ai_core_events

    @staticmethod
    def get_custom_pmu_metrics(custom_metric: str, metrics_list: list) -> list:
        """
        get custom pmu name list from sample json
        :param custom_metric: "Custom:0x10,0x20" for example
        :param metrics_list: metrics list
        """
        return metrics_list + AicPmuUtils.get_custom_pmu_events(custom_metric[7:])

    def init(self: any) -> bool:
        """
        create db and tables
        """

        self.conn, self.cur = DBManager.create_connect_db(PathManager.get_db_path(self.result_dir, self.db_name))
        if not (self.conn and self.cur):
            return False
        self.cur.execute("PRAGMA page_size=8192")
        self.cur.execute("PRAGMA journal_mode=WAL")
        return True

    def create_aicore_originaldatatable(self: any, table_name: str) -> None:
        """
        create table
        :param table_name:table name
        :return:
        """
        sql = DBManager.sql_create_general_table(table_name, 'AICoreOriginalData', self.TABLES_PATH)
        if not sql:
            logging.error("generate sql statement failed!")
            error(self.FILE_NAME, "generate sql statement failed!")
            DBManager.destroy_db_connect(self.conn, self.cur)
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        DBManager.execute_sql(self.conn, sql)

    def create_core_table(self: any, events: list, ai_core_data: list) -> None:
        """
        create ai event tables
        :param events: ai core events
        :param ai_core_data: ai core data
        :return:
        """
        if not DBManager.judge_table_exist(self.cur, 'AICoreOriginalData'):
            self.create_aicore_originaldatatable('AICoreOriginalDataMap')
        self.flush(ai_core_data)
        logging.info("start create ai core events tables")
        try:
            if self._init_ai_core_events_table(events):
                self._init_task_cyc_table()
                self._insert_event_table(events)
                logging.info('create event tables finished')
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err))

    def insert_metric_summary_table(self: any, freq: int or float, key: str) -> None:
        """
        insert metric summary table
        :param key: key of metric
        :param freq: Frequency
        :return:
        """
        try:
            metrics = self._get_metrics(key)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err))
            error(self.FILE_NAME, str(err))
            return
        if metrics:
            self._do_insert_metric_summary_table(freq, metrics)

    def insert_metric_value(self: any) -> int:
        """
        insert metric value
        :return:
        """
        metrics_config = read_cpu_cfg(self.TYPE, "formula")
        if judge_custom_pmu_scene(self.sample_config, metrics_type=self.metrics_type):
            pmu_list = self.get_custom_pmu_metrics(self.sample_config.get(self.metrics_type), [])
            custom_pmu = {custom_pmu: None for custom_pmu in pmu_list}
            metrics_config.update(custom_pmu)
        data = []
        if not metrics_config:
            return NumberConstant.ERROR
        sql = "CREATE TABLE IF NOT EXISTS MetricSummary (metric text, value numeric, coreid INT)"
        DBManager.execute_sql(self.conn, sql)
        core_id = DBManager.fetch_all_data(self.cur, "select distinct(coreid) from EventCount;")
        for core in core_id:
            data.extend(
                [key.replace("(gb/s)", "(GB/s)").replace("(kb)", "(KB)"), None, core[0]]
                for key in list(metrics_config.keys())
            )

        DBManager.insert_data_into_table(self.conn, "MetricSummary", data)
        return NumberConstant.SUCCESS

    def sql_insert_metric_summary_table(self: any, metrics: list, freq: float) -> str:
        """
        generate sql statement for inserting metric from EventCount
        :param metrics: metrics to be calculated
        :param freq: running frequency, which can be used to calculate aic metrics
        :return: merged sql sentence
        """
        algos = []
        cal = CalculateAiCoreData(self.result_dir)
        cal.add_fops_header(self.metrics_type, metrics)
        field_dict = read_cpu_cfg(self.TYPE, 'formula')
        res = []
        for metric in metrics:
            replaced_metric = metric.lower()
            field_val = field_dict.get(replaced_metric, replaced_metric).replace(
                '/block_num*((block_num+core_num-1)/core_num)', ''
            )
            res.append((replaced_metric, field_val))
        field_dict = OrderedDict(res)
        for field in field_dict:
            algo = field_dict[field]
            algo = algo.replace("freq", str(freq))
            algo = cal.update_fops_data(field, algo)
            algos.append(algo)

        sql = (
            "SELECT "
            + ",".join("cast(" + algo + " as decimal(8,2))" for algo in algos)
            + " FROM EventCount where coreid = ?"
        )
        if ChipManager().is_chip_v1_1():
            sql = self._adapt_register_change(sql)
        return sql

    def flush(self: any, data_list: list) -> None:
        """
        insert data into database
        :param data_list: ffts pmu data list
        :return: None
        """
        count_num = data_list[0][0]
        column = 'mode,replayid,timestamp,coreid,task_cyc,'
        column = column + ','.join('event{}'.format(i) for i in range(1, count_num + 1))
        sql = 'insert into AICoreOriginalData ({column}) values ({value})'.format(
            column=column, value='?,' * (4 + count_num) + '?'
        )
        DBManager.executemany_sql(self.conn, sql, Utils.generator_to_list(x[1:] for x in data_list))

    def clear(self: any) -> None:
        """
        clear ai core table
        :return: None
        """
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_METRICS_SUMMARY)
        if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_METRIC_SUMMARY):
            DBManager.drop_table(self.conn, DBNameConstant.TABLE_METRIC_SUMMARY)

    def _create_event_count_table(self: any, events: list) -> None:
        sql = (
            "CREATE TABLE IF NOT EXISTS EventCount ("
            + ",".join(pmu_event.replace('0x', 'r') + " numeric" for pmu_event in events)
            + ",task_cyc numeric,coreid INT)"
        )
        self.cur.execute(sql)
        self.conn.commit()

    def _init_ai_core_events_table(self: any, events: list) -> bool:
        self._create_event_count_table(events)
        ai_core_events = self.get_ai_core_event_chunk(events)
        if not ai_core_events:
            logging.error('get ai core pmu events failed!')
            return False
        for replay, event in enumerate(ai_core_events):
            for index, value in enumerate(event):
                DBManager.drop_table(self.conn, value.replace('0x', 'r'))
                DBManager.execute_sql(
                    self.conn,
                    "create table {tablename}(timestamp INTEGER, pmucount INTEGER, coreid INTEGER)".format(
                        tablename=value.replace('0x', 'r')
                    ),
                )
                sql = (
                    "insert into {tablename} select timestamp, event{index}, coreid "
                    "from AICoreOriginalData "
                    "where replayid = ?".format(
                        tablename=value.replace('0x', 'r'),
                        index=index + 1,
                    )
                )
                DBManager.execute_sql(self.conn, sql, (replay,))
        return True

    def _get_ai_core_event_sum(self: any, events: list) -> dict:
        event_sum = {}
        for i in events:
            result = self.cur.execute(
                "select sum(pmucount),coreid from {tablename} group by coreid order by coreid".format(
                    tablename=i.replace("0x", "r")
                )
            ).fetchall()
            for j in result:
                if j[-1] in event_sum:
                    event_sum.get(j[-1]).append(j[0])
                else:
                    event_sum[j[-1]] = [j[0]]
        result = self.cur.execute(
            "select sum(pmucount),coreid from task_cyc group by coreid order by coreid"
        ).fetchall()
        for j in result:
            if j[-1] in event_sum:
                event_sum.get(j[-1]).append(j[0])
            else:
                event_sum[j[-1]] = [j[0]]

        event_sum = {key: value + [key] for key, value in event_sum.items()}
        return event_sum

    def _init_task_cyc_table(self: any) -> None:
        self.cur.execute("create table task_cyc(timestamp INTEGER, pmucount INTEGER, coreid INTEGER)")
        sql = "insert into task_cyc select timestamp, task_cyc, coreid from AICoreOriginalData where replayid is 0"
        self.cur.execute(sql)

    def _insert_event_table(self: any, events: list) -> None:
        event_sum = self._get_ai_core_event_sum(events)
        sql = 'insert into EventCount values ({value})'.format(value="?," * (len(events)) + "?,?")
        self.cur.executemany(sql, list(event_sum.values()))
        if self.cur.execute("SELECT count(*) FROM sqlite_master WHERE type='table' AND name='r11'").fetchall()[0][0]:
            self.cur.execute("drop table if exists cycles")
            self.cur.execute("alter table r11 rename to cycles")
        self.conn.commit()

    def _do_insert_metric_summary_table(self: any, freq: int, metrics: list) -> None:
        logging.info('start insert into MetricSummary')
        core_id = DBManager.fetch_all_data(self.cur, "select distinct(coreid) from EventCount;")
        sql = self.sql_insert_metric_summary_table(metrics, freq)
        for core in core_id:
            result = DBManager.fetch_all_data(self.cur, sql, (core[0],))
            for row in result:
                index = 0
                for _metric in metrics:
                    value = row[index] if row[index] else 0
                    DBManager.execute_sql(
                        self.conn,
                        "UPDATE MetricSummary SET value=? WHERE metric=? and coreid=?",
                        (value, _metric, core[0]),
                    )
                    index += 1

    def _get_metrics(self: any, key: str) -> list:
        metrics = ["total_time(ms)"]
        if key.startswith("Custom"):
            return self.get_custom_pmu_metrics(key, metrics)
        if key not in AiCoreMetricsManager.AICORE_METRICS_LIST:
            return []
        sample_metrics = AiCoreMetricsManager.AICORE_METRICS_LIST.get(key)
        if not sample_metrics:
            return []
        sample_metrics_lst = sample_metrics.split(",")
        for _key in sample_metrics_lst:
            if _key.lower() not in (item[0] for item in config_file_obj(file_name='ai_core').items('formula')):
                message = f"Invalid metric {_key} ."
                raise ProfException(ProfException.PROF_SYSTEM_EXIT, message)
        metrics.extend(sample_metrics_lst)
        return metrics

    def _adapt_register_change(self, sql: str) -> str:
        """
        chip id 7, 8, 11 register is changed when the pmu type is MemoryUB, 0x1a5 is one of new register.
        In the above scene, special adaptation is required.
        """
        if self.sample_config.get("ai_core_metrics") == "MemoryUB" and "0x1a5" in self.sample_config.get(
            "ai_core_profiling_events"
        ):
            sql = sql.replace("r3e", "r1a6")
            sql = sql.replace("r3d", "r1a5")
            sql = sql.replace("r44", "r191")
            sql = sql.replace("SUM(r43)", "(SUM(r17f)+SUM(r180))")
        return sql
