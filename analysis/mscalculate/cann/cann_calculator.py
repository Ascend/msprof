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
from typing import List

from common_func.constant import Constant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.cpp_enable_scene import CannCalculatorScene
from common_func.rt_add_info_center import RTAddInfoCenter
from mscalculate.cann.cann_analysis_chain import CANNAnalysisChain
from mscalculate.cann.cann_analysis_gear import HCCLGear, ACLGear
from mscalculate.cann.cann_analysis_gear import ModelGear
from mscalculate.cann.cann_analysis_gear import NodeGear
from mscalculate.cann.cann_analysis_gear import RootGear
from mscalculate.cann.cann_analysis_gear import TaskGear
from mscalculate.cann.cann_event_generator import CANNEventGenerator
from mscalculate.interface.icalculator import ICalculator
from msinterface.msprof_c_interface import dump_cann_trace


class CANNCalculator(ICalculator, MsMultiProcess):
    """
    calculator for total cann
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.event_generator = CANNEventGenerator(self._project_path)
        self.analysis_chains: List[CANNAnalysisChain] = []
        self.gears = dict()

    def calculate(self: any) -> None:
        """
        run the data calculators
        """
        # for future multi-process
        self.gears = {
            Constant.ROOT_LEVEL: RootGear(self._project_path),
            Constant.ACL_LEVEL: ACLGear(self._project_path),
            Constant.MODEL_LEVEL: ModelGear(self._project_path),
            Constant.NODE_LEVEL: NodeGear(self._project_path),
            Constant.TASK_LEVEL: TaskGear(self._project_path),
            Constant.HCCL_LEVEL: HCCLGear(self._project_path),
        }
        thread_dbs = self.event_generator.run()
        for db in thread_dbs:
            for gear in self.gears.values():
                gear.set_db(db)
            chain = CANNAnalysisChain(db.thread_id, db, self.gears)
            self.analysis_chains.append(chain)
            chain.start()

    def save(self: any) -> None:
        """
        save data to database
        """
        for gear in self.gears.values():
            gear.flush_data()

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        logging.info("start to analysis cann software callstack")
        if CannCalculatorScene().is_cpp_enable():
            dump_cann_trace(self._project_path)
            return
        else:
            logging.warning("Data will not be parsed by msprof_analysis.so!")
        RTAddInfoCenter(self._project_path)
        self.calculate()
        self.save()
