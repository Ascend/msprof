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

from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class SubTaskTimeModel(ParserModel):
    """
    subtask time model class
    """

    def __init__(self: any, result_dir: str):
        super().__init__(result_dir, DBNameConstant.DB_SOC_LOG, [DBNameConstant.TABLE_SUBTASK_TIME])

    def flush(self: any, data_list: list) -> None:
        """
        flush subtask time data to db
        :param data_list:subtask time data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_SUBTASK_TIME, data_list)
