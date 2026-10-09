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

import configparser
import logging
from typing import Any
from typing import List

from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.utils import Utils
from mscalculate.interface.icalculator import ICalculator
from mscalculate.l2_cache.l2_cache_metric import HitRateMetric
from mscalculate.l2_cache.l2_cache_metric import VictimRateMetric
from msconfig.config_manager import ConfigManager
from msmodel.l2_cache.soc_pmu_model import SocPmuCalculatorModel
from profiling_bean.prof_enum.data_tag import DataTag


class SocPmuCalculator(ICalculator, MsMultiProcess):
    """
    calculator for soc pmu
    """

    REQUEST_EVENTS = "request_events"
    HIT_EVENTS = "hit_events"
    MISS_EVENTS = "miss_events"
    NEGATIVE_STR = "-"
    NO_EVENT_LENGTH = 3
    SMMU_TYPE = 'SMMU'

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list.get(DataTag.SOC_PMU, [])
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._cfg_parser = configparser.ConfigParser(interpolation=None)
        self._model = SocPmuCalculatorModel(self._project_path)
        self._events = self._read_job_soc_pmu_events_info()
        self._platform_type = ""
        self._soc_pmu_ps_data = []
        self._soc_pmu_cal_data = []
        self._event_indexes = {}

    @staticmethod
    def _is_valid_index(index_dict: dict) -> bool:
        """
        check if event index info is valid
        """
        for index_slice in index_dict.values():
            if Constant.INVALID_INDEX_DICT in index_slice:
                return False
        return True

    def calculate(self: any) -> None:
        """
        run the data calculators
        """
        self._set_soc_pmu_events_indexes()
        if not self._check_event_indexes():
            return
        self._model.init()
        raw_soc_pmu_ps_data = self._model.get_all_data(DBNameConstant.TABLE_SOC_PMU)
        if not raw_soc_pmu_ps_data:
            logging.error("Soc pmu parser data is empty.")
            return
        self._soc_pmu_ps_data = self._model.split_events_data(raw_soc_pmu_ps_data)
        if self._soc_pmu_ps_data:
            self._cal_metrics()

    def save(self: any) -> None:
        """
        save data to database
        """
        if self._soc_pmu_cal_data and self._model:
            logging.info("Calculating soc pmu data finished, and starting to store result in db.")
            self._model.create_table()
            self._model.flush(self._soc_pmu_cal_data)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        if not self._file_list:
            return
        db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_SOC_PMU)
        if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_SOC_PMU_SUMMARY):
            logging.info(
                "The Table %s already exists in the %s, and won't be calculate again.",
                DBNameConstant.TABLE_SOC_PMU_SUMMARY,
                DBNameConstant.DB_SOC_PMU,
            )
            return
        logging.info("Start to calculate the data of soc pmu.")
        if not self._pre_check():
            return
        self.calculate()
        if self._soc_pmu_ps_data:
            self.save()

    def _read_job_soc_pmu_events_info(self: any) -> List[Any]:
        config = ConfigMgr.read_sample_config(self._project_path)
        events_from_config = config.get("npuEvents", "")
        if not events_from_config:
            return []
        events_cfg = events_from_config.split(';')
        events = []
        for cfg in events_cfg:
            key_value = cfg.split(':')
            if len(key_value) < 2:
                logging.error("npuEvents format is incorrect, npuEvents should be like npuEvents: SMMU:0x00,0x01")
                continue
            key = key_value[0].strip().upper()
            values = key_value[1].strip().split(',')
            if key == self.SMMU_TYPE:
                events = [value.strip().lower() for value in values]
                break
        return events

    def _get_event_index(self: any, event_type: str):
        abs_event_type = event_type[1:] if event_type.startswith(self.NEGATIVE_STR) else event_type
        if abs_event_type in self._events:
            return {
                "coefficient": 1 if abs_event_type == event_type else -1,
                "index": self._events.index(abs_event_type),
            }
        else:
            return Constant.INVALID_INDEX_DICT

    def _update_event_indexes_dict(self: any, used_event_type_list: list) -> None:
        for used_event_type in used_event_type_list:
            event_list = self._cfg_parser.get(self._platform_type, used_event_type).split(",")
            event_index_list = Utils.generator_to_list(self._get_event_index(event) for event in event_list)
            self._event_indexes.update({used_event_type: event_index_list})

    def _set_soc_pmu_events_indexes(self: any) -> None:
        if not self._platform_type:
            return
        used_event_type_list = [self.REQUEST_EVENTS, self.HIT_EVENTS, self.MISS_EVENTS]
        self._update_event_indexes_dict(used_event_type_list)

    def _check_event_indexes(self: any) -> bool:
        if not self._is_valid_index(self._event_indexes):
            logging.error(
                "Invalid soc pmu events, in platform: %s, excepted soc pmu events: %s, collected: %s",
                self._platform_type,
                str(self._cfg_parser.items(self._platform_type)),
                ",".join(self._events),
            )
            return False
        return True

    def _cal_metrics(self: any) -> None:
        """
        Calculate hit rate and miss rate.
        """

        def calculate_event_value(event_type, event_values):
            return sum(
                int(event_values[idx['index']]) * idx['coefficient'] for idx in self._event_indexes.get(event_type)
            )

        for i, events in enumerate(self._soc_pmu_ps_data):
            event_values = events[self.NO_EVENT_LENGTH :]
            request = calculate_event_value(self.REQUEST_EVENTS, event_values)
            hit = calculate_event_value(self.HIT_EVENTS, event_values)
            miss = calculate_event_value(self.MISS_EVENTS, event_values)

            hit_rate = HitRateMetric(hit, request).run_rules()
            miss_rate = VictimRateMetric(miss, request).run_rules()

            self._soc_pmu_cal_data.append(events[: self.NO_EVENT_LENGTH] + [hit_rate, miss_rate])

    def _pre_check(self: any) -> bool:
        """
        check if platform and soc pmu events in info.json is legal
        :return: basic info is legal for soc pmu calculating
        """
        self._platform_type = InfoConfReader().get_root_data(Constant.PLATFORM_VERSION)
        # get all maps between platforms and soc pmu events
        self._cfg_parser = ConfigManager.get(ConfigManager.SOC_PMU)
        if not self._cfg_parser.has_section(self._platform_type):
            logging.error("Invalid platform %s for soc pmu profiling", self._platform_type)
            return False
        # check if soc pmu info is legal
        if not self._events:
            logging.error("Soc pmu events is empty or format is incorrect.")
            return False
        if len(self._events) > Constant.SOC_PMU_ITEM:
            logging.error("Soc pmu events number should less than %s.", Constant.SOC_PMU_ITEM)
            return False
        if not set(self._events).issubset(Constant.SOC_PMU_EVENTS):
            logging.error("Soc pmu events value should be in %s", Constant.SOC_PMU_EVENTS)
            return False
        return True
