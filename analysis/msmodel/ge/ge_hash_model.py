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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.ge_hash_dto import GeHashDto


class GeHashModel(ParserModel):
    """
    ge hash model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_GE_HASH, table_list)
        self.table_list = table_list

    def flush(self: any, data_list: list) -> None:
        """
        insert data to table
        :param data_list: ge hash data
        :return:
        """
        self.insert_data_to_db(self.table_list[0], data_list)

    def get_ge_hash_model_name(self: any) -> any:
        """
        get ge hash model name
        """
        return self.__class__.__name__


class GeHashViewModel(ViewModel):
    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_GE_HASH, [])

    def get_ge_hash_data(self) -> dict:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_GE_HASH):
            return {}
        sql = "SELECT hash_key, hash_value FROM {}".format(DBNameConstant.TABLE_GE_HASH)
        return dict(DBManager.fetch_all_data(self.cur, sql))

    def get_type_hash_data(self) -> dict:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_TYPE_HASH):
            return {}
        sql = "SELECT hash_key, hash_value, level FROM {}".format(DBNameConstant.TABLE_TYPE_HASH)
        hash_data = DBManager.fetch_all_data(self.cur, sql, dto_class=GeHashDto)
        res_data = {}
        for data in hash_data:
            res_data.setdefault(data.level, {}).update({data.hash_key: data.hash_value})
        return res_data
