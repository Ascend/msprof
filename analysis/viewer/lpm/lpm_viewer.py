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

import copy
from abc import abstractmethod

# fill data step, 1000us(1ms)
FILL_STEP = 1000


class LpmConvInfoViewer:
    """
    base viewer for lpm conv data
    """

    def __init__(self: any, params: dict) -> None:
        self._params = params

    @staticmethod
    def _split_and_fill_data(data_lists) -> list:
        """
        fill the changed voltage list, filled voltage same with pre value
        :input: voltage lists
        :return: filled result list
        """
        if not data_lists:
            return []

        result = [data_lists[0]]  # 初始化结果列表，包含第一个元素

        for next_item in data_lists[1:]:
            current = result[-1]  # 当前处理的元素是结果列表的最后一个元素
            current_time = float(current[1])
            next_time = float(next_item[1])
            time_diff = next_time - current_time

            # 如果时间差超过1000us(1ms)，填充中间数据
            # 确保填充时间不超过next_time
            if time_diff > FILL_STEP:
                num_fill = min(int(time_diff // FILL_STEP), int((next_time - 1 - current_time) // FILL_STEP))
                for slice_time in range(1, num_fill + 1):
                    fill_time = current_time + slice_time * FILL_STEP
                    new_item = copy.deepcopy(current)
                    new_item[1] = str(fill_time)
                    result.append(new_item)

            # 将下一个元素添加到结果列表
            result.append(next_item)
        return result

    @abstractmethod
    def get_all_data(self: any) -> list:
        """
        base method to get timeline data
        """
        pass
