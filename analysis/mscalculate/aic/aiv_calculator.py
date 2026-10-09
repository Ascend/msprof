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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.utils import Utils
from mscalculate.aic.aic_calculator import AicCalculator
from mscalculate.aic.aic_utils import AicPmuUtils
from msmodel.aic.aiv_pmu_model import AivPmuModel
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.aiv_pmu import AivPmuBean
from viewer.calculate_rts_data import judge_custom_pmu_scene
from viewer.calculate_rts_data import get_metrics_from_sample_config


class AivCalculator(AicCalculator, MsMultiProcess):
    """
    class used to parse aicore data by iter
    """

    AICORE_LOG_SIZE = 128

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(file_list, sample_config)
        self._file_list = file_list.get(DataTag.AIV, [])
        self._aiv_data_list = []
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        self.core_type = 1
        # table_name_list[:2]:'total_time(ms)', 'total_cycles', unused
        self.table_name_list = get_metrics_from_sample_config(self._project_path, StrConstant.AIV_PROFILING_METRICS)[2:]

    def aiv_calculate(self: any) -> None:
        """
        calculate for ai vector core
        :return: None
        """
        self._parse_all_file()

    def save(self: any) -> None:
        """
        :return:
        """
        if self._aiv_data_list:
            with AivPmuModel(self._project_path) as aiv_pmu_model:
                aiv_pmu_model.flush(self._aiv_data_list)

    def ms_run(self: any) -> None:
        """
        :return:
        """
        if self._sample_json.get('aiv_profiling_mode') == StrConstant.AIC_SAMPLE_BASED_MODE:
            return
        db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_METRICS_SUMMARY)
        if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_METRIC_SUMMARY):
            logging.info(
                "The Table %s already exists in the %s, and won't be calculate again.",
                DBNameConstant.TABLE_METRIC_SUMMARY,
                DBNameConstant.DB_METRICS_SUMMARY,
            )
            return
        self.init_params()
        if self._file_list:
            self.aiv_calculate()
            self.save()

    def _parse(self: any, all_log_bytes: bytes) -> None:
        if judge_custom_pmu_scene(self.sample_config, metrics_type='aiv_metrics'):
            aic_pmu_events = AicPmuUtils.get_custom_pmu_events(self._sample_json.get('aiv_profiling_events'))
        else:
            aic_pmu_events = AicPmuUtils.get_pmu_events(self._sample_json.get('aiv_profiling_events'))
        for log_data in Utils.chunks(all_log_bytes, self.AICORE_LOG_SIZE):
            _aic_pmu_log = AivPmuBean.decode(log_data)
            total_time = self.calculate_total_time(_aic_pmu_log, data_type='aiv')
            self.calculate_pmu_list(_aic_pmu_log, aic_pmu_events, self._aiv_data_list, total_time)
