# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

from common_func.ms_constant.stars_constant import StarsConstant
from msmodel.stars.block_log_model import BlockLogModel
from msparser.stars.log_base_parser import LogBaseParser
from profiling_bean.stars.block_log_bean import BlockLogBean


class BlockLogParser(LogBaseParser):
    """
    class used to parser AIC/AIV block log
    """

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__(result_dir)
        self._model = BlockLogModel(result_dir, db, table_list)
        self._decoder = BlockLogBean
        self._data_list = []
        self._mismatch_task = []
        self._start_functype = StarsConstant.BLOCK_LOG_START_FUNCTYPE
        self._end_functype = StarsConstant.BLOCK_LOG_END_FUNCTYPE

    def flush(self: any) -> None:
        """
        flush data to model
        :return: NA for block log data is now disabled
        """
        return
