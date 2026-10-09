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
from msmodel.l2_cache.l2_cache_calculator_model import L2CacheCalculatorModel
from profiling_bean.prof_enum.data_tag import DataTag


class L2CacheCalculator(ICalculator, MsMultiProcess):
    """
    calculator for l2 cache
    """

    REQUEST_EVENTS = "request_events"
    HIT_EVENTS = "hit_events"
    VICTIM_EVENTS = "victim_events"
    NEGATIVE_STR = "-"
    NO_EVENT_LENGTH = 3

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._cfg_parser = configparser.ConfigParser(interpolation=None)
        self._model = L2CacheCalculatorModel(self._project_path)
        self._l2_cache_events = self._read_job_l2_cache_events_info()
        self._platform_type = ""
        self._l2_cache_ps_data = []
        self._l2_cache_cal_data = []
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
        # get platform l2 cache indexes from config file
        self._set_l2_cache_events_indexes()
        if not self._check_event_indexes():
            return
        self._model.init()
        raw_l2_cache_ps_data = self._get_l2_cache_ps_data()
        if not raw_l2_cache_ps_data:
            logging.error("l2 cache parser data is empty")
            return
        self._l2_cache_ps_data = self._model.split_events_data(raw_l2_cache_ps_data)
        if self._l2_cache_ps_data:
            self._cal_metrics()

    def save(self: any) -> None:
        """
        save data to database
        """
        if self._l2_cache_cal_data and self._model:
            logging.info("calculating l2 cache data finished, and starting to store result in db.")
            self._model.create_table()
            self._model.flush(self._l2_cache_cal_data)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        if not self._file_list.get(DataTag.L2CACHE, []) and not self._file_list.get(DataTag.SOC_PMU, []):
            return
        db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_L2CACHE)
        if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_L2CACHE_SUMMARY):
            logging.info(
                "The Table %s already exists in the %s, and won't be calculate again.",
                DBNameConstant.TABLE_L2CACHE_SUMMARY,
                DBNameConstant.DB_L2CACHE,
            )
            return
        logging.info("start to calculate the data of l2 cache")
        if not self._pre_check():
            return
        self.calculate()
        if self._l2_cache_ps_data:
            self.save()

    def _read_job_l2_cache_events_info(self: any) -> list:
        config = ConfigMgr.read_sample_config(self._project_path)
        events_from_config = config.get("l2CacheTaskProfilingEvents", "").split(",")
        l2_cache_events = Utils.generator_to_list(event.strip().lower() for event in events_from_config)
        return l2_cache_events

    def _get_l2_cache_ps_data(self: any) -> list:
        raw_l2_cache_ps_data = self._model.get_all_data(DBNameConstant.TABLE_L2CACHE_PARSE)
        return raw_l2_cache_ps_data

    def _get_event_index(self: any, event_type: str):
        abs_event_type = event_type[1:] if event_type.startswith(self.NEGATIVE_STR) else event_type
        if abs_event_type not in self._l2_cache_events:
            return Constant.INVALID_INDEX_DICT
        else:
            return {
                "coefficient": 1 if abs_event_type == event_type else -1,
                "index": self._l2_cache_events.index(abs_event_type),
            }

    def _update_event_indexes_dict(self: any, used_event_type_list: list) -> None:
        for used_event_type in used_event_type_list:
            event_list = self._cfg_parser.get(self._platform_type, used_event_type).split(",")
            event_index_list = Utils.generator_to_list(self._get_event_index(event) for event in event_list)
            self._event_indexes.update({used_event_type: event_index_list})

    def _set_l2_cache_events_indexes(self: any) -> None:
        if not self._platform_type:
            return
        used_event_type_list = [self.REQUEST_EVENTS, self.HIT_EVENTS, self.VICTIM_EVENTS]
        self._update_event_indexes_dict(used_event_type_list)

    def _check_event_indexes(self: any) -> bool:
        if not self._is_valid_index(self._event_indexes):
            logging.error(
                "invalid l2 cache events, in platform: %s, excepted l2 cache events: %s, collected: %s",
                self._platform_type,
                str(self._cfg_parser.items(self._platform_type)),
                ",".join(self._l2_cache_events),
            )
            return False
        return True

    def _cal_metrics(self: any) -> None:
        """
        calculate hit rate and victim rate.
        """
        for _index, l2_cache_event_item in enumerate(self._l2_cache_ps_data):
            l2_cache_event_item = l2_cache_event_item[self.NO_EVENT_LENGTH :]
            request_event_value = sum(
                int(l2_cache_event_item[idx['index']]) * idx['coefficient']
                for idx in self._event_indexes.get(self.REQUEST_EVENTS)
            )
            hit_event_value = sum(
                int(l2_cache_event_item[idx['index']]) * idx['coefficient']
                for idx in self._event_indexes.get(self.HIT_EVENTS)
            )
            victim_event_value = sum(
                int(l2_cache_event_item[idx['index']]) * idx['coefficient']
                for idx in self._event_indexes.get(self.VICTIM_EVENTS)
            )
            # calculate hit rate and victim rate
            hit_rate = HitRateMetric(hit_event_value, request_event_value).run_rules()
            victim_rate = VictimRateMetric(victim_event_value, request_event_value).run_rules()

            tmp_list = self._l2_cache_ps_data[_index][: self.NO_EVENT_LENGTH]
            tmp_list.extend([hit_rate, victim_rate])
            self._l2_cache_cal_data.append(tmp_list)

    def _pre_check(self: any) -> bool:
        """
        check if platform and l2 cache events in info.json is legal
        :return: basic info is legal for l2 cache calculating
        """
        # get job platform info from info.json
        self._platform_type = InfoConfReader().get_root_data(Constant.PLATFORM_VERSION)

        # get all maps between platforms and l2 cache events
        self._cfg_parser = ConfigManager.get("L2CacheConfig")
        if not self._cfg_parser.has_section(self._platform_type):
            logging.error("invalid platform %s for l2 cache profiling", self._platform_type)
            return False

        # check if l2 cache info is legal
        if not self._l2_cache_events:
            return False
        if len(self._l2_cache_events) > Constant.L2_CACHE_ITEM:
            logging.error("Option --L2_cache_events number should less than %s.", Constant.L2_CACHE_ITEM)
            return False
        if not set(self._l2_cache_events).issubset(Constant.L2_CACHE_EVENTS):
            logging.error("Option --L2_cache_events value should be in %s", Constant.L2_CACHE_EVENTS)
            return False
        return True
