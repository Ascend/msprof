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

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from common_func.msvp_common import format_high_precision_for_csv
from msmodel.npu_mem.npu_ai_stack_mem_model import NpuAiStackMemModel
from profiling_bean.prof_enum.data_tag import ModuleName


class NpuModuleMemViewer:
    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._data = []
        self._table = DBNameConstant.TABLE_NPU_MODULE_MEM
        self._model = NpuAiStackMemModel(
            self._project_path, DBNameConstant.DB_NPU_MODULE_MEM, [DBNameConstant.TABLE_NPU_MODULE_MEM]
        )

    def get_summary_data(self: any) -> tuple:
        """
        get summary data from npu mem data
        :return: summary data
        """
        if not self._model.check_db() or not self._model.check_table():
            logging.warning(
                "%s data not found, maybe it failed to parse, please check the data parsing log.", self._table
            )
            return MsvpConstant.MSVP_EMPTY_DATA
        origin_summary_data = self._model.get_table_data(self._table)
        if not origin_summary_data:
            logging.error("get %s summary data failed in npu module memory viewer, please check.", self._table)
            return MsvpConstant.MSVP_EMPTY_DATA
        return self._npu_module_mem_reformat(origin_summary_data)

    def _npu_module_mem_reformat(self, data_dict: dict) -> tuple:
        for datum in data_dict:
            try:
                module_name = ModuleName(datum.module_id).name
            except ValueError:
                logging.warning("Invalid module id, please check!")
                module_name = ModuleName.UNKNOWN.name
            if datum.total_size > 0:
                total_size = round(datum.total_size / NumberConstant.KILOBYTE, NumberConstant.ROUND_THREE_DECIMAL)
            else:
                total_size = datum.total_size
            self._data.append(
                [
                    module_name,
                    format_high_precision_for_csv(
                        InfoConfReader().trans_into_local_time(
                            InfoConfReader().time_from_host_syscnt(
                                int(datum.syscnt), NumberConstant.MICRO_SECOND, is_host=False
                            ),
                            use_us=True,
                        )
                    ),
                    total_size,
                    datum.device_type,
                ]
            )
        return self._configs.get(StrConstant.CONFIG_HEADERS), self._data, len(self._data)
