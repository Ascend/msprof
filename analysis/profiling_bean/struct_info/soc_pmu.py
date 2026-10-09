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
import struct

from msparser.data_struct_size_constant import StructFmt
from msparser.interface.idata_bean import IDataBean


class SocPmuBean(IDataBean):
    """
    soc pmu bean data for the data parsing by soc pmu parser.
    """

    SOC_PMU_EVENT_START_INDEX = 12
    SOC_PMU_DATA_LEN = 20

    def __init__(self: any) -> None:
        self._task_type = None
        self._stream_id = None
        self._task_id = None
        self._data_type = None
        self._events_list = []

    @property
    def task_type(self: any) -> int:
        """
        soc pmu task type
        """
        return self._task_type

    @property
    def task_id(self: any) -> int:
        """
        soc pmu task id
        """
        return self._task_id

    @property
    def events_list(self: any) -> list:
        """
        soc pmu events list
        """
        return self._events_list

    @property
    def stream_id(self: any) -> int:
        """
        soc pmu stream id
        """
        return self._stream_id

    @property
    def data_type(self: any) -> int:
        """
        soc pmu stream id
        """
        return self._data_type

    def decode(self: any, bin_data: bytes) -> None:
        """
        decode the soc pmu bin data
        :param bin_data: soc pmu bin data
        :return: instance of soc pmu
        """
        if not self.construct_bean(struct.unpack(StructFmt.SOC_PMU_FMT, bin_data)):
            logging.error("soc pmu data struct is incomplete, please check the soc pmu file.")

    def construct_bean(self: any, *args: any) -> bool:
        """
        refresh the soc pmu data
        :param args: soc pmu data
        :return: True or False
        """
        soc_pmu_data = args[0]
        if len(soc_pmu_data) != self.SOC_PMU_DATA_LEN:
            return False
        self._task_type = soc_pmu_data[8]
        self._stream_id = soc_pmu_data[9]
        self._task_id = soc_pmu_data[10]
        self._data_type = soc_pmu_data[11]
        for _event_index in range(self.SOC_PMU_EVENT_START_INDEX, self.SOC_PMU_DATA_LEN):
            self._events_list.append(str(soc_pmu_data[_event_index]))
        return True


class SocPmuChip6Bean(SocPmuBean):
    """
    soc pmu chip6 bean data for the data parsing by soc pmu parser.
    """

    SOC_PMU_EVENT_START_INDEX = 10
    SOC_PMU_DATA_LEN = 18

    def decode(self: any, bin_data: bytes) -> None:
        """
        decode the soc pmu bin data for chip v6
        :param bin_data: soc pmu bin data
        :return: instance of soc pmu
        """
        if not self.construct_bean(struct.unpack(StructFmt.SOC_PMU_CHIP6_FMT, bin_data)):
            logging.error("soc pmu data struct is incomplete, please check the soc pmu file.")

    def construct_bean(self: any, *args: any) -> bool:
        """
        refresh the soc pmu data
        :param args: soc pmu data
        :return: True or False
        """
        soc_pmu_data = args[0]
        if len(soc_pmu_data) != self.SOC_PMU_DATA_LEN:
            return False
        self._data_type = soc_pmu_data[6]
        self._task_type = soc_pmu_data[7]
        self._stream_id = soc_pmu_data[8]
        self._task_id = soc_pmu_data[9]
        for _event_index in range(self.SOC_PMU_EVENT_START_INDEX, self.SOC_PMU_DATA_LEN):
            self._events_list.append(str(soc_pmu_data[_event_index]))
        return True
