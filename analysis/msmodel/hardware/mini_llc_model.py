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
from abc import ABC
from collections import namedtuple

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.utils import Utils
from msmodel.interface.base_model import BaseModel


class MiniLlcModel(BaseModel, ABC):
    """
    acsq task model class
    """

    FILE_NAME = os.path.basename(__file__)
    LLC_CAPACITY = 64.0
    PERCENTAGE = 100

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.events_name_list = []
        self.aic_profiling_events = None  # ai core pmu event
        self.aiv_profiling_events = None

    @staticmethod
    def calculate_read_bandwidth(llc_metric: list, start_time: int) -> tuple:
        """
        calculate read bandwidth metric data
        :param llc_metric: llc metric
        :param start_time: start timestamp
        :return:
        """
        read_hit_rate = []
        read_total = []
        read_hit = []
        try:
            read_hit_rate, read_total, read_hit = MiniLlcModel._calculate_read_bandwidth_helper(llc_metric, start_time)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ZeroDivisionError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        return read_hit_rate, read_total, read_hit

    @staticmethod
    def calculate_write_bandwidth(llc_metric: list, start_time: int) -> tuple:
        """
        calculate write bandwidth metric data
        :param llc_metric: llc metric
        :param start_time: start timestamp
        :return:
        """
        write_hit_rate = []
        write_total = []
        write_hit = []
        try:
            write_hit_rate, write_total, write_hit = MiniLlcModel._calculate_write_bandwidth_helper(
                llc_metric, start_time
            )
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ZeroDivisionError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        return write_hit_rate, write_total, write_hit

    @staticmethod
    def _read_bandwidth_helper(llc_metric: list) -> tuple:
        llc_read_total_second = []
        llc_read_hit_second = []
        for k, v in enumerate(llc_metric[1:]):
            llc_read_total_second_tmp = 0
            llc_read_hit_second_tmp = 0
            if v.timestamp - llc_metric[k].timestamp != 0:
                llc_read_total_second_tmp = (
                    (v.read_allocate + v.read_noallocate)
                    * MiniLlcModel.LLC_CAPACITY
                    / (v.timestamp - llc_metric[k].timestamp)
                    / (NumberConstant.KILOBYTE * NumberConstant.KILOBYTE)
                )
                llc_read_hit_second_tmp = (
                    v.read_hit
                    * MiniLlcModel.LLC_CAPACITY
                    / (v.timestamp - llc_metric[k].timestamp)
                    / (NumberConstant.KILOBYTE * NumberConstant.KILOBYTE)
                )
            llc_read_total_second.append(llc_read_total_second_tmp)
            llc_read_hit_second.append(llc_read_hit_second_tmp)
        return llc_read_total_second, llc_read_hit_second

    @staticmethod
    def _write_bandwidth_helper(llc_metric: list) -> tuple:
        llc_write_total_second = []
        llc_write_hit_second = []
        for k, v in enumerate(llc_metric[1:]):
            llc_write_total_tmp = 0
            llc_write_hit_tmp = 0
            if v.timestamp - llc_metric[k].timestamp != 0:
                llc_write_total_tmp = (
                    (v.write_allocate + v.write_noallocate)
                    * MiniLlcModel.LLC_CAPACITY
                    / (v.timestamp - llc_metric[k].timestamp)
                    / (NumberConstant.KILOBYTE * NumberConstant.KILOBYTE)
                )
                llc_write_hit_tmp = (
                    v.write_hit
                    * MiniLlcModel.LLC_CAPACITY
                    / (v.timestamp - llc_metric[k].timestamp)
                    / (NumberConstant.KILOBYTE * NumberConstant.KILOBYTE)
                )
            llc_write_total_second.append(llc_write_total_tmp)
            llc_write_hit_second.append(llc_write_hit_tmp)
        return llc_write_total_second, llc_write_hit_second

    @staticmethod
    def _calculate_read_bandwidth_helper(llc_metric: list, start_time: int) -> tuple:
        read_hit_rate = []
        read_total = []
        read_hit = []
        if llc_metric:
            llc_first = llc_metric[0]
            if llc_first.timestamp - start_time != 0:
                llc_read_total_first = [
                    (llc_first.read_allocate + llc_first.read_noallocate)
                    * MiniLlcModel.LLC_CAPACITY
                    / (llc_first.timestamp - start_time)
                    / (NumberConstant.KILOBYTE * NumberConstant.KILOBYTE)
                ]
                llc_read_hit_first = [
                    llc_first.read_hit
                    * MiniLlcModel.LLC_CAPACITY
                    / (llc_first.timestamp - start_time)
                    / (NumberConstant.KILOBYTE * NumberConstant.KILOBYTE)
                ]
            else:
                llc_read_total_first = [0]
                llc_read_hit_first = [0]

            for i in llc_metric:
                if i.read_allocate + i.read_noallocate != 0:
                    read_hit_rate.append(i.read_hit / (i.read_allocate + i.read_noallocate) * MiniLlcModel.PERCENTAGE)
                else:
                    read_hit_rate.append(0)
            llc_read_total_second, llc_read_hit_second = MiniLlcModel._read_bandwidth_helper(llc_metric)
            read_total = llc_read_total_first + llc_read_total_second
            read_hit = llc_read_hit_first + llc_read_hit_second
        result = (read_hit_rate, read_total, read_hit)
        return result

    @staticmethod
    def _calculate_write_bandwidth_helper(llc_metric: list, start_time: int) -> tuple:
        write_hit_rate = []
        write_total = []
        write_hit = []
        if llc_metric:
            llc_first = llc_metric[0]
            if llc_first.timestamp - start_time != 0:
                llc_write_total_first = [
                    (llc_first.write_allocate + llc_first.write_noallocate)
                    * MiniLlcModel.LLC_CAPACITY
                    / (llc_first.timestamp - start_time)
                    / (NumberConstant.KILOBYTE * NumberConstant.KILOBYTE)
                ]
                llc_write_hit_first = [
                    llc_first.write_hit
                    * MiniLlcModel.LLC_CAPACITY
                    / (llc_first.timestamp - start_time)
                    / (NumberConstant.KILOBYTE * NumberConstant.KILOBYTE)
                ]
            else:
                llc_write_total_first = [0]
                llc_write_hit_first = [0]
            for i in llc_metric:
                if i.write_allocate + i.write_noallocate != 0:
                    write_hit_rate.append(
                        i.write_hit / (i.write_allocate + i.write_noallocate) * MiniLlcModel.PERCENTAGE
                    )
                else:
                    write_hit_rate.append(0)
            llc_write_total_second, llc_write_hit_second = MiniLlcModel._write_bandwidth_helper(llc_metric)
            write_total = llc_write_total_first + llc_write_total_second
            write_hit = llc_write_hit_first + llc_write_hit_second
        result = (write_hit_rate, write_total, write_hit)
        return result

    def create_table(self: any) -> None:
        """
        create MiniLlc table
        """
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                self.drop_tab()
            table_map = "{0}Map".format(table_name)
            sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def drop_tab(self: any) -> None:
        """
        drop exists table
        :return: None
        """
        DBManager.execute_sql(self.conn, 'DROP TABLE IF EXISTS LLCOriginalData')
        DBManager.execute_sql(self.conn, 'DROP TABLE IF EXISTS LLCMetricData')
        DBManager.execute_sql(self.conn, 'DROP TABLE IF EXISTS LLCDsidData')
        DBManager.execute_sql(self.conn, 'DROP TABLE IF EXISTS LLCBandwidth')
        DBManager.execute_sql(self.conn, 'DROP TABLE IF EXISTS LLCCapacity')

    def flush(self: any, data_list: dict) -> None:
        """
        insert llc data into db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_LLC_ORIGIN, data_list.get('original_data'))
        self.insert_data_to_db(DBNameConstant.TABLE_MINI_LLC_METRICS, data_list.get('metric'))
        self.insert_data_to_db(DBNameConstant.TABLE_LLC_DSID, data_list.get('dsid'))

    def calculate(self: any, llc_profiling: str) -> None:
        """
        calculate llc
        :param llc_profiling: bandwidth or capacity
        :return:
        """
        self.calculate_bandwidth_data(llc_profiling)
        self.calculate_capacity_data(llc_profiling)

    def calculate_bandwidth_data(self: any, llc_profiling: str) -> None:
        """
        calculate llc bandwidth data order by timestamp
        :param llc_profiling: llc profiling mode
        :return:
        """
        if llc_profiling != StrConstant.LLC_BAND_ITEM:
            return
        device_sql = 'select distinct(device_id) from LLCMetricData where replayid=0;'
        device_list = DBManager.fetch_all_data(self.cur, device_sql)
        if device_list:
            self._calculate_bandwidth_data_helper(device_list)

    def get_bandwidth_insert_data(self: any, llc_metric: list, device_data: list) -> list:
        """
        zip read and write data
        :param llc_metric: llc metric
        :param device_data: device data
        :return:
        """
        timestamp = Utils.generator_to_list(i.timestamp for i in llc_metric)
        start_time = InfoConfReader().get_start_timestamp()
        read_hit_rate, read_total, read_hit = self.calculate_read_bandwidth(llc_metric, start_time)
        write_hit_rate, write_total, write_hit = self.calculate_write_bandwidth(llc_metric, start_time)

        insert_data = list(
            zip(device_data, timestamp, read_hit_rate, read_total, read_hit, write_hit_rate, write_total, write_hit)
        )
        return insert_data

    def calculate_capacity_data(self: any, llc_profiling: str) -> None:
        """
        calculate llc capacity data order by timestamp
        :param llc_profiling: llc profiling mode
        :return: None
        """
        try:
            if llc_profiling == StrConstant.LLC_CAPACITY_ITEM:
                device_sql = 'select distinct(device_id) from LLCDsidData where replayid=0;'
                device_list = DBManager.fetch_all_data(self.cur, device_sql)
                if device_list:
                    self._capacity_data_helper(device_list)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)

    def _capacity_data_helper(self: any, device_list: list) -> None:
        for device in device_list:
            core2cpu = cal_core2cpu(self.result_dir, device[0])
            ctrl_dsid_name = core2cpu.get('ctrlcpu', '')
            ai_dsid_name = core2cpu.get('aicpu', '')

            sql = (
                "select device_id,timestamp,"
                "({ctrl})*{LLC_CAPACITY}/({KILOBYTE}*{KILOBYTE}),"
                "({ai})*{LLC_CAPACITY}/({KILOBYTE}*{KILOBYTE})from LLCDsidData "
                "where device_id = ? and "
                "replayid=0".format(
                    ctrl="+".join(ctrl_dsid_name),
                    LLC_CAPACITY=self.LLC_CAPACITY,
                    KILOBYTE=NumberConstant.KILOBYTE,
                    ai="+".join(ai_dsid_name),
                )
            )
            dsid_data = DBManager.fetch_all_data(self.cur, sql, (device[0],))
            if dsid_data:
                insert_sql = 'insert into LLCCapacity values(?,?,?,?)'
                DBManager.executemany_sql(self.conn, insert_sql, dsid_data)

    def _calculate_bandwidth_data_helper(self: any, device_list: list) -> None:
        metric = namedtuple(
            'metric',
            [
                'timestamp',
                'read_allocate',
                'read_noallocate',
                'read_hit',
                'write_allocate',
                'write_noallocate',
                'write_hit',
            ],
        )
        for device in device_list:
            sql = (
                'select timestamp,read_allocate,read_noallocate,read_hit,'
                'write_allocate,write_noallocate,write_hit '
                'from LLCMetricData where device_id = ? and replayid=0;'
            )
            llc_data = DBManager.fetch_all_data(self.cur, sql, (device[0],))
            if llc_data:
                llc_metric = Utils.generator_to_list(metric._make(i) for i in llc_data)
                device_data = [device[0]] * len(llc_metric)
                try:
                    insert_data = self.get_bandwidth_insert_data(llc_metric, device_data)
                except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
                    logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
                    return
                insert_sql = 'insert into LLCBandwidth values(?,?,?,?,?,?,?,?)'
                DBManager.executemany_sql(self.conn, insert_sql, insert_data)


def check_device_cpu(cpu_num: str) -> str:
    for num in cpu_num.replace(' ', '').split(","):
        if not num.isdigit():
            logging.error("Invalid device cpu: %s", cpu_num)
            return ''
    return cpu_num


def cal_core2cpu(project_path: str, device_id: int) -> dict:
    """
    calculate CORE2CPU list through
    :param project_path: project path
    :param device_id: device id
    :return: dict for core to cpu
    """
    core2cpu = {}
    info_path = os.path.join(project_path, StrConstant.INFO_JSON + '.' + str(device_id))
    try:
        if os.path.isfile(info_path):
            ctrl_cpu = InfoConfReader().get_data_under_device("ctrl_cpu")
            ai_cpu = InfoConfReader().get_data_under_device("ai_cpu")
            ctrl_cpu = check_device_cpu(ctrl_cpu)
            ai_cpu = check_device_cpu(ai_cpu)
            core2cpu['ctrlcpu'] = Utils.generator_to_list("dsid{}".format(x) for x in ctrl_cpu.split(",") if ctrl_cpu)
            core2cpu['aicpu'] = Utils.generator_to_list("dsid{}".format(x) for x in ai_cpu.split(",") if ai_cpu)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError, AttributeError) as err:
        logging.error(err)

    if not core2cpu:
        core2cpu = {
            "ctrlcpu": ["dsid0", "dsid1", "dsid2", "dsid3"],
            "aicpu": ["dsid4", "dsid5", "dsid6", "dsid7"],
        }
    return core2cpu
