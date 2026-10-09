# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.voltage.voltage_data_viewer_model import AicVoltageViewerModel
from msmodel.voltage.voltage_data_viewer_model import BusVoltageViewerModel
from viewer.lpm.lpm_viewer import LpmConvInfoViewer


class VoltageViewer(LpmConvInfoViewer):
    """
    Read aicore voltage and generate aicore voltage view
    """

    def __init__(self, params: dict):
        super().__init__(params)
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._pid = InfoConfReader().get_json_pid_data()
        self.aic_voltage_model = AicVoltageViewerModel(params)
        self.bus_voltage_model = BusVoltageViewerModel(params)

    def get_changed_voltage(self, voltage_model) -> list:
        """
        generate changed voltage data list body
        :input: voltage_model [aic_voltage_model, bus_voltage_model]
        :return: changed voltage
        """
        voltage_lists = []
        voltage_model.init()
        voltage_rows = voltage_model.get_data()
        if not voltage_rows:
            return voltage_lists
        for row in voltage_rows:
            # row index 0 is syscnt, and row index 1 is voltage
            to_local_ts = InfoConfReader().trans_syscnt_into_local_time(row[0])
            data_list = [voltage_model.PROCESSOR_NAME, to_local_ts, self._pid, 0, OrderedDict({"mV": row[1]})]
            voltage_lists.append(data_list)
        voltage_model.finalize()

        _, end_ts = InfoConfReader().get_collect_time()
        if not end_ts:
            end_ts = str(Decimal(voltage_lists[-1][1]) + 1)
        final_data = copy.deepcopy(voltage_lists[-1])
        # 增加一条记录，时间替换为采集结束时间，截断柱状图
        final_data[1] = end_ts
        voltage_lists.append(final_data)
        filled_voltage = self._split_and_fill_data(voltage_lists)
        changed_voltage = TraceViewManager.column_graph_trace(
            TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, filled_voltage
        )

        return changed_voltage

    def get_all_data(self):
        """
        1、extend voltage info line with header
        2、get aic voltage info and bus voltage info
        3、concat voltage result
        """
        result = []
        if not ChipManager().is_chip_v4() or InfoConfReader().is_host_profiling():
            return result

        # get_aic_voltage_data
        changed_aic_voltage = self.get_changed_voltage(self.aic_voltage_model)
        result.extend(changed_aic_voltage)

        # get_bus_voltage_data
        changed_bus_voltage = self.get_changed_voltage(self.bus_voltage_model)
        result.extend(changed_bus_voltage)

        # add header
        if not result:
            return []

        result.extend(
            TraceViewManager.metadata_event(
                [
                    [
                        "process_name",
                        self._pid,
                        InfoConfReader().get_json_tid_data(),
                        TraceViewHeaderConstant.PROCESS_VOLTAGE,
                    ]
                ]
            )
        )
        return result
