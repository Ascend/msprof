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
from abc import ABC

from common_func.info_conf_reader import InfoConfReader
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.stars.low_power_model import LowPowerViewModel
from viewer.interface.base_viewer import BaseViewer


class LowPowerViewer(BaseViewer, ABC):
    """
    class for get lowpower sample data
    （1）Ddie有10个硬件数据：0-I2C_IMON回读；1-AVSBUS_BUS回读；2-AVSBUS_AIC回读；3-AIC结温；4-非AIC结温；5-MEM_DRAM_DIE结温；
        6-软件DVFS下发的AIC频率（不使用，通过软件ch10上报）；7-PPU上报的AIC平均频率；8-软件DVFS下发的BUS频率（不使用，
        通过软件ch11上报）；9-PPU上报的BUS平均频率；
        6个软件数据：10-软件DVFS下发的AIC频率；11-软件DVFS下发的BUS频率；12-EDP降频POWERBRAKE计数；13-EDP降频IWARNING2计数;
        14-EDP降频IWARNING1计数;15-EDP降频IWARNING0计数;
    （2）Udie有9个硬件数据：0-Udie结温；1-PMBUS0的回读数据0；2-PMBUS0的回读数据1；3-PMBUS1的回读数据0；4-PMBUS1的回读数据1；
        5-PMBUS2的回读数据0；6-PMBUS2的回读数据1；7-PMBUS3的回读数据0；8-PMBUS3的回读数据1；

    此处的软件数据和硬件数据不需要完全展示，为各形态芯片保持一致，仅展示AIC频率
    """

    D_DIE_ID_LIST = [0, 1]
    U_DIE_ID_LIST = [2, 3]
    DIE_ID_KEY = {
        0: "die 0",
        1: "die 1",
        2: "die 2",
        3: "die 3",
    }
    # 数据索引常量
    TIMESTAMP_INDEX = 0
    DIE_ID_INDEX = 1
    AIC_FREQ_INDEX = 12

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.model_list = {'low_power': LowPowerViewModel}

    def get_trace_timeline(self: any, datas: list) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        pid = InfoConfReader().get_json_pid_data()
        process_name = TraceViewHeaderConstant.PROCESS_AI_CORE_FREQ

        # Dynamically generate metadata for all die_ids in D_DIE_ID_LIST
        meta_data = [["process_name", pid, die_id, process_name] for die_id in self.D_DIE_ID_LIST]
        result = TraceViewManager.metadata_event(meta_data)
        column_trace_data = []

        for data in datas:
            local_time = InfoConfReader().trans_into_local_time(raw_timestamp=data[self.TIMESTAMP_INDEX])
            die_id = data[self.DIE_ID_INDEX]
            if die_id not in self.D_DIE_ID_LIST:
                continue
            if die_id not in self.DIE_ID_KEY:
                logging.error("Unexpected die id: %s.", die_id)
                continue

            value = data[self.AIC_FREQ_INDEX]
            die_process_name = f"{process_name} Die {die_id}"
            column_trace_data.append([die_process_name, local_time, pid, die_id, {"value": value}])

        result.extend(
            TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, column_trace_data)
        )
        return result
