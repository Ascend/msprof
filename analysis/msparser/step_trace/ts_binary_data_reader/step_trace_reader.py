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

from common_func.db_name_constant import DBNameConstant
from common_func.platform.chip_manager import ChipManager
from profiling_bean.struct_info.step_trace import StepTrace
from profiling_bean.struct_info.step_trace import StepTraceChipV6


class StepTraceReader:
    """
    class for step trace reader
    """

    def __init__(self: any) -> None:
        self._data = []
        self._table_name = DBNameConstant.TABLE_STEP_TRACE

    @property
    def data(self: any) -> list:
        """
        get data
        :return: data
        """
        return self._data

    @property
    def table_name(self: any) -> str:
        """
        get table_name
        :return: table_name
        """
        return self._table_name

    def read_binary_data(self: any, bean_data: any) -> None:
        """
        read step trace binary data and store them into list
        :param bean_data: binary data
        :return: None
        """
        decoder = StepTraceChipV6 if ChipManager().is_chip_v6() else StepTrace
        step_trace_bean = decoder.decode(bean_data)
        if step_trace_bean:
            self._data.append(
                (
                    step_trace_bean.index_id,
                    step_trace_bean.model_id,
                    step_trace_bean.timestamp,
                    step_trace_bean.stream_id,
                    step_trace_bean.task_id,
                    step_trace_bean.tag_id,
                )
            )
