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


class HwtsLogBean(StructDecoder):
    """
    class used to decode hwts log from bytes
    """

    last_cnt = -1

    def __init__(self: any, *args: any) -> None:
        # 3=0b00000011, keep 2 lower bits which represent log type
        args = args[0]
        self._task_type = args[1]
        self._stream_id = Utils.get_stream_id(args[6])
        self._task_id = args[4]
        self._sys_cnt = args[5]
        self._sys_tag = args[0] & 7
        cnt = args[0] >> 4
        self._hwts_log_reliable(cnt)

    @property
    def task_type(self: any) -> int:
        """
        for task type
        """
        return self._task_type

    @property
    def sys_tag(self: any) -> int:
        """
        for sys tag
        """
        return self._sys_tag

    @property
    def task_id(self: any) -> int:
        """
        for task id
        """
        return self._task_id

    @property
    def stream_id(self: any) -> int:
        """
        for stream id
        """
        return self._stream_id

    @property
    def sys_cnt(self: any) -> int:
        """
        for sys cnt
        """
        return self._sys_cnt

    def is_log_type(self: any) -> bool:
        """
        check hwts log type ,and 0 is start log ,1 is end log.
        :return:
        """
        return self._sys_tag in (0, 1)

    def _hwts_log_reliable(self, cnt):
        if HwtsLogBean.last_cnt == -1:
            HwtsLogBean.last_cnt = cnt
            return
        expected_cnt = (HwtsLogBean.last_cnt + 1) % 16
        if expected_cnt != cnt:
            logging.error(
                "An lost before the operator (stream id = %d, task id = %d) count has been detected",
                self._stream_id,
                self._task_id,
            )
        HwtsLogBean.last_cnt = cnt
