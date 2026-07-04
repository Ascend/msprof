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
from abc import ABC

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.msvp_common import float_calculate
from common_func.platform.chip_manager import ChipManager
from common_func.utils import Utils
from msconfig.config_manager import ConfigManager
from msmodel.interface.base_model import BaseModel


class LlcModel(BaseModel, ABC):
    """
    acsq task model class
    """

    TABLES_PATH = ConfigManager.TABLES_TRAINING
    EVENT_ITEM = 8
    EVENT_LIST = Utils.generator_to_list("event{}".format(i) for i in range(EVENT_ITEM))
    READ_PMU_LIST = {
        "event0": int('0x00', Constant.HEX_NUMBER),
        "event1": int('0x01', Constant.HEX_NUMBER),
        "event2": int('0x02', Constant.HEX_NUMBER),
        "event3": int('0x13', Constant.HEX_NUMBER),
        "event4": int('0x20', Constant.HEX_NUMBER),
        "event5": int('0x22', Constant.HEX_NUMBER),
        "event6": int('0x34', Constant.HEX_NUMBER),
        "event7": int('0x36', Constant.HEX_NUMBER),
    }
    WRITE_PMU_LIST = {
        "event0": int('0x00', Constant.HEX_NUMBER),
        "event1": int('0x01', Constant.HEX_NUMBER),
        "event2": int('0x03', Constant.HEX_NUMBER),
        "event3": int('0x14', Constant.HEX_NUMBER),
        "event4": int('0x21', Constant.HEX_NUMBER),
        "event5": int('0x23', Constant.HEX_NUMBER),
        "event6": int('0x35', Constant.HEX_NUMBER),
        "event7": int('0x37', Constant.HEX_NUMBER),
    }
    LLC_CACHE_SIZE = 64.0
    LLC_TO_SECOND = 10**6

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.l3_list = self._init_l3_list_dispatch()
        self.metrics_data = []

    @staticmethod
    def calculate_hit_rate(item: list) -> int:
        """
        Calculate hit rate metric
        :param item: hit item
        :return:
        """
        try:
            return float_calculate(
                [float_calculate([item[5], item[6], item[8], item[10]]), float_calculate([item[7], item[9]])], "/"
            )
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ZeroDivisionError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return Constant.DEFAULT_COUNT
        finally:
            pass

    @staticmethod
    def calculate_throughput(item: list, time_difference: str) -> str:
        """
        Calculate throughput
        :param item: llc item
        :param time_difference: detal time
        :return:
        """
        try:
            total_byte = float_calculate([item[3], item[4]], '+')
            return float_calculate(
                [
                    total_byte,
                    1 / NumberConstant.LLC_BYTE,
                    NumberConstant.KILOBYTE,
                    NumberConstant.KILOBYTE,
                    time_difference,
                ],
                '/',
            )
        except ZeroDivisionError as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return Constant.DEFAULT_COUNT
        finally:
            pass

    @staticmethod
    def _init_l3_list_dispatch() -> list:
        if ChipManager().is_chip_v1_1():
            llid_count = 1
        elif ChipManager().is_chip_v4():
            llid_count = 2
        else:
            llid_count = 4
        return list(range(llid_count))

    def flush(self: any, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_LLC_ORIGIN, data_list)

    def drop_tab(self: any) -> None:
        """
        drop exists table
        :return: None
        """
        DBManager.execute_sql(self.conn, 'DROP TABLE IF EXISTS {}'.format(DBNameConstant.TABLE_LLC_ORIGIN))
        DBManager.execute_sql(self.conn, 'DROP TABLE IF EXISTS {}'.format(DBNameConstant.TABLE_LLC_EVENTS))
        DBManager.execute_sql(self.conn, 'DROP TABLE IF EXISTS {}'.format(DBNameConstant.TABLE_LLC_METRICS))
        for event in self.EVENT_LIST:
            DBManager.execute_sql(
                self.conn, 'DROP TRIGGER IF EXISTS {}_{}'.format(DBNameConstant.TABLE_LLC_EVENTS, event)
            )

    def create_table(self: any) -> None:
        """
        create llc table and trigger
        :return: None
        """
        self.drop_tab()
        table_name = {
            DBNameConstant.TABLE_LLC_ORIGIN: None,
            DBNameConstant.TABLE_LLC_METRICS: None,
            DBNameConstant.TABLE_LLC_EVENTS: ["device_id", "l3tId", "timestamp"],
        }
        try:
            for name in table_name:
                sql = DBManager.sql_create_table_with_key(name + 'Map', name, self.TABLES_PATH, table_name.get(name))
                if not sql:
                    logging.error("generate sql statement failed!")
                    DBManager.destroy_db_connect(self.conn, None)
                    raise ProfException(ProfException.PROF_SYSTEM_EXIT)
                DBManager.execute_sql(self.conn, sql)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def insert_metrics_data(self: any) -> None:
        """
        Insert metrics value into metrics table
        :return: None
        """
        self.calculate_metrics()
        try:
            if self.metrics_data:
                sql = "INSERT INTO {0} VALUES ({1})".format(  # nosec B608
                    DBNameConstant.TABLE_LLC_METRICS, ",".join('?' for _ in self.metrics_data[0])
                )
                DBManager.executemany_sql(self.conn, sql, self.metrics_data)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            del self.metrics_data[:]

    def calculate_time_diff(self: any, time_start: int, time_stop: int) -> str:
        """
        Calculate time interval and transfer it to seconds.
        :param time_start: start timestamp
        :param time_stop: stop timestamp
        :return:
        """
        return (time_start - time_stop) / self.LLC_CACHE_SIZE / NumberConstant.NANO_SECOND

    def calculate_metrics(self: any) -> None:
        """
        Calculate llc hit rate and throughput.
        :return:
        """
        try:
            for llc_id in self.l3_list:
                sql = "SELECT * FROM {0} WHERE l3tId is {1} ORDER BY device_id, timestamp ".format(  # nosec B608
                    DBNameConstant.TABLE_LLC_EVENTS, llc_id
                )
                llc_event_data = DBManager.fetch_all_data(self.cur, sql)
                self._insert_llc_data(llc_event_data)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

    def create_events_trigger(self: any, llc_profiling: str) -> int:
        """
        Create sql trigger to update 'TABLE_LLC_EVENTS' by 'TABLE_LLC_ORIGIN'.
        :param llc_profiling: llc profiling mode
        :return:
        """
        if llc_profiling == "read":
            _pmu_list = self.READ_PMU_LIST
        elif llc_profiling == "write":
            _pmu_list = self.WRITE_PMU_LIST
        else:
            logging.error("Invalid llc_profiling option.")
            return NumberConstant.ERROR
        try:
            self._do_create_trigger(_pmu_list)
            return NumberConstant.SUCCESS
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return NumberConstant.ERROR

    def _insert_llc_data(self: any, llc_event_data: list) -> None:
        if llc_event_data:
            for item_index, item in enumerate(llc_event_data):
                tmp_hit_rate = self.calculate_hit_rate(item)
                time_difference = self.calculate_time_diff(
                    llc_event_data[item_index][2], llc_event_data[item_index - 1][2]
                )
                if item_index:
                    tmp_throughput = self.calculate_throughput(item, time_difference)
                else:
                    tmp_throughput = Constant.DEFAULT_COUNT
                self.metrics_data.append((item[0], item[1], item[2], tmp_hit_rate, tmp_throughput))
        else:
            logging.error("%s has no data.", DBNameConstant.TABLE_LLC_EVENTS)

    def _do_create_trigger(self: any, pmu_list: dict) -> None:
        for event_key in pmu_list:
            ignore_list = []
            for event_key_temp in self.EVENT_LIST:
                if event_key_temp == event_key:
                    ignore_list.append("new.counts")
                else:
                    ignore_list.append(
                        "(SELECT {0} FROM {1} WHERE device_id = new.device_id AND "  # nosec B608
                        "l3tId = new.l3tId AND timestamp = new.timestamp)".format(
                            event_key_temp, DBNameConstant.TABLE_LLC_EVENTS
                        )
                    )
            trigger_create = (
                "CREATE TRIGGER IF NOT EXISTS {1}_{0} AFTER INSERT ON {1} "
                "WHEN new.event={4} BEGIN INSERT OR REPLACE INTO {2} values "
                "(new.device_id, "
                "new.l3tId, "
                "new.timestamp, {3}); END".format(
                    event_key,
                    DBNameConstant.TABLE_LLC_ORIGIN,
                    DBNameConstant.TABLE_LLC_EVENTS,
                    ",".join(ignore_list),
                    pmu_list.get(event_key),
                )
            )
            DBManager.execute_sql(self.conn, trigger_create)
