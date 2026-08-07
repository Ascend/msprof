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
# 与chip6_pmu_calculator同为分叉代码，_format_ge_data override父类时存在常量列表重复

import itertools
import logging
import os
import sqlite3
import struct
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
from common_func.platform.ai_core_metrics_manager import AiCoreMetricsManager
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from common_func.utils import Utils
from framework.offset_calculator import FileCalculator
from framework.offset_calculator import OffsetCalculator
from mscalculate.aic.aic_utils import AicPmuUtils
from mscalculate.aic.pmu_calculator import PmuCalculator
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
from mscalculate.flip.flip_calculator import FlipCalculator
from msmodel.aic.aic_pmu_model import AicPmuModel
from msmodel.freq.freq_parser_model import FreqParserModel
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from msmodel.stars.ffts_pmu_model import FftsPmuModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.stars.ffts_block_pmu import FftsBlockPmuBean
from profiling_bean.stars.ffts_pmu import FftsPmuBean
from viewer.calculate_rts_data import get_metrics_from_sample_config
from viewer.calculate_rts_data import judge_custom_pmu_scene


class FftsPmuCalculator(PmuCalculator, MsMultiProcess):
    """
    一、
    只有MIX类型的算子才会同时有AIC和AIV的数据，MIX算子在AIC和AIV上都会跑，先跑的叫主核，后跑的叫从核
    1.开启block模式
    主核会上报context pmu数据和block pmu数据，从核只会上报block pmu数据；
    主核上报的context pmu数量为1，上报的block pmu数量等于block_num；从核上报的block pmu数据数量等于mix_block_num
    计算从核的pmu数据时，需要过滤block pmu数据中由主核上报的block pmu数据，防止重复计算
    2.不开启block模式
    主核只会上报context pmu数据，从核只会上报block pmu数据；
    主核上报的context pmu数量为1；从核上报的block pmu数据数量等于mix_block_num
    假设主核是AIC，那么aic的一系列pmu数据由context pmu计算得到，aiv的一系列pmu数据由从核上报的block pmu计算得到；
    二、
    原有代码存在的问题：
    1.将所有key（stream_id-task_id-subtask-id）相同的block pmu数据放在一起计算。没有考虑在静态图模式下，每个算子跑N轮的情况
    在静态图模式下，需要将key相同的block pmu按照迭代的次数分组，得到N组block pmu数据
    2.在开启block模式时，没有将主核上报的block pmu数据过滤掉，导致重复计算
    主核上报的context pmu数据，和block pmu数据计算得到的total cycle和pmu一系列数据结果相同，应该只计算从核上报的block pmu数据
    代码修改点：
    1.针对静态图场景：
    对相同stream_id+task_id+subtask_id的block级别数据根据迭代次数N进行划分，共划分为N组，对应N条相同key相同的context pmu
    2.针对开启block模式：
    在解析数据时添加core type，用于区分block pmu数据中哪些为主核上报，哪些为从核上报，用于过滤掉主核上报的block pmu数据
    保持原有的代码的输入、输出不变，只修改原有代码中从核pmu数据的计算方式
    """

    AIC_CORE_TYPE = 0
    AIV_CORE_TYPE = 1
    FFTS_PMU_SIZE = 128
    PMU_LENGTH = 8
    STREAM_TASK_CONTEXT_KEY_FMT = "{0}-{1}-{2}"

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._result_dir = self.sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_model = HwtsIterModel(self._result_dir)
        self._iter_range = self.sample_config.get(StrConstant.PARAM_ITER_ID)
        self._model = FftsPmuModel(self._result_dir, DBNameConstant.DB_METRICS_SUMMARY, [])
        self._data_list = {}
        self._sample_json = ConfigMgr.read_sample_config(self._result_dir)
        self._file_list = file_list.get(DataTag.FFTS_PMU, [])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        self._wrong_func_type_count = 0
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
        self._is_mix_needed = ChipManager().is_chip_v4()
        self.block_dict = {}
        self.mix_pmu_dict = {}
        self.freq_data = []
        self.pmu_data = []
        self.aic_calculator = CalculateAiCoreData(self._result_dir)
        self.aic_table_name_list = []
        self.aiv_table_name_list = []
        self.data_name = []
        self._logged_func_types: set = set()

    @staticmethod
    def _get_total_cycle_and_pmu_data(data: any, is_true: bool) -> tuple:
        """
        default value for pmu cycle list can be set to zero.
        """
        return (data.total_cycle, data.pmu_list) if is_true else (0, [0] * FftsPmuCalculator.PMU_LENGTH)

    @staticmethod
    def _get_group_number(number: int, group_size_list: list) -> int:
        """
        得到block pmu数据分组的编号
        主要针对静态图场景，以及走路径五（aclnn）场景下，Task Type为MIX_AIV类型算子的mix_block_num为0的情况
        Input:
        number: 同一个key下，该条block pmu数据为第几条被上报的数据
        group_size_list: 包含每组数据大小的列表
        Output:
        group_number: 该条block pmu数据应该被分到第几组
        """
        group_count = len(group_size_list)
        # 主要针对静态图场景
        if group_count == 1 and group_size_list[0] != 0:
            return number // group_size_list[0]
        for i in range(group_count):
            # 通过前缀和来判断当前索引所对应的数据应该被分到第几组，如果group_size_list中包含0则通过前缀和跳过
            if number < sum(group_size_list[: i + 1]):
                return i
        return group_count - 1

    @staticmethod
    def _is_not_mix_main_core(core_data, data_type) -> bool:
        return bool(
            (core_data.is_ffts_mix_aic_data() and data_type == 'aiv')
            or (core_data.is_ffts_mix_aiv_data() and data_type == 'aic')
        )

    @staticmethod
    def __get_mix_type(data: any) -> str:
        """
        get mix type
        """
        if data.ffts_type != 4:
            return ''
        if data.subtask_type == 6:
            return Constant.TASK_TYPE_MIX_AIC
        if data.subtask_type == 7:
            return Constant.TASK_TYPE_MIX_AIV
        return ''

    def ms_run(self: any) -> None:
        config = ConfigMgr.read_sample_config(self._result_dir)
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
            db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_METRICS_SUMMARY)
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

    def calculate(self: any) -> None:
        """
        calculate parser data
        :return: None
        """
        if not self._data_list.get(StrConstant.CONTEXT_PMU_TYPE):
            logging.warning("No ai core pmu data found, data list is empty!")
            return
        if self._wrong_func_type_count:
            logging.warning("Some PMU data fails to be parsed, err count: %s", self._wrong_func_type_count)
        for data in self._data_list.get(StrConstant.BLOCK_PMU_TYPE, []):
            self.calculate_block_pmu_list(data)
        self.add_block_pmu_list()
        self.freq_data = FreqParserModel.get_freq_data(self._project_path)
        if any(freq == 0 for _, freq in self.freq_data):
            logging.error("The sampled frequency is 0Hz, using default frequency %sHz.", self._freq)
            self.freq_data = []
        self._set_ffts_table_name_list()
        self.pmu_data = [None] * len(self._data_list.get(StrConstant.CONTEXT_PMU_TYPE, []))
        if self._is_mix_needed:
            self.calculate_mix_pmu_list(self.pmu_data)
        else:
            self.calculate_pmu_list(self.pmu_data)
        if ChipManager().is_chip_all_data_export() and InfoConfReader().is_all_export_version():
            self.pmu_data = FlipCalculator.set_device_batch_id(self.pmu_data, self._result_dir)
        if not self._is_mix_needed:
            # 去除timestamp字段
            self.pmu_data = [pmu_data[:-1] for pmu_data in self.pmu_data]

    def save(self: any) -> None:
        """
        save parser data to db
        :return: None
        """
        try:
            with self._model as _model:
                _model.flush(self.pmu_data)
        except sqlite3.Error as err:
            logging.error("Save ffts pmu data failed! %s", err)

    def calculate_mix_pmu_list(self: any, pmu_data_list: list) -> None:
        """
        当MIX算子存在时，计算pmu数据
        :param pmu_data_list: out args
        :return:
        """
        enumerate_data_list = self._data_list.get(StrConstant.CONTEXT_PMU_TYPE, [])
        pmu_data_type = None
        for index, data in enumerate(enumerate_data_list):
            task_type = 0 if data.is_aic_data() else 1
            block_num = self._get_current_block('block_num', data)
            mix_block_num = self._get_current_block('mix_block_num', data)
            aic_pmu_value, aiv_pmu_value, aic_total_cycle, aiv_total_cycle = self.add_block_mix_pmu_to_context_pmu(
                data, task_type, mix_block_num
            )
            block_num_dict = {}
            # False代表主核（False），True代表从核（True）
            block_num_dict.setdefault(False, block_num)
            block_num_dict.setdefault(True, mix_block_num)
            aic_total_time = self.calculate_total_time(data, aic_total_cycle, block_num_dict)
            aiv_total_time = self.calculate_total_time(data, aiv_total_cycle, block_num_dict, data_type='aiv')
            aic_pmu_value = self.aic_calculator.add_pipe_time(
                aic_pmu_value, aic_total_time, self._sample_json.get('ai_core_metrics')
            )
            aic_pmu_value = {k: aic_pmu_value[k] for k in self.aic_table_name_list if k in aic_pmu_value}

            aiv_pmu_value = self.aic_calculator.add_pipe_time(
                aiv_pmu_value, aiv_total_time, self._sample_json.get('ai_core_metrics')
            )
            aiv_pmu_value = {k: aiv_pmu_value[k] for k in self.aiv_table_name_list if k in aiv_pmu_value}

            aic_pmu_value_list = list(
                itertools.chain.from_iterable(PmuMetrics(aic_pmu_value).get_pmu_by_event_name(aic_pmu_value))
            )
            aiv_pmu_value_list = list(
                itertools.chain.from_iterable(PmuMetrics(aiv_pmu_value).get_pmu_by_event_name(aiv_pmu_value))
            )
            if not pmu_data_type:
                aic_pmu_name = ["aic_" + str(i) for i in range(len(aic_pmu_value_list))]
                aiv_pmu_name = ["aiv_" + str(i) for i in range(len(aiv_pmu_value_list))]
                self.data_name = [
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
                    namedtuple("PmuData", self.data_name), {}
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
                InfoConfReader().time_from_syscnt(data.time_list[0]),
                InfoConfReader().time_from_syscnt(data.time_list[1]),
                data.ffts_type,
                task_type,
                -1,  # default batch_id
            )
            pmu_data_list[index] = pmu_data

    def calculate_pmu_list(self: any, pmu_data_list: list) -> None:
        """
        当MIX算子不存在时，计算pmu数据
        无论是否开启block级别，都使用了context pmu数据，没有用到block pmu数据
        :param pmu_data_list: pmu events list
        :return:
        """
        self.__update_model_instance()
        enumerate_data_list = self._data_list.get(StrConstant.CONTEXT_PMU_TYPE, [])
        pmu_data_type = None
        for index, data in enumerate(enumerate_data_list):
            task_type = 0 if data.is_aic_data() else 1
            pmu_list = {}
            if judge_custom_pmu_scene(self._sample_json):
                pmu_events = AicPmuUtils.get_custom_pmu_events(self._sample_json.get('ai_core_profiling_events'))
            else:
                pmu_events = AicPmuUtils.get_pmu_events(self._sample_json.get('ai_core_profiling_events'))
            block_num_dict = {}
            block_num_dict.setdefault(False, self._get_current_block('block_num', data))
            block_num_dict.setdefault(True, self._get_current_block('mix_block_num', data))
            total_time = self.calculate_total_time(data, data.total_cycle, block_num_dict)
            _, pmu_list = self.aic_calculator.compute_ai_core_data(
                Utils.generator_to_list(pmu_events), pmu_list, data.total_cycle, data.pmu_list
            )
            pmu_list = self.aic_calculator.add_pipe_time(pmu_list, total_time, self._sample_json.get('ai_core_metrics'))
            pmu_list = {k: pmu_list[k] for k in self.aic_table_name_list if k in pmu_list}
            AicPmuUtils.remove_redundant(pmu_list)
            if not pmu_data_type:
                pmu_name = ["pmu_" + str(i) for i in range(len(pmu_list))]
                self.data_name = [
                    "total_time",
                    "total_cycle",
                    *pmu_name,
                    "task_id",
                    "stream_id",
                    "task_type",
                    "batch_id",
                    "timestamp",
                ]
                pmu_data_type = CustomizedNamedtupleFactory.enhance_namedtuple(
                    namedtuple("PmuData", self.data_name), {}
                )
            pmu_data = pmu_data_type(
                total_time,
                data.total_cycle,
                *list(itertools.chain.from_iterable(pmu_list.values())),
                data.task_id,
                data.stream_id,
                task_type,
                -1,  # default batch_id
                InfoConfReader().time_from_syscnt(data.time_list[1]),  # end time
            )
            pmu_data_list[index] = pmu_data

    @staticmethod
    def calc_total_time_by_cycle(total_cycle: int, freq: int, block_num: int, core_num: int) -> float:
        """
        基于cycle数计算total_time，纯计算方法，供chip6等子类复用
        """
        return Utils.cal_total_time(total_cycle, int(freq), block_num, core_num)

    def calculate_total_time(
        self: any, data: any, total_cycle: int, block_num_dict: dict, data_type: str = 'aic'
    ) -> float:
        core_num = self._core_num_dict.get(data_type)
        block_num = block_num_dict.get(self._is_not_mix_main_core(data, data_type))
        freq = self._freq
        if self.freq_data and len(data.time_list) >= 2:
            freq = self._get_current_freq(data.time_list[1])
        total_time = self.calc_total_time_by_cycle(total_cycle, int(freq), block_num, core_num)
        return total_time

    def add_block_mix_pmu_to_context_pmu(self, data: any, task_type: int, mix_block_num: int) -> tuple:
        """
        add block mix pmu to context pmu
        将block pmu数据添加到context pmu中
        如果task_type的值为0，则代表AIC；如果其值为1，则代表AIV
        静态图模式下，同一个算子运行n轮，其stream_id、task_id和subtask_id都相同。context pmu对应的block pmu数据有n条，需要一一匹配。
        demo mix_pmu_info:
        [{'mix_type': 'MIX_AIC', 'total_cycle': 1, 'pmu': {'vec_ratio': [...], 'mac_ratio': [...],
                                                           'scalar_ratio': [...], 'mte1_ratio': [...],
                                                           'mte2_ratio': [...], 'mte3_ratio': [...],
                                                           'icache_req_ratio': [...], 'icache_miss_rate': [...]}}]
        :return: tuple
        """
        # 主核的total_cycle和pmu数据由context pmu提供
        aic_pmu_value = self._get_pmu_value(
            *self._get_total_cycle_and_pmu_data(data, data.is_aic_data()), self._aic_pmu_events
        )
        aiv_pmu_value = self._get_pmu_value(
            *self._get_total_cycle_and_pmu_data(data, not data.is_aic_data()), self._aiv_pmu_events
        )
        data_key = self.STREAM_TASK_CONTEXT_KEY_FMT.format(data.stream_id, data.task_id, data.subtask_id)
        mix_pmu_info = self.mix_pmu_dict.get(data_key, {})
        aic_total_cycle = data.total_cycle if not task_type else 0
        aiv_total_cycle = data.total_cycle if task_type else 0
        # 从核的total_cycle和pmu数据由block pmu提供，如果某条context pmu数据的mix_block_num为0则直接跳过
        if mix_pmu_info and mix_block_num != 0:
            pmu_info = mix_pmu_info.pop(0)
            mix_pmu_value, mix_total_cycle = pmu_info.get('pmu'), pmu_info.get('total_cycle')
            if pmu_info.get('mix_type') == Constant.TASK_TYPE_MIX_AIV:
                aic_pmu_value, aic_total_cycle = mix_pmu_value, mix_total_cycle
            else:
                aiv_pmu_value, aiv_total_cycle = mix_pmu_value, mix_total_cycle
        res_tuple = aic_pmu_value, aiv_pmu_value, aic_total_cycle, aiv_total_cycle
        return res_tuple

    def calculate_block_pmu_list(self: any, data: any) -> None:
        """
        assortment mix pmu data
        :return: None
        """
        mix_type = self.__get_mix_type(data)
        if not mix_type:
            return
        task_key = self.STREAM_TASK_CONTEXT_KEY_FMT.format(data.stream_id, data.task_id, data.subtask_id)
        aic_pmu_value = self._get_total_cycle_and_pmu_data(data, data.is_aic_data())
        aiv_pmu_value = self._get_total_cycle_and_pmu_data(data, not data.is_aic_data())

        pmu_list = aic_pmu_value if mix_type == Constant.TASK_TYPE_MIX_AIC else aiv_pmu_value
        self.block_dict.setdefault(task_key, []).append((mix_type, pmu_list, data.core_type))

    def get_group_size_list(self: any, key: str) -> list:
        """
        通过self._block_num中的block_num_group来获取一个包含每组大小的列表
        1.不开启block模式，每组的大小为mix_block_num
        2.开启block模式，每组的大小为（block_num + mix_block_num）
        先判断采集到这份数据时，是否开启了block模式，然后再获取每组的group size
        """
        group_size_list = []
        for item in self._block_num.get('block_num_group', {}).get(key, []):
            if self.is_block():
                group_size_list.append((item[0] + item[1]))
            else:
                group_size_list.append(item[1])
        return group_size_list

    def is_block(self) -> bool:
        # 开启block模式的判断方法：
        # 1.sample.json文件中存在'taskBlock'字段
        # 2.'taskBlock'字段的value值为'on'
        if 'taskBlock' in self._sample_json and self._sample_json.get('taskBlock') == 'on':
            return True
        return False

    def add_block_pmu_list(self) -> None:
        """
        通过block pmu数据计算total cycle和pmu等数据
        在静态图模式下，假设某一个MIX算子跑了N轮，其key为1-1-1，那么mix_pmu_dict['1-1-1']中就会有N条数据
        demo: 不开启block模式, mix_block_num = 2, 一共跑了2轮
        input: {'1-1-1': [('MIX_AIC', (1, (1, 2, 3, 4, 1, 2, 3, 4)), 1),
                          ('MIX_AIC', (1, (4, 3, 2, 1, 4, 3, 2, 1)), 1),
                          ('MIX_AIC', (1, (2, 3, 4, 5, 2, 3, 4, 5)), 1),
                          ('MIX_AIC', (1, (5, 4, 3, 2, 5, 4, 3, 2)), 1)]}
        output: {'1-1-1': [{'mix_type': 'MIX_AIC', 'total_cycle': 2,
                               'pmu': {'vec_ratio': [...], 'mac_ratio': [...], 'scalar_ratio': [...],
                                       'mte1_ratio': [...], 'mte2_ratio': [...], 'mte3_ratio': [...],
                                       'icache_req_ratio': [...], 'icache_miss_rate': [...]}},
                              {'mix_type': 'MIX_AIC', 'total_cycle': 2,
                              'pmu': {'vec_ratio': [...], 'mac_ratio': [...], 'scalar_ratio': [...],
                                       'mte1_ratio': [...], 'mte2_ratio': [...], 'mte3_ratio': [...],
                                       'icache_req_ratio': [...], 'icache_miss_rate': [...]}}]}
        """
        if not self.block_dict:
            return
        for key, value in self.block_dict.items():
            group_size_list = self.get_group_size_list(key)
            grouped_block_dict = self.group_block_with_iter(value, group_size_list)
            mix_pmu_list = []
            for _, pmu_info_value in grouped_block_dict.items():
                mix_type_set = {pmu_info[0] for pmu_info in pmu_info_value}
                # One mix operator has only one mix_type.
                if len(mix_type_set) != 1:
                    logging.error('Pmu data type error, task key: stream_id-task_id-subtask_id: %s', key)
                    continue
                mix_type = mix_type_set.pop()
                total_cycle, cycle_list = self._calculate_total_cycle_and_cycle_list(mix_type, pmu_info_value)
                # 通过上面得到的total_cycle、cycle_list来计算从核的pmu数据
                if mix_type == Constant.TASK_TYPE_MIX_AIC:
                    pmu = self._get_pmu_value(total_cycle, cycle_list, self._aiv_pmu_events)
                elif mix_type == Constant.TASK_TYPE_MIX_AIV:
                    pmu = self._get_pmu_value(total_cycle, cycle_list, self._aic_pmu_events)
                else:
                    logging.error('Mix type error, the key: stream_id-task_id-subtask_id: %s', key)
                    pmu = cycle_list
                mix_pmu_list.append({'mix_type': mix_type, 'total_cycle': total_cycle, 'pmu': pmu})
            self.mix_pmu_dict[key] = mix_pmu_list

    def group_block_with_iter(self, value, group_size_list) -> dict:
        """
        将拥有相同key（stream_id-task_id-subtask_id）的原始block pmu数据进行分组，主要针对静态图模式
        假设在静态图模式下，一个MIX算子跑了N个迭代，其block_num 为10, mix_block_num为20
        1.不开启block模式，则该算子总共上报20N条block pmu数据，每20条block pmu数据为1组
        2.开启block模式，则该算子总共上报（10+20）*N条block pmu数据，每（10+20）条block pmu数据为1组
        demo single_value:
        ('MIX_AIC', (3054, (59, 0, 749, 0, 2, 1667, 100, 48)), 1)
        1 表示core type为AIV，0 表示core type为AIC
        """
        pmu_info_dict = {}
        for number, item in enumerate(value):
            # item[0], item[1][0], item[1][1], item[2]分别表示mix type、total cycle、pmu list和core type
            single_pmu_value = (item[0], item[1][0], item[1][1], item[2])
            group_number = self._get_group_number(number, group_size_list)
            if group_number in pmu_info_dict:
                pmu_info_dict[group_number] += (single_pmu_value,)
            else:
                pmu_info_dict[group_number] = (single_pmu_value,)
        return pmu_info_dict

    def _calculate_total_cycle_and_cycle_list(self, mix_type_value, pmu_info_value):
        """
        通过block pmu数据计算total cycle和cycle list
        如果没有开启block模式，那么所有的block pmu数据都会被保留
        如果开启block模式，那么需要过滤掉主核上报的block pmu数据
        demo pmu_info_value:
        ('MIX_AIC', 330021, (28, 0, 20301, 0, 1190, 113, 2546, 149), 1)
        1 表示core type为AIV，0 表示core type为AIC
        """
        total_cycle = sum(cycle[1] for cycle in pmu_info_value)
        cycle_list = [sum(cycle[2][index] for cycle in pmu_info_value) for index in range(self.PMU_LENGTH)]
        # 开启block模式时，只保留从核上报的block pmu数据来计算total cycle、cycle list
        # total cycle、cycle list由保留下来的block pmu数据相加得到
        if self.is_block():
            if mix_type_value == Constant.TASK_TYPE_MIX_AIC:
                # cycle[1]、cycle[2][index]、cycle[3]分别表示每条block pmu数据的total cycle、cycle list和core type
                total_cycle = sum(cycle[1] for cycle in pmu_info_value if cycle[3] == self.AIV_CORE_TYPE)
                cycle_list = [
                    sum(cycle[2][index] for cycle in pmu_info_value if cycle[3] == self.AIV_CORE_TYPE)
                    for index in range(self.PMU_LENGTH)
                ]
            elif mix_type_value == Constant.TASK_TYPE_MIX_AIV:
                total_cycle = sum(cycle[1] for cycle in pmu_info_value if cycle[3] == self.AIC_CORE_TYPE)
                cycle_list = [
                    sum(cycle[2][index] for cycle in pmu_info_value if cycle[3] == self.AIC_CORE_TYPE)
                    for index in range(self.PMU_LENGTH)
                ]
        return total_cycle, cycle_list

    def _parse_all_file(self) -> None:
        if not self._need_to_analyse():
            return
        try:
            for _file in self._file_list:
                file_path = PathManager.get_data_file_path(self._result_dir, _file)
                self._parse_binary_file(file_path)
        except (OSError, SystemError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _parse_by_iter(self) -> None:
        runtime_db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_METRICS_SUMMARY)
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
                self._file_list, self.FFTS_PMU_SIZE, self._result_dir, pmu_offset, pmu_count
            )
            for chunk in Utils.chunks(_file_calculator.prepare_process(), self.FFTS_PMU_SIZE):
                self._get_pmu_decode_data(chunk)

    def _parse_binary_file(self: any, file_path: str) -> None:
        """
        read binary data an decode
        :param file_path:
        :return:
        """
        offset_calculator = OffsetCalculator(self._file_list, self.FFTS_PMU_SIZE, self._result_dir)
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
        if Utils.get_func_type(func_type) == StarsConstant.FFTS_PMU_TAG:
            context_pmu = FftsPmuBean.decode(bin_data)
            # 仅处理context级别数据 block数据不受ov影响
            if context_pmu.ov_flag:
                logging.warning(
                    "An overflow in the operator (stream id = %d, task id = %d) count has been detected."
                    "Total_cycle value is invalid!",
                    context_pmu.stream_id,
                    context_pmu.task_id,
                )
                return
            self._data_list.setdefault(StrConstant.CONTEXT_PMU_TYPE, []).append(context_pmu)
        elif Utils.get_func_type(func_type) == StarsConstant.FFTS_BLOCK_PMU_TAG:
            self._data_list.setdefault(StrConstant.BLOCK_PMU_TYPE, []).append(FftsBlockPmuBean.decode(bin_data))
        else:
            self._wrong_func_type_count += 1

            # 用set确保每种异常类型打印一次
            if func_type not in self._logged_func_types:
                logging.error('Func type error, data may have been lost. Func type: %s', func_type)
                self._logged_func_types.add(func_type)

    def _need_to_analyse(self: any) -> bool:
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_METRICS_SUMMARY)
        if not os.path.exists(db_path):
            return True
        if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_METRIC_SUMMARY):
            return False
        return True

    def _get_pmu_value(self, total_cycle, pmu_list, pmu_metrics) -> list:
        _, pmu_list = self.aic_calculator.compute_ai_core_data(pmu_metrics, {}, total_cycle, pmu_list)
        return pmu_list

    def _get_current_block(self: any, block_type: str, ai_core_data: any) -> any:
        """
        get the current block num when stream id and task id occurs again
        :param ai_core_data: ai core pmu data
        :return: block num
        """
        block_key = self.STREAM_TASK_CONTEXT_KEY_FMT.format(
            ai_core_data.stream_id, ai_core_data.task_id, ai_core_data.subtask_id
        )
        if not ai_core_data.is_ffts_plus_type():
            block_key = self.STREAM_TASK_CONTEXT_KEY_FMT.format(
                ai_core_data.stream_id, ai_core_data.task_id, NumberConstant.DEFAULT_GE_CONTEXT_ID
            )
        block = self._block_num.get(block_type, {}).get(block_key)
        if not block:
            return 0
        return block.pop(0) if len(block) > 1 else block[0]

    def _format_ge_data(self: any, ge_data: list) -> None:
        for data in ge_data:
            if data.task_type not in [
                Constant.TASK_TYPE_AI_CORE,
                Constant.TASK_TYPE_AIV,
                Constant.TASK_TYPE_COMMUNICATION,
                Constant.TASK_TYPE_MIX_AIC,
                Constant.TASK_TYPE_MIX_AIV,
            ]:
                continue
            _key = self.STREAM_TASK_CONTEXT_KEY_FMT.format(data.stream_id, data.task_id, data.context_id)
            self._block_num['block_num'].setdefault(_key, []).append(int(data.block_num))
            if data.task_type in [Constant.TASK_TYPE_MIX_AIV, Constant.TASK_TYPE_MIX_AIC]:
                self._block_num['block_num_group'].setdefault(_key, []).append(
                    [int(data.block_num), int(data.mix_block_num)]
                )
                self._block_num['mix_block_num'].setdefault(_key, []).append(int(data.mix_block_num))

    def __update_model_instance(self):
        self._model = AicPmuModel(self._project_path)

    def _set_ffts_table_name_list(self):
        """
        table_name_list[:2]:'total_time(ms)', 'total_cycles', unused
        """
        self.aic_table_name_list = get_metrics_from_sample_config(
            self._project_path, StrConstant.AI_CORE_PROFILING_METRICS
        )[2:]
        if self._is_mix_needed:
            self.aiv_table_name_list = get_metrics_from_sample_config(
                self._project_path, StrConstant.AIV_PROFILING_METRICS
            )[2:]
        if self._sample_json.get(StrConstant.AI_CORE_PROFILING_METRICS, "") == AiCoreMetricsManager.PMU_PIPE_EXECUT:
            self.aic_table_name_list = [
                "vec_exe_time",
                "vec_exe_ratio",
                "mac_time",
                "mac_ratio_extra",
                "scalar_time",
                "scalar_ratio",
                "mte1_time",
                "mte1_ratio_extra",
                "mte2_time",
                "mte2_ratio",
                "mte3_time",
                "mte3_ratio",
                "fixpipe_time",
                "fixpipe_ratio",
            ]


class PmuMetrics:
    """
    Pmu metrics for ai core and ai vector core.
    """

    def __init__(self, pmu_dict: dict):
        self.init(pmu_dict)

    def init(self, pmu_dict: dict):
        """
        construct ffts pmu data.
        """
        for pmu_name, pmu_value in pmu_dict.items():
            setattr(self, pmu_name, pmu_value)

    def get_pmu_by_event_name(self, event_name_list):
        """
        get pmu value list order by pmu event name list.
        """
        AicPmuUtils.remove_redundant(event_name_list)
        return [getattr(self, event_name, 0) for event_name in event_name_list if hasattr(self, event_name)]
