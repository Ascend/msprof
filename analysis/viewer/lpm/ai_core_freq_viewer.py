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

import copy
from collections import OrderedDict
from decimal import Decimal

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.api.api_data_viewer_model import ApiDataViewModel
from msmodel.freq.freq_data_viewer_model import FreqDataViewModel
from viewer.lpm.lpm_viewer import LpmConvInfoViewer


class AiCoreFreqViewer(LpmConvInfoViewer):
    """
    Read aicore freq and generate aicore freq view
    """

    def __init__(self, params: dict):
        super().__init__(params)
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._pid = InfoConfReader().get_json_pid_data()
        self.freq_model = FreqDataViewModel(params)
        self.apidata_model = ApiDataViewModel(params)

    def get_all_data(self: any) -> list:
        """
        1、read the fixed aicore freq from info.json
        2、read the floating aicore freq from freq.db
        3、concat 1/2 freq
        """
        result = []
        flag = (
            not ChipManager().is_chip_v4()
            and not ChipManager().is_chip_v1_1_1()
            and not ChipManager().is_chip_v3_3()
            or InfoConfReader().is_host_profiling()
        )
        if flag:
            return result

        # add header for freq view
        result.extend(
            TraceViewManager.metadata_event(
                [
                    [
                        "process_name",
                        self._pid,
                        InfoConfReader().get_json_tid_data(),
                        TraceViewHeaderConstant.PROCESS_AI_CORE_FREQ,
                    ]
                ]
            )
        )
        # freq unit is MHZ
        freq_lists = []
        with self.freq_model as _model:
            freq_rows = _model.get_data()
            if not freq_rows:
                freq_rows.append(
                    (
                        InfoConfReader().get_dev_cnt(),
                        InfoConfReader().get_freq(StrConstant.AIC) / NumberConstant.FREQ_TO_MHz,
                    )
                )
            for row in freq_rows:
                # row index 0 is syscnt, and row index 1 is frequency
                to_local_ts = InfoConfReader().trans_syscnt_into_local_time(row[0])
                data_list = [
                    TraceViewHeaderConstant.PROCESS_AI_CORE_FREQ,
                    to_local_ts,
                    self._pid,
                    0,
                    OrderedDict({"MHz": row[1]}),
                ]
                freq_lists.append(data_list)
        _, end_ts = InfoConfReader().get_collect_time()
        if not end_ts:
            end_ts = str(Decimal(freq_lists[-1][1]) + 1)
        final_data = copy.deepcopy(freq_lists[-1])
        # 增加一条记录，时间替换为采集结束时间，截断柱状图
        final_data[1] = end_ts
        freq_lists.append(final_data)
        filled_freq = self._split_and_fill_data(freq_lists)
        changed_frequency = TraceViewManager.column_graph_trace(
            TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, filled_freq
        )
        result.extend(changed_frequency)

        return result
