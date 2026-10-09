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
import os

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from framework.offset_calculator import OffsetCalculator
from msmodel.aic.ai_core_sample_model import AiCoreSampleModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.aicore_sample import AicoreSample
from profiling_bean.struct_info.aicore_sample import AicoreSampleV6
from viewer.calculate_rts_data import check_aicore_events, judge_custom_pmu_scene


class ParsingCoreSampleData(MsMultiProcess):
    """
    base parsing sample data class
    """

    def __init__(self: any, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self.result_dir = self.sample_config.get("result_dir")
        self.file_list = []
        self.ai_core_data = []
        self.device_id = self.sample_config.get("device_id", "0")
        self.model = None
        self._db_name = DBNameConstant.DB_NAME_AICORE
        self._replayid = 0
        self._core_type = 'aic'
        self._metrics_type = StrConstant.AI_CORE_PROFILING_METRICS
        self._event = self.sample_config.get("ai_core_profiling_events", "").split(",")
        self._sample_metrics_key = self.sample_config.get(StrConstant.AI_CORE_PROFILING_METRICS)
        self._set_fmt_size_and_bean()

    def get_aicore_type(self: any) -> str:
        """
        get ai core type
        :return:
        """
        return self.sample_config.get('ai_core_profiling_mode')

    def ms_run(self: any) -> None:
        """
        ai core sample based parser
        :return:
        """
        if not self.get_aicore_type() == StrConstant.AIC_SAMPLE_BASED_MODE:
            return
        self.start_parsing_data_file()
        try:
            self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def start_parsing_data_file(self: any) -> None:
        """
        parser ai core / aiv sample based file
        :return:
        """
        try:
            for file_name in self.file_list:
                if is_valid_original_data(file_name, self.result_dir):
                    logging.info("start parsing data file: %s", file_name)
                    self.read_binary_data(file_name)
                    FileManager.add_complete_file(self.result_dir, file_name)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save ai core / aiv sample based data to db
        :return:
        """
        self.model = AiCoreSampleModel(
            self.result_dir, self._db_name, [DBNameConstant.TABLE_EVENT_COUNT], self._metrics_type
        )
        check_aicore_events(
            self._event, is_custom=judge_custom_pmu_scene(self.sample_config, metrics_type=self._metrics_type)
        )
        if self.model and self.ai_core_data:
            self.model.init()
            self.model.create_core_table(self._event, self.ai_core_data)

            self.model.insert_metric_value()
            freq = InfoConfReader().get_freq(self._core_type)

            self.model.insert_metric_summary_table(freq, self._sample_metrics_key)
            self.model.finalize()

    def read_binary_data(self: any, binary_data_path: str) -> None:
        """
        read binary data
        :param binary_data_path: file path
        :return:
        """
        if not os.path.exists(PathManager.get_data_file_path(self.result_dir, binary_data_path)):
            return
        file_size = os.path.getsize(PathManager.get_data_file_path(self.result_dir, binary_data_path))
        file_name = PathManager.get_data_file_path(self.result_dir, binary_data_path)
        calculate = OffsetCalculator(self.file_list, self._fmt_size, self.result_dir)
        try:
            with FileOpen(file_name, 'rb') as file_:
                ai_core_data = calculate.pre_process(file_.file_reader, file_size)
                self._insert_ai_core_data(file_size, ai_core_data)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", binary_data_path, err, exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

    def _insert_ai_core_data(self: any, file_size: int, ai_core_data: list) -> None:
        delta_dev = InfoConfReader().get_delta_time()
        for _index in range(file_size // self._fmt_size):
            binary_data = ai_core_data[_index * self._fmt_size : (_index + 1) * self._fmt_size]
            if not binary_data[0]:
                break
            aicore_data_bean = self._decoder.decode(binary_data)
            if aicore_data_bean is not None:
                tmp = [
                    aicore_data_bean.count_num,
                    aicore_data_bean.mode,
                    self._replayid,
                    aicore_data_bean.timestamp + delta_dev * NumberConstant.NANO_SECOND,
                    aicore_data_bean.core_id,
                    aicore_data_bean.task_cyc,
                ]
                tmp.extend(aicore_data_bean.event_count[: aicore_data_bean.count_num])
                self.ai_core_data.append(tmp)
            else:
                break

    def _set_fmt_size_and_bean(self):
        if ChipManager().is_chip_v6():
            self._fmt_size = StructFmt.AICORE_SAMPLE_V6_FMT_SIZE
            self._decoder = AicoreSampleV6
        else:
            self._fmt_size = StructFmt.AICORE_SAMPLE_FMT_SIZE
            self._decoder = AicoreSample


class ParsingAICoreSampleData(ParsingCoreSampleData):
    """
    parsing ai vector sample data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.file_list = sorted(file_list.get(DataTag.AI_CORE, []), key=lambda x: int(x.split("_")[-1]))


class ParsingAIVectorCoreSampleData(ParsingCoreSampleData):
    """
    parsing ai vector core data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.file_list = sorted(file_list.get(DataTag.AIV, []), key=lambda x: int(x.split("_")[-1]))
        self._db_name = DBNameConstant.DB_NAME_AI_VECTOR_CORE
        self._event = self.sample_config.get("aiv_profiling_events", "").split(",")
        self._sample_metrics_key = self.sample_config.get(StrConstant.AIV_PROFILING_METRICS)
        self._core_type = 'aiv'
        self._metrics_type = StrConstant.AIV_PROFILING_METRICS
        self.calculate = OffsetCalculator(self.file_list, self._fmt_size, self.result_dir)

    def get_aicore_type(self: any) -> str:
        """
        get ai core type
        :return:
        """
        return self.sample_config.get('aiv_profiling_mode')


class ParsingFftsAICoreSampleData(ParsingCoreSampleData):
    """
    parsing ai vector sample data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.file_list = sorted(file_list.get(DataTag.FFTS_PMU, []), key=lambda x: int(x.split("_")[-1]))
        self.calculate = OffsetCalculator(self.file_list, self._fmt_size, self.result_dir)
        self.data_dict = {
            'aic': {
                'data_list': [],
                'db_name': DBNameConstant.DB_NAME_AICORE,
                'event': self.sample_config.get("ai_core_profiling_events", "").split(","),
                'metrics_key': self.sample_config.get(StrConstant.AI_CORE_PROFILING_METRICS),
                'metric_type': StrConstant.AI_CORE_PROFILING_METRICS,
            },
            'aiv': {
                'data_list': [],
                'db_name': DBNameConstant.DB_NAME_AI_VECTOR_CORE,
                'event': self.sample_config.get("aiv_profiling_events", "").split(","),
                'metrics_key': self.sample_config.get(StrConstant.AIV_PROFILING_METRICS),
                'metric_type': StrConstant.AIV_PROFILING_METRICS,
            },
        }

    def save(self: any) -> None:
        """
        save ai core / aiv sample based data to db
        :return:
        """
        for data_type, data_dict in self.data_dict.items():
            db_name = data_dict.get('db_name')
            data_list = data_dict.get('data_list')
            event = data_dict.get('event')
            metrics_key = data_dict.get('metrics_key')
            metrics_type = data_dict.get('metric_type')
            check_aicore_events(event, is_custom=judge_custom_pmu_scene(self.sample_config, metrics_type=metrics_type))
            if not all([db_name, data_list, event, metrics_key]):
                logging.info("No need to create db for %s pmu_events", data_type)
                continue
            model = AiCoreSampleModel(self.result_dir, db_name, [DBNameConstant.TABLE_EVENT_COUNT], metrics_type)
            if model and data_list:
                model.init()
                model.create_core_table(event, data_list)

                model.insert_metric_value()
                freq = InfoConfReader().get_freq(data_type)

                model.insert_metric_summary_table(freq, metrics_key)
                model.finalize()

    def _insert_ai_core_data(self: any, file_size: int, ai_core_data: list) -> None:
        delta_dev = InfoConfReader().get_delta_time()
        for _index in range(file_size // self._fmt_size):
            binary_data = ai_core_data[_index * self._fmt_size : (_index + 1) * self._fmt_size]

            if not binary_data[0]:
                break
            aicore_data_bean = self._decoder.decode(binary_data)
            if aicore_data_bean is not None:
                timestamp = self._calculate_timestamp(aicore_data_bean, delta_dev)
                tmp = [
                    aicore_data_bean.count_num,
                    aicore_data_bean.mode,
                    self._replayid,
                    timestamp,
                    aicore_data_bean.core_id,
                    aicore_data_bean.task_cyc,
                ]
                tmp.extend(aicore_data_bean.event_count[: aicore_data_bean.count_num])
            else:
                break
            # core id in [0, 24]:aic  in [25, +):aiv
            core_id_edge = ChipManager().get_max_core_id()
            if aicore_data_bean.core_id > core_id_edge:
                self.data_dict.setdefault('aiv', {}).setdefault('data_list', []).append(tmp)
            else:
                self.data_dict.setdefault('aic', {}).setdefault('data_list', []).append(tmp)

    def _calculate_timestamp(self: any, aicore_data_bean: any, delta_dev: int) -> float:
        if ChipManager().is_chip_v6():
            # timestamp is syscnt on chip v6
            return InfoConfReader().time_from_syscnt(aicore_data_bean.timestamp)
        return aicore_data_bean.timestamp + delta_dev * NumberConstant.NANO_SECOND
