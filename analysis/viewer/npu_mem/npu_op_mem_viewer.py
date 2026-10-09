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

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from common_func.msvp_common import format_high_precision_for_csv
from msmodel.npu_mem.npu_ai_stack_mem_model import NpuAiStackMemModel


class NpuOpMemViewer:
    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._data = []
        self._model = NpuAiStackMemModel(
            self._project_path, DBNameConstant.DB_MEMORY_OP, [DBNameConstant.TABLE_NPU_OP_MEM]
        )

    def get_summary_data(self: any) -> tuple:
        """
        get summary data from npu mem data and format data for csv output
        :return: summary data
        """

        if not self._model.check_db() or not self._model.check_table():
            logging.warning(
                "operator memory data not found, maybe it failed to parse, please check the data parsing log."
            )
            return MsvpConstant.MSVP_EMPTY_DATA
        origin_summary_data = self._model.get_table_data(DBNameConstant.TABLE_NPU_OP_MEM)
        if origin_summary_data:
            self._data = [
                [
                    datum.name,
                    round(datum.size / NumberConstant.KILOBYTE, NumberConstant.ROUND_THREE_DECIMAL),
                    format_high_precision_for_csv(
                        InfoConfReader().trans_into_local_time(
                            InfoConfReader().time_from_host_syscnt(datum.allocation_time, NumberConstant.MICRO_SECOND),
                            use_us=True,
                            is_host=True,
                        )
                    ),
                    round(
                        InfoConfReader().get_host_duration(datum.duration, NumberConstant.MICRO_SECOND),
                        NumberConstant.ROUND_THREE_DECIMAL,
                    ),
                    round(
                        datum.allocation_total_allocated / NumberConstant.KILOBYTE, NumberConstant.ROUND_THREE_DECIMAL
                    ),
                    round(
                        datum.allocation_total_reserved / NumberConstant.KILOBYTE, NumberConstant.ROUND_THREE_DECIMAL
                    ),
                    round(datum.release_total_allocated / NumberConstant.KILOBYTE, NumberConstant.ROUND_THREE_DECIMAL),
                    round(datum.release_total_reserved / NumberConstant.KILOBYTE, NumberConstant.ROUND_THREE_DECIMAL),
                    datum.device_type,
                ]
                for datum in origin_summary_data
            ]
        return self._configs.get(StrConstant.CONFIG_HEADERS), self._data, len(self._data)
