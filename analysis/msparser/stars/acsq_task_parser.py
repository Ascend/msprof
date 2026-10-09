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


from common_func.ms_constant.stars_constant import StarsConstant
from common_func.platform.chip_manager import ChipManager
from msmodel.stars.acsq_task_model import AcsqTaskModel
from msparser.stars.log_base_parser import LogBaseParser
from profiling_bean.stars.acsq_task import AcsqTask
from profiling_bean.stars.acsq_task_v6_bean import AcsqTaskV6


class AcsqTaskParser(LogBaseParser):
    """
    class used to parser acsq task log
    """

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__(result_dir)
        self._model = AcsqTaskModel(result_dir, db, table_list)
        self._decoder = AcsqTaskV6 if ChipManager().is_chip_v6() else AcsqTask
        self._data_list = []
        self._mismatch_task = []
        self._start_functype = StarsConstant.ACSQ_START_FUNCTYPE
        self._end_functype = StarsConstant.ACSQ_END_FUNCTYPE
