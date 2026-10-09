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

from common_func.ms_constant.str_constant import StrConstant
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from msmodel.add_info.static_op_mem_viewer_model import StaticOpMemViewModel


class StaticOpMemViewer:
    """
    class for get static_op_mem data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        self.configs = configs
        self.params = params

    def get_summary_data(self: any) -> tuple:
        """
        to get summary data
        :return:summary data
        """
        model = StaticOpMemViewModel(
            self.params.get(StrConstant.PARAM_RESULT_DIR),
            DBNameConstant.DB_STATIC_OP_MEM,
            [DBNameConstant.TABLE_STATIC_OP_MEM],
        )
        if not model.check_table():
            return MsvpConstant.MSVP_EMPTY_DATA
        static_op_mem_summary_data = model.get_summary_data()
        if not static_op_mem_summary_data:
            return MsvpConstant.MSVP_EMPTY_DATA
        return self.configs.get(StrConstant.CONFIG_HEADERS), static_op_mem_summary_data, len(static_op_mem_summary_data)
