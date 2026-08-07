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

# pylint: disable=duplicate-code
# chip6从ffts分叉而来，框架层(ms_run/parse/_parse_by_iter/_parse_binary_file)存在天然结构重复

import itertools
import logging
import os
import sqlite3
import struct
from collections import deque
from collections import namedtuple

from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.stars_constant import StarsConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msprof_object import CustomizedNamedtupleFactory
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from common_func.utils import Utils
from framework.offset_calculator import FileCalculator
from framework.offset_calculator import OffsetCalculator
from mscalculate.aic.aic_utils import AicPmuUtils
from mscalculate.aic.pmu_calculator import PmuCalculator
from mscalculate.ascend_task.host_task_collector import HostTaskCollector
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
from mscalculate.stars.ffts_pmu_calculator import PmuMetrics
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from msmodel.stars.ffts_pmu_model import V6PmuModel
from msmodel.stars.low_power_model import LowPowerViewModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.stars.pmu_bean_v6 import PmuBeanV6
from viewer.calculate_rts_data import get_metrics_from_sample_config
from viewer.calculate_rts_data import judge_custom_pmu_scene


class Chip6PmuCalculator(PmuCalculator, MsMultiProcess):
    """
    只有MIX类型的算子才会同时有AIC和AIV的数据，MIX算子在AIC和AIV上都会跑，先跑的叫主核，后跑的叫从核
        - v6芯片：不论开不开启block模式，主核都会上报1条context_pmu数据。
          开启block模式，主核和从核会分别上报一条context pmu数据，以及若干条Block pmu数据。
          不开启block模式，主核和从核会分别上报一条context pmu数据。
    """

    AIC_CORE_TYPE = 0
    AIV_CORE_TYPE = 1
    FFTS_PMU_SIZE = 128
    CHIPV6_PMU_LENGTH = 10
    STREAM_TASK_CONTEXT_KEY_FMT = "{0}-{1}-{2}"

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        project_path = self.sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_model = HwtsIterModel(project_path)
        self._iter_range = self.sample_config.get(StrConstant.PARAM_ITER_ID)
        self._data_list_dict = {}
        self._sample_json = ConfigMgr.read_sample_config(project_path)
        self._file_list = file_list.get(DataTag.FFTS_PMU, [])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        self._wrong_func_type_count = 0
        self._overflow_count = 0
        self._first_ov_task = None
        self._set_pmu_events()
        self._model = V6PmuModel(project_path)
        self._aic_calculator = CalculateAiCoreData(project_path)
        self._aic_table_name_list = []
        self._aiv_table_name_list = []
        self._data_name = []
        self.pmu_data = []
        self.block_pmu_data = []  # for chip v6
        self._logged_func_types: set = set()
        self.freq_data = []

    @staticmethod
    def _get_total_cycle_and_pmu_data(data: any, use_actual_data: bool) -> tuple:
        """
        default value for pmu cycle list can be set to zero.
        """
        return (data.total_cycle, data.pmu_list) if use_actual_data else (0, [0] * Chip6PmuCalculator.CHIPV6_PMU_LENGTH)

    def ms_run(self: any) -> None:
        config = ConfigMgr.read_sample_config(self.sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH))
        if (
            not self._file_list
            or config.get(StrConstant.AICORE_PROFILING_MODE) == StrConstant.AIC_SAMPLE_BASED_MODE
            or config.get(StrConstant.AIV_PROFILING_MODE) == StrConstant.AIC_SAMPLE_BASED_MODE
        ):
            return
        self.init_params()
        self.parse()
        self.calculate()
        self.save()

    def parse(self: any) -> None:
        if ProfilingScene().is_all_export():
            db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_METRICS_SUMMARY)
            if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_METRIC_SUMMARY):
                logging.info(
                    "The Table %s already exists in the %s, and won't be calculate again.",
                    DBNameConstant.TABLE_METRIC_SUMMARY,
                    DBNameConstant.DB_METRICS_SUMMARY,
                )
                return
            self._parse_all_file()
        else:
            self._parse_by_iter()
        self._set_stream_id_by_host()
        if self._first_ov_task:
            logging.warning(
                "An operator overflow has been detected. The first operator is "
                "(stream id = %d, task id = %d), with a total of %d overflowed operators."
                "Total_cycle value is invalid!",
                self._first_ov_task[0],
                self._first_ov_task[1],
                self._overflow_count,
            )

    def calculate(self: any) -> None:
        """
        calculate parser data
        :return: None
        """
        if not self._data_list_dict.get(StrConstant.CONTEXT_PMU_TYPE):
            logging.warning("No ai core pmu data found, data list is empty!")
            return
        if self._wrong_func_type_count:
            logging.warning("Some PMU data fails to be parsed, err count: %s", self._wrong_func_type_count)
        self.get_block_pmu_data()
        self._set_table_name_list()
        self.freq_data = self._get_low_power_freq_data()
        self.pmu_data = self.calculate_mix_pmu_list()

    def save(self: any) -> None:
        """
        save parser data to db
        :return: None
        """
        try:
            with self._model as _model:
                _model.flush(self.pmu_data)
                _model.block_flush(self.block_pmu_data)
        except sqlite3.Error as err:
            logging.error("Save chip6 pmu data failed! %s", err)

    def calculate_mix_pmu_list(self: any) -> list:
        """
        当MIX场景可能存在时，计算pmu数据
        :return: 计算后的pmu数据列表
        """
        context_data_list = self._data_list_dict.get(StrConstant.CONTEXT_PMU_TYPE, [])
        main_core_data_list, slave_core_data_dict = self.get_main_slave_data(context_data_list)
        pmu_data_type = None
        pmu_data_list = []
        for index, data in enumerate(main_core_data_list):
            task_type = 0 if data.is_aic_data() else 1
            data_key = self.STREAM_TASK_CONTEXT_KEY_FMT.format(data.stream_id, data.task_id, data.subtask_id)
            slave_core_data_list = slave_core_data_dict.get(data_key, [])
            if data.is_mix_data() and slave_core_data_list:
                slave_core_data = slave_core_data_list.popleft()
                aic_pmu_value, aiv_pmu_value, aic_total_cycle, aiv_total_cycle = self.add_ctx_mix_pmu_to_context_pmu(
                    data, slave_core_data
                )
                aic_total_time, aiv_total_time = self._calculate_total_time_for_mix(data, slave_core_data)
            else:
                aic_pmu_value, aiv_pmu_value, aic_total_cycle, aiv_total_cycle = (
                    self.get_pmu_value_and_total_cycle_for_no_mix(data)
                )
                aic_total_time, aiv_total_time = self._calculate_total_time(data)
            metrics_type = self._sample_json.get('ai_core_metrics')
            aic_pmu_value = self._aic_calculator.add_pipe_time(aic_pmu_value, aic_total_time, metrics_type)
            aiv_pmu_value = self._aic_calculator.add_pipe_time(aiv_pmu_value, aiv_total_time, metrics_type)
            aic_pmu_value_list = self.process_pmu_values(aic_pmu_value, self._aic_table_name_list)
            aiv_pmu_value_list = self.process_pmu_values(aiv_pmu_value, self._aiv_table_name_list)
            if not pmu_data_type:
                aic_pmu_name = [f"aic_{i}" for i in range(len(aic_pmu_value_list))]
                aiv_pmu_name = [f"aiv_{i}" for i in range(len(aiv_pmu_value_list))]
                self._data_name = [
                    "aic_total_time",
                    "aic_total_cycle",
                    *aic_pmu_name,
                    "aiv_total_time",
                    "aiv_total_cycle",
                    *aiv_pmu_name,
                    "task_id",
                    "stream_id",
                    "subtask_id",
                    "subtask_type",
                    "start_time",
                    "timestamp",
                    "ffts_type",
                    "task_type",
                    "batch_id",
                ]
                pmu_data_type = CustomizedNamedtupleFactory.enhance_namedtuple(
                    namedtuple("PmuData", self._data_name), {}
                )
            pmu_data = pmu_data_type(
                aic_total_time,
                aic_total_cycle,
                *aic_pmu_value_list,
                aiv_total_time,
                aiv_total_cycle,
                *aiv_pmu_value_list,
                data.task_id,
                data.stream_id,
                data.subtask_id,
                data.subtask_type,
                InfoConfReader().time_from_syscnt(data.start_time),
                InfoConfReader().time_from_syscnt(data.end_time),
                data.ffts_type,
                task_type,
                0,  # default batch_id
            )
            pmu_data_list.append(pmu_data)
        return pmu_data_list

    def process_pmu_values(self, pmu_value, table_name_list):
        pmu_value = {k: pmu_value[k] for k in table_name_list if k in pmu_value}
        return list(itertools.chain.from_iterable(PmuMetrics(pmu_value).get_pmu_by_event_name(pmu_value)))

    def get_main_slave_data(self, enumerate_data_list):
        """
        V6芯片时，获取主从核数据
        """
        main_core_data_list = []
        slave_core_data_dict = {}
        for data in enumerate_data_list:
            if data.is_mst_data():
                main_core_data_list.append(data)
            else:
                data_key = self.STREAM_TASK_CONTEXT_KEY_FMT.format(data.stream_id, data.task_id, data.subtask_id)
                slave_core_data_dict.setdefault(data_key, deque()).append(data)
        return main_core_data_list, slave_core_data_dict

    def add_ctx_mix_pmu_to_context_pmu(self, data: any, slave_core_data) -> tuple:
        """
        v6芯片中， mix算子在有一条context从核数据，从context中添加从核数据
        """
        main_pmu_value = self._get_pmu_value(
            *self._get_total_cycle_and_pmu_data(data, data.is_mst_data()), self._aic_pmu_events
        )
        main_core_total_cycle = data.total_cycle
        slave_core_total_cycle = slave_core_data.total_cycle
        slave_pmu_value = self._get_pmu_value(
            *self._get_total_cycle_and_pmu_data(slave_core_data, not slave_core_data.is_mst_data()),
            self._aiv_pmu_events,
        )
        aic_pmu_value, aiv_pmu_value = (
            (main_pmu_value, slave_pmu_value) if (data.is_aic_data()) else (slave_pmu_value, main_pmu_value)
        )
        aic_total_cycle, aiv_total_cycle = (
            (main_core_total_cycle, slave_core_total_cycle)
            if (data.is_aic_data())
            else (slave_core_total_cycle, main_core_total_cycle)
        )
        return aic_pmu_value, aiv_pmu_value, aic_total_cycle, aiv_total_cycle

    def get_pmu_value_and_total_cycle_for_no_mix(self, data) -> tuple:
        aic_pmu_value = self._get_pmu_value(
            *self._get_total_cycle_and_pmu_data(data, data.is_aic_data()), self._aic_pmu_events
        )
        aiv_pmu_value = self._get_pmu_value(
            *self._get_total_cycle_and_pmu_data(data, not data.is_aic_data()), self._aiv_pmu_events
        )
        aic_total_cycle = data.total_cycle if data.is_aic_data() else 0
        aiv_total_cycle = data.total_cycle if not data.is_aic_data() else 0
        return aic_pmu_value, aiv_pmu_value, aic_total_cycle, aiv_total_cycle

    def get_block_pmu_data(self: any) -> None:
        block_pmu_data_type = CustomizedNamedtupleFactory.enhance_namedtuple(
            namedtuple(
                "PmuBlockData",
                ["stream_id", "task_id", "subtask_id", "batch_id", "start_time", "duration", "core_type", "core_id"],
            ),
            {},
        )
        self.block_pmu_data = [
            block_pmu_data_type(
                data.stream_id,
                data.task_id,
                data.subtask_id,
                None,
                InfoConfReader().time_from_syscnt(data.start_time),
                InfoConfReader().duration_from_syscnt(data.end_time - data.start_time),
                data.core_type,  # aic还是aiv看self.AIC_CORE_TYPE
                data.core_id,
            )
            for data in self._data_list_dict.get(StrConstant.BLOCK_PMU_TYPE, [])
        ]

    def _parse_all_file(self) -> None:
        if not self._need_to_analyse():
            return
        try:
            for _file in self._file_list:
                file_path = PathManager.get_data_file_path(self._project_path, _file)
                self._parse_binary_file(file_path)
        except (OSError, SystemError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _parse_by_iter(self) -> None:
        runtime_db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_METRICS_SUMMARY)
        if os.path.exists(runtime_db_path):
            with self._model as model:
                model.drop_table(DBNameConstant.TABLE_METRIC_SUMMARY)
        with self._iter_model as iter_model:
            if not iter_model.check_db() or not iter_model.check_table():
                return
            pmu_offset, pmu_count = self._iter_model.get_task_offset_and_sum(
                self._iter_range, HwtsIterModel.AI_CORE_TYPE
            )
            if pmu_count <= 0:
                logging.warning("The ffts pmu data that is not satisfied by the specified iteration!")
                return
            _file_calculator = FileCalculator(
                self._file_list, self.FFTS_PMU_SIZE, self._project_path, pmu_offset, pmu_count
            )
            for chunk in Utils.chunks(_file_calculator.prepare_process(), self.FFTS_PMU_SIZE):
                self._get_pmu_decode_data(chunk)

    def _parse_binary_file(self: any, file_path: str) -> None:
        """
        read binary data an decode
        :param file_path:
        :return:
        """
        offset_calculator = OffsetCalculator(self._file_list, self.FFTS_PMU_SIZE, self._project_path)
        with FileOpen(file_path, 'rb') as _pmu_file:
            _file_size = os.path.getsize(file_path)
            file_data = offset_calculator.pre_process(_pmu_file.file_reader, _file_size)
            for chunk in Utils.chunks(file_data, self.FFTS_PMU_SIZE):
                self._get_pmu_decode_data(chunk)

    def _get_pmu_decode_data(self: any, bin_data: bytes) -> any:
        try:
            func_type, _ = struct.unpack(StructFmt.STARS_HEADER_FMT, bin_data[:4])
        except (IndexError, ValueError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR) from err
        self._get_pmu_decode_data_list(bin_data, func_type)

    def _get_pmu_decode_data_list(self, bin_data, func_type):
        func_type_str = Utils.get_func_type(func_type)
        if func_type_str == StarsConstant.PMU_TAG_V6:
            context_pmu = PmuBeanV6.decode(bin_data)
            # 仅处理context级别数据 block数据不受ov影响
            if context_pmu.ov_flag:
                self._first_ov_task = (
                    (context_pmu.stream_id, context_pmu.task_id) if not self._first_ov_task else self._first_ov_task
                )
                self._overflow_count += 1
                return
            self._data_list_dict.setdefault(StrConstant.CONTEXT_PMU_TYPE, []).append(context_pmu)
        elif func_type_str == StarsConstant.FFTS_BLOCK_PMU_TAG:
            self._data_list_dict.setdefault(StrConstant.BLOCK_PMU_TYPE, []).append(PmuBeanV6.decode(bin_data))
        else:
            self._wrong_func_type_count += 1
            # 同类型报错只触发一次
            if func_type not in self._logged_func_types:
                logging.error('Func type error, data may have been lost. Func type: %s', func_type)
                self._logged_func_types.add(func_type)

    def _set_stream_id_by_host(self):
        device_id = InfoConfReader().get_device_id()
        host_collector = HostTaskCollector(self._project_path)
        host_task_dict = host_collector.get_host_task_stream_table(int(device_id))
        for data_list in self._data_list_dict.values():
            for data in data_list:
                data.stream_id = host_task_dict.get(data.task_id, Constant.UINT16_MAX)

    def _need_to_analyse(self: any) -> bool:
        db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_METRICS_SUMMARY)
        if not os.path.exists(db_path) or not DBManager.check_tables_in_db(
            db_path, DBNameConstant.TABLE_METRIC_SUMMARY
        ):
            return True
        return False

    def _get_pmu_value(self, total_cycle, pmu_list, pmu_metrics) -> list:
        _, pmu_list = self._aic_calculator.compute_ai_core_data(pmu_metrics, {}, total_cycle, pmu_list)
        return pmu_list

    def _set_pmu_events(self):
        if judge_custom_pmu_scene(self._sample_json):
            self._aic_pmu_events = AicPmuUtils.get_custom_pmu_events(
                self._sample_json.get(StrConstant.AI_CORE_PMU_EVENTS)
            )
            self._aiv_pmu_events = AicPmuUtils.get_custom_pmu_events(
                self._sample_json.get(StrConstant.AI_VECTOR_CORE_PMU_EVENTS)
            )
        else:
            self._aic_pmu_events = AicPmuUtils.get_pmu_events(self._sample_json.get(StrConstant.AI_CORE_PMU_EVENTS))
            self._aiv_pmu_events = AicPmuUtils.get_pmu_events(
                self._sample_json.get(StrConstant.AI_VECTOR_CORE_PMU_EVENTS)
            )

    def _set_table_name_list(self):
        self._aic_table_name_list = get_metrics_from_sample_config(
            self._project_path, StrConstant.AI_CORE_PROFILING_METRICS
        )[2:]
        self._aiv_table_name_list = get_metrics_from_sample_config(
            self._project_path, StrConstant.AIV_PROFILING_METRICS
        )[2:]

    def _get_low_power_freq_data(self: any) -> list:
        """
        从lowPower表中获取AIC频率数据
        :return: [(sys_time, avg_freq_MHz), ...] 按时间排序的列表
        """
        with LowPowerViewModel(
            self._project_path, DBNameConstant.DB_LOW_POWER, [DBNameConstant.TABLE_LOWPOWER]
        ) as model:
            return model.get_aicore_avg_freq_data()

    def _calculate_total_time_for_mix(self: any, data, slave_core_data):
        """
        计算mix类型的总时间：主核使用end_time-start_time，从核使用total_cycle计算
        """
        if data.is_aic_data():
            # 主核(AIC)：保持原有时间差逻辑
            aic_total_time = InfoConfReader().duration_from_syscnt(data.end_time - data.start_time)
            # 从核(AIV)：使用total_cycle计算
            slave_block_num = self._get_current_block('mix_block_num', data)
            slave_core_num = self._core_num_dict.get('aiv', 0)
            slave_freq = self._get_current_freq(InfoConfReader().time_from_syscnt(slave_core_data.end_time))
            aiv_total_time = Utils.cal_total_time(
                slave_core_data.total_cycle, slave_freq, slave_block_num, slave_core_num
            )
        else:
            # 主核(AIV)：保持原有时间差逻辑；从核(AIC)无数据，归零
            aiv_total_time = InfoConfReader().duration_from_syscnt(data.end_time - data.start_time)
            aic_total_time = 0
        aic_total_time = round(aic_total_time, NumberConstant.ROUND_TWO_DECIMAL)
        aiv_total_time = round(aiv_total_time, NumberConstant.ROUND_TWO_DECIMAL)
        return aic_total_time, aiv_total_time

    def _calculate_total_time(self: any, data):
        """
        计算非mix类型（仅主核或仅从核）的总时间
        """
        if data.is_aic_data():
            aic_total_time = InfoConfReader().duration_from_syscnt(data.end_time - data.start_time)
            aiv_total_time = 0
        else:
            aiv_total_time = InfoConfReader().duration_from_syscnt(data.end_time - data.start_time)
            aic_total_time = 0
        aic_total_time = round(aic_total_time, NumberConstant.ROUND_TWO_DECIMAL)
        aiv_total_time = round(aiv_total_time, NumberConstant.ROUND_TWO_DECIMAL)
        return aic_total_time, aiv_total_time
