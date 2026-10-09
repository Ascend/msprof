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

from abc import ABCMeta
from abc import abstractmethod


class IMetrics(metaclass=ABCMeta):
    """
    interface for data metrics
    """

    @staticmethod
    def get_division(divisor: any, dividend: any) -> float:
        """
        get divisor / dividend with specific decimal place.
        """
        if dividend == 0:
            return 0
        return divisor / dividend

    @staticmethod
    def get_mul(value1: any, value2: any) -> float:
        """
        get float value of value1 * value2
        """
        return 1.0 * value1 * value2

    @abstractmethod
    def run_rules(self: any) -> any:
        """
        run the metric rules
        :return: result for running rules
        """
