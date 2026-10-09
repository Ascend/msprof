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

from msparser.stars.stars_log_parser import StarsLogCalCulator
from profiling_bean.prof_enum.data_tag import DataTag


class SocProfilerParser(StarsLogCalCulator):
    """
    to read and parse stars data in soc_profiler buffer
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(file_list, sample_config)
        self._file_list = file_list.get(DataTag.SOC_PROFILER, [])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        self._fmt_size = self.DEFAULT_FMT_SIZE
