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
from collections import namedtuple

from common_func.db_name_constant import DBNameConstant
from common_func.db_manager import DBManager
from common_func.msprof_object import CustomizedNamedtupleFactory
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel


class GeModel(ParserModel):
    """
    ge info model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_GE_INFO, table_list)
        self._current_table_name = None

    def flush_all(self: any, data_dict: dict) -> None:
        """
        insert all ge data to table
        :param data_dict: ge data
        :return:
        """
        for table_name in data_dict.keys():
            self._current_table_name = table_name
            self.flush(data_dict.get(table_name, []))

    def flush(self: any, data_list: list) -> None:
        """
        insert ge data into database
        """
        self.insert_data_to_db(self._current_table_name, data_list)

    def delete_table(self: any, table_name: str) -> None:
        """
        delete ge data
        """
        self.cur.execute('delete from {}'.format(table_name))  # nosec

    def get_ge_model_name(self: any) -> any:
        """
        get ge model name
        """
        return self.__class__.__name__


class GeInfoViewModel(ViewModel):
    TASK_INFO_TYPE = CustomizedNamedtupleFactory.enhance_namedtuple(
        namedtuple(
            "TaskInfo",
            [
                "model_id",
                "op_name",
                "stream_id",
                "task_id",
                "block_num",
                "mix_block_num",
                "op_state",
                "task_type",
                "op_type",
                "index_id",
                "thread_id",
                "timestamp",
                "batch_id",
                "tensor_num",
                "input_formats",
                "input_data_types",
                "input_shapes",
                "output_formats",
                "output_data_types",
                "output_shapes",
                "device_id",
                "context_id",
                "op_flag",
                "hashid",
            ],
        ),
        {},
    )

    def __init__(self, result_dir: str, table_list: list):
        super().__init__(result_dir, DBNameConstant.DB_GE_INFO, table_list)

    def get_ge_info_by_device_id(self: any, table_name: str, device_id: str, task_type_filter: tuple = tuple()) -> any:
        fields = ", ".join(self.TASK_INFO_TYPE._fields)
        ge_sql = "select {0} from {1} where device_id={2} ".format(fields, table_name, device_id)  # nosec
        condition = ""
        for t in task_type_filter:
            condition += " AND task_type != '{0}' ".format(t)
        ge_sql = ge_sql + condition
        task_info_data = DBManager.fetch_all_data(self.cur, ge_sql)
        return [self.TASK_INFO_TYPE(*data) for data in task_info_data]
