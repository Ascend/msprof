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

# pylint: disable=duplicate-code

from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class GeFusionModel(ParserModel):
    """
    ge model info model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_GE_MODEL_INFO, table_list)
        self._current_table_name = None

    def flush_all(self: any, data_dict: dict) -> None:
        """
        insert all ge fusion data to table
        :param data_dict: ge fusion data
        :return:
        """
        for table_name in data_dict.keys():
            self._current_table_name = table_name
            self.flush(data_dict.get(table_name, []))

    def flush(self: any, data_list: list) -> None:
        """
        insert one table ge fusion data into database
        """
        self.insert_data_to_db(self._current_table_name, data_list)

    def get_ge_fusion_model_name(self: any) -> any:
        """
        get ge fusion model name
        """
        return self.__class__.__name__
