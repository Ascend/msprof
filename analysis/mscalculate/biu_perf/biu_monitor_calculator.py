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
from abc import abstractmethod

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.path_manager import PathManager
from msmodel.biu_perf.biu_perf_model import BiuPerfModel


class BiuMonitorCalculator:
    """
    calculate biu perf data
    """

    def __init__(self: any, sample_config: dict) -> None:
        self.sample_config = sample_config
        self.project_path = sample_config.get("result_dir")
        self.original_table_name = None
        self.table_name = None
        self.model = None
        self.data = []

    @staticmethod
    def group_by_core_id(monitor_data: list) -> dict:
        """
        group monitor data by core_type
        """
        monitor_data_dict = {}
        for monitor_datum in monitor_data:
            monitor_datum_list = monitor_data_dict.setdefault(monitor_datum.core_id, [])
            monitor_datum_list.append(monitor_datum)
        return monitor_data_dict

    @staticmethod
    @abstractmethod
    def get_unit_name(core_id: int, group_id: int, core_type: str) -> str:
        """
        get unit name
        """

    @staticmethod
    @abstractmethod
    def get_pid(core_id: int, group_id: int) -> int:
        """
        get pid
        """

    @staticmethod
    def get_ratio(cycle_num: int) -> float:
        """
        Calculate the ratio of cycles
        """
        return cycle_num / InfoConfReader().get_instr_profiling_freq()

    @staticmethod
    def get_interval(ratio: float) -> int:
        """
        get interval in timeline according to its ratio
        """
        return int(ratio * InfoConfReader().get_instr_profiling_freq())

    def calculate(self: any) -> None:
        """
        calculate biu cycle data
        :return: None
        """
        with self.model as _model:  # pylint: disable=not-context-manager
            monitor_data = _model.get_all_data(self.original_table_name)

        monitor_data_dict = self.group_by_core_id(monitor_data)

        for monitor_data in monitor_data_dict.values():
            for data_index, monitor_datum in enumerate(monitor_data):
                interval_start = data_index * InfoConfReader().get_instr_profiling_freq()
                unit_name = self.get_unit_name(monitor_datum.core_id, monitor_datum.group_id, monitor_datum.core_type)
                pid = self.get_pid(monitor_datum.core_id, monitor_datum.group_id)
                self.create_data(pid, interval_start, unit_name, monitor_datum)

    @abstractmethod
    def create_data(self: any, pid: int, interval_start: int, unit_name: str, monitor_datum: any) -> None:
        """
        create biu flow or biu cycles data
        """

    def save(self: any) -> None:
        """
        save calculate data to db
        :return: None
        """
        if not self.data:
            logging.warning("Biu flow data or biu cycles data list is empty!")
            return

        with self.model as _model:  # pylint: disable=not-context-manager
            self.model.create_table()
            _model.flush(self.table_name, self.data)

    def ms_run(self: any) -> None:
        """
        calculate for biu perf
        :return: None
        """
        db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_BIU_PERF)
        if DBManager.check_tables_in_db(db_path, self.table_name):
            logging.info(
                "The Table %s already exists in the %s, and won't be calculate again.",
                self.table_name,
                DBNameConstant.DB_BIU_PERF,
            )
            return
        self.calculate()
        self.save()


class MonitorFlowCalculator(BiuMonitorCalculator):
    def __init__(self: any, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.original_table_name = DBNameConstant.TABLE_FLOW_MONITOR
        self.table_name = DBNameConstant.TABLE_BIU_FLOW
        self.model = BiuPerfModel(self.project_path, [self.table_name])

    @staticmethod
    def get_unit_name(core_id: int, group_id: int, core_type: str) -> str:
        return "biu_group{}".format(group_id)

    @staticmethod
    def get_pid(core_id: int, group_id: int) -> int:
        return 4 * group_id + 4

    def create_data(self: any, pid: int, interval_start: int, unit_name: str, monitor_datum: any) -> None:
        flow_type_dict = {
            "Latency Read": monitor_datum.stat_rlat_raw,
            "Latency Write": monitor_datum.stat_wlat_raw,
            "Bandwidth Read": monitor_datum.stat_flux_rd,
            "Bandwidth Write": monitor_datum.stat_flux_wr,
        }

        for tid, flow_type in enumerate(flow_type_dict):
            self.data.append(
                [monitor_datum.timestamp, flow_type_dict.get(flow_type), flow_type, unit_name, pid, tid, interval_start]
            )


class MonitorCyclesCalculator(BiuMonitorCalculator):
    def __init__(self: any, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.original_table_name = DBNameConstant.TABLE_CYCLES_MONITOR
        self.table_name = DBNameConstant.TABLE_BIU_CYCLES
        self.model = BiuPerfModel(self.project_path, [self.table_name])

    @staticmethod
    def get_unit_name(core_id: int, group_id: int, core_type: str) -> str:
        return "{0}_core{1}_group{2}".format(core_type, core_id, group_id)

    @staticmethod
    def get_pid(core_id: int, group_id: int) -> int:
        # core is ai cube
        if core_id in range(0, 25):
            return 4 * group_id + 1

        # core is ai aiv0
        if core_id > 24 and core_id % 2 == 1:
            return 4 * group_id + 2

        # core is ai aiv1
        if core_id > 24 and core_id % 2 == 0:
            return 4 * group_id + 3

        logging.error("Core id %d is unknown.", core_id)
        return 0

    def create_data(self: any, pid: int, interval_start: int, unit_name: str, monitor_datum: any) -> None:
        cycles_type_dict = {
            "Vector": monitor_datum.vector_cycles,
            "Scalar": monitor_datum.scalar_cycles,
            "Cube": monitor_datum.cube_cycles,
            "Mte1": monitor_datum.lsu1_cycles,
            "Mte2": monitor_datum.lsu2_cycles,
            "Mte3": monitor_datum.lsu3_cycles,
        }

        for tid, cycle_type in enumerate(cycles_type_dict):
            cycle_num = cycles_type_dict.get(cycle_type)
            ratio = self.get_ratio(cycle_num)
            interval = self.get_interval(ratio)
            self.data.append(
                [monitor_datum.timestamp, interval, cycle_type, unit_name, cycle_num, ratio, pid, tid, interval_start]
            )
