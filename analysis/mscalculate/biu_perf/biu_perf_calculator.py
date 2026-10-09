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

from common_func.constant import Constant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from mscalculate.biu_perf.biu_monitor_calculator import MonitorCyclesCalculator
from mscalculate.biu_perf.biu_monitor_calculator import MonitorFlowCalculator


class BiuPerfCalculator(MsMultiProcess):
    """
    calculate biu perf data
    """

    def __init__(self: any, _: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self.monitor_calculator_list = [MonitorFlowCalculator, MonitorCyclesCalculator]

    def ms_run(self: any) -> None:
        """
        calculate for biu perf
        :return: None
        """
        try:
            for monitor_calculator in self.monitor_calculator_list:
                monitor_calculator(self.sample_config).ms_run()
        except ProfException as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
