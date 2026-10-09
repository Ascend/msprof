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

from common_func.ms_constant.number_constant import NumberConstant
from mscalculate.interface.imetrics import IMetrics


class HitRateMetric(IMetrics):
    """
    hit rate metrics
    """

    def __init__(self: any, hit_value: int, request_value: int) -> None:
        self.hit_value = hit_value
        self.request_value = request_value

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return HitRateMetric.__name__

    def run_rules(self: any) -> float:
        """
        run rules
        """
        return self._calculate_hit_rate()

    def _calculate_hit_rate(self: any) -> float:
        return round(self.get_division(self.hit_value, self.request_value), NumberConstant.DECIMAL_ACCURACY)


class VictimRateMetric(IMetrics):
    """
    victim rate metrics
    """

    def __init__(self: any, victim_value: int, request_value: int) -> None:
        self.victim_value = victim_value
        self.request_value = request_value

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return VictimRateMetric.__name__

    def run_rules(self: any) -> float:
        """
        run rules
        """
        return self._calculate_victim_rate()

    def _calculate_victim_rate(self: any) -> float:
        return round(self.get_division(self.victim_value, self.request_value), NumberConstant.DECIMAL_ACCURACY)
