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

import itertools
import logging
import os

from common_func.config_mgr import ConfigMgr
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.file_manager import FileOpen
from common_func.profiling_scene import ProfilingScene
from common_func.utils import Utils
from framework.offset_calculator import FileCalculator
from framework.offset_calculator import OffsetCalculator
from mscalculate.aic.aic_utils import AicPmuUtils
from mscalculate.aic.pmu_calculator import PmuCalculator
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
from msmodel.aic.aic_pmu_model import AicPmuModel
from msmodel.aic.aic_pmu_model import V5AicPmuModel
from msmodel.ge.ge_info_calculate_model import GeInfoModel
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from msmodel.v5.v5_stars_model import V5StarsViewModel
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.aic_pmu import AicPmuBean
from viewer.calculate_rts_data import judge_custom_pmu_scene
from viewer.calculate_rts_data import get_metrics_from_sample_config


class AicCalculator(PmuCalculator, MsMultiProcess):
    """
    class used to parse aicore data by iter
    """

    AICORE_LOG_SIZE = 128

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_model = HwtsIterModel(self._project_path)
        self.ge_info_model = GeInfoModel(self._project_path)
        self._sample_json = ConfigMgr.read_sample_config(self._project_path)
        self._file_list = file_list.get(DataTag.AI_CORE, [])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        self._aic_data_list = []
        self._iter_range = self.sample_config.get(StrConstant.PARAM_ITER_ID)
        self.core_type = 0
        self.aic_calculator = CalculateAiCoreData(self._project_path)
        # table_name_list[:2]:'total_time(ms)', 'total_cycles', unused
        self.table_name_list = get_metrics_from_sample_config(
            self._project_path, StrConstant.AI_CORE_PROFILING_METRICS
        )[2:]

    def calculate(self: any) -> None:
        """
        calculate the ai core
        :return: None
        """
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

    def calculate_pmu_list(self: any, data: any, profiling_events: list, data_list: list, total_time: float) -> None:
        """
        calculate pmu
        :param profiling_events: pmu events list
        :param data: pmu data
        :param data_list: out args
        :param total_time: total time
        :return:
        """
        pmu_list = {}
        _, pmu_list = self.aic_calculator.compute_ai_core_data(
            Utils.generator_to_list(profiling_events), pmu_list, data.total_cycle, data.pmu_list
        )

        pmu_list = self.aic_calculator.add_pipe_time(pmu_list, total_time, self._sample_json.get('ai_core_metrics'))
        pmu_list = {k: pmu_list[k] for k in self.table_name_list if k in pmu_list}
        AicPmuUtils.remove_redundant(pmu_list)
        data_list.append(
            [
                total_time,
                data.total_cycle,
                *list(itertools.chain.from_iterable(pmu_list.values())),
                data.task_id,
                data.stream_id,
                self.core_type,
                -1,  # -1 是batch_id
            ]
        )

    def calculate_total_time(self: any, data: AicPmuBean, data_type: str = 'aic'):
        core_num = self._core_num_dict.get(data_type)
        block_num = self._get_current_block('block_num', data)
        total_time = Utils.cal_total_time(data.total_cycle, int(self._freq), block_num, core_num)
        return total_time

    def save(self: any) -> None:
        """
        save ai core data
        :return: None
        """
        if self._aic_data_list:
            with AicPmuModel(self._project_path) as aic_pmu_model:
                aic_pmu_model.flush(self._aic_data_list)

    def ms_run(self: any) -> None:
        """
        entrance or ai core calculator
        :return: None
        """
        if self._sample_json.get('ai_core_profiling_mode') == StrConstant.AIC_SAMPLE_BASED_MODE:
            return

        if not self._file_list:
            return
        self.init_params()
        self.calculate()
        self.save()

    def _parse_by_iter(self: any) -> None:
        """
        Parse the specified iteration data
        :return: None
        """
        if self._iter_model.check_db() and self._iter_model.check_table():
            pmu_offset, pmu_count = self._iter_model.get_task_offset_and_sum(
                self._iter_range, HwtsIterModel.AI_CORE_TYPE
            )
            if pmu_count <= 0:
                logging.warning("The ai core data that is not satisfied by the specified iteration!")
                return
            _file_calculator = FileCalculator(
                self._file_list, self.AICORE_LOG_SIZE, self._project_path, pmu_offset, pmu_count
            )
            self._parse(_file_calculator.prepare_process())
            self._iter_model.finalize()

    def _parse_all_file(self: any) -> None:
        _offset_calculator = OffsetCalculator(self._file_list, self.AICORE_LOG_SIZE, self._project_path)
        for _file in self._file_list:
            _file = PathManager.get_data_file_path(self._project_path, _file)
            logging.info("start parsing ai core data file: %s", os.path.basename(_file))
            with FileOpen(_file, 'rb') as _aic_reader:
                self._parse(_offset_calculator.pre_process(_aic_reader.file_reader, os.path.getsize(_file)))

    def _parse(self: any, all_log_bytes: bytes) -> None:
        if judge_custom_pmu_scene(self._sample_json):
            aic_pmu_events = AicPmuUtils.get_custom_pmu_events(self._sample_json.get("ai_core_profiling_events"))
        else:
            aic_pmu_events = AicPmuUtils.get_pmu_events(self._sample_json.get("ai_core_profiling_events"))
        for log_data in Utils.chunks(all_log_bytes, self.AICORE_LOG_SIZE):
            _aic_pmu_log = AicPmuBean.decode(log_data)
            total_time = self.calculate_total_time(_aic_pmu_log)
            self.calculate_pmu_list(_aic_pmu_log, aic_pmu_events, self._aic_data_list, total_time)


class V5AicCalculator(AicCalculator):
    """
    class used to parse aicore data by iter
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(file_list, sample_config)
        # table_name_list[:2]:'total_time(ms)', 'total_cycles', unused
        self.table_name_list = get_metrics_from_sample_config(
            self._project_path, StrConstant.AI_CORE_PROFILING_METRICS
        )[2:]

    def calculate(self: any) -> None:
        """
        calculate the ai core
        :return: None
        """
        db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_METRICS_SUMMARY)
        if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_METRIC_SUMMARY):
            logging.info(
                "The Table %s already exists in the %s, and won't be calculate again.",
                DBNameConstant.TABLE_METRIC_SUMMARY,
                DBNameConstant.DB_METRICS_SUMMARY,
            )
            return
        self._parse_without_decode()

    def save(self: any) -> None:
        """
        save ai core data
        :return: None
        """
        if not self._aic_data_list:
            return
        with V5AicPmuModel(self._project_path) as _model:
            _model.flush(self._aic_data_list)

    def ms_run(self: any) -> None:
        """
        entrance or ai core calculator
        :return: None
        """
        if self._sample_json.get('ai_core_profiling_mode') == StrConstant.AIC_SAMPLE_BASED_MODE:
            return

        self.init_params()
        self.calculate()
        self.save()

    def _parse_without_decode(self: any) -> None:
        with V5StarsViewModel(self._project_path) as _model:
            pmu_data = _model.get_v5_pmu_details()
            aic_pmu_events = AicPmuUtils.get_pmu_events(self._sample_json.get("ai_core_profiling_events"))
        for data in pmu_data:
            core_num = self._core_num_dict.get("aic")
            total_time = Utils.cal_total_time(data.total_cycle, int(self._freq), data.block_num, core_num)
            self.calculate_pmu_list(data, aic_pmu_events, self._aic_data_list, total_time)
