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
import sqlite3

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from msmodel.interface.view_model import ViewModel


def get_ge_model_data(params: dict, table_name: str, configs: dict) -> tuple:
    """
    get ge model data
    """
    result_data = []
    project_path = params.get(StrConstant.PARAM_RESULT_DIR)
    search_data_sql = (
        "select {0}.model_name, {1}.model_id, fusion_name, op_names, memory_input/{BYTES_TO_KB}, "
        "memory_output/{BYTES_TO_KB}, memory_weight/{BYTES_TO_KB}, memory_workspace/{BYTES_TO_KB},"
        " memory_total/{BYTES_TO_KB} from {1} inner join {0} where {0}.model_id={1}.model_id ".format(
            table_name, DBNameConstant.TABLE_GE_FUSION_OP_INFO, BYTES_TO_KB=NumberConstant.BYTES_TO_KB
        )
    )
    model_view = ViewModel(project_path, configs.get(StrConstant.CONFIG_DB), [table_name])
    if not model_view.check_table():
        return MsvpConstant.MSVP_EMPTY_DATA
    data = model_view.get_sql_data(search_data_sql)
    if not data:
        return MsvpConstant.MSVP_EMPTY_DATA
    hash_dict = get_ge_hash_dict(project_path)
    _update_hash_data(data, hash_dict, result_data)
    return configs.get(StrConstant.CONFIG_HEADERS), result_data, len(result_data)


def _update_hash_data(data: list, hash_dict: dict, result_data: list) -> None:
    for _data in data:
        _data = list(_data)
        _data[0] = hash_dict.get(_data[0], _data[0])
        _data[2] = hash_dict.get(_data[2], _data[2])
        _data[3] = ";".join(map(str, [hash_dict.get(str(i), i) for i in list(_data[3].split(","))]))
        result_data.append(_data)


def get_ge_hash_dict(project_path: str) -> dict:
    """
    get ge hash dict
    """
    hash_view = ViewModel(project_path, DBNameConstant.DB_GE_HASH, [DBNameConstant.TABLE_GE_HASH])
    try:
        if not hash_view.check_table():
            return {}
        return dict(hash_view.get_all_data(DBNameConstant.TABLE_GE_HASH))
    except sqlite3.Error as err:
        logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        return {}
    finally:
        hash_view.finalize()


def get_ge_model_name_dict(project_path: str) -> dict:
    """
    get ge model name dict
    """
    model_view = ViewModel(project_path, DBNameConstant.DB_GE_MODEL_INFO, [DBNameConstant.TABLE_MODEL_NAME])
    try:
        if not model_view.check_table():
            model_view.finalize()
            return {}
    except sqlite3.Error as err:
        logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        return {}
    sql = "select model_id, model_name from {}".format(DBNameConstant.TABLE_MODEL_NAME)
    data = model_view.get_sql_data(sql)
    model_view.finalize()
    hash_dict = get_ge_hash_dict(project_path)
    model_name_list = []
    for _data in data:
        _data = list(_data)
        _data[1] = hash_dict.get(_data[1], _data[1])
        model_name_list.append(_data)
    return dict(model_name_list)
