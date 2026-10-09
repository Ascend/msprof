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
from common_func.utils import Utils
from profiling_bean.struct_info.struct_decoder import StructDecoder


class AicPmuBean(StructDecoder):
    """
    class used to decode aic pmu data
    """

    last_cnt = -1

    def __init__(self: any, *args: any) -> None:
        filed = args[0]
        self._filed = filed
        self._stream_id = Utils.get_stream_id(filed[17])
        self._task_id = filed[4]
        self._total_cycle = filed[7]
        self._pmu_list = filed[9:17]
        # 获取CNT，识别对应的CNT是否按4bit（0~15）循环出现，判断数据是否存在丢失
        self._cnt = filed[0] >> 4
        self._pmu_reliable()

    @property
    def stream_id(self: any) -> int:
        """
        get stream_id
        :return: stream_id
        """
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        """
        get task_id
        :return: task_id
        """
        return self._task_id

    @property
    def total_cycle(self: any) -> int:
        """
        get total_cycle
        :return: total_cycle
        """
        return self._total_cycle

    @property
    def pmu_list(self: any) -> list:
        """
        get pmu_list
        :return: pmu_list
        """
        return self._pmu_list

    def _pmu_reliable(self):
        # 识别丢包场景
        if AicPmuBean.last_cnt == -1:
            AicPmuBean.last_cnt = self._cnt
            return
        expected_cnt = (AicPmuBean.last_cnt + 1) % 16
        if expected_cnt != self._cnt:
            logging.error(
                "An lost before the operator (stream id = %d, task id = %d) count has been detected",
                self._stream_id,
                self._task_id,
            )
        AicPmuBean.last_cnt = self._cnt
