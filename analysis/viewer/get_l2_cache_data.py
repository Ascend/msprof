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

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.msvp_constant import MsvpConstant
from common_func.platform.chip_manager import ChipManager
from common_func.utils import Utils


def get_l2_cache_data(db_path: str, table_name: str, unused_cols: list) -> tuple:
    """
    get l2 cache data
    """
    conn, cursor = DBManager.check_connect_db_path(db_path)
    if not (conn and cursor) or not DBManager.judge_table_exist(cursor, table_name):
        return MsvpConstant.MSVP_EMPTY_DATA
    used_cols = DBManager.get_filtered_table_headers(cursor, table_name, *unused_cols)
    data = DBManager.fetch_all_data(cursor, "select {} from {}".format(",".join(used_cols), table_name))
    modify_l2_cache_headers(used_cols)
    DBManager.destroy_db_connect(conn, cursor)
    return used_cols, data, len(data)


def modify_l2_cache_headers(headers: list) -> None:
    """
    modify l2 cache headers
    """
    for index, header in enumerate(headers):
        headers[index] = " ".join(Utils.generator_to_list(letter.capitalize() for letter in header.split("_")))


def add_op_name(l2_cache_header: list, l2_cache_data: list, op_dict: dict) -> bool:
    """
    add operator name
    """
    if not {"Task Id", "Stream Id"}.issubset(l2_cache_header):
        return False
    task_id_inx = l2_cache_header.index("Task Id")
    stream_id_inx = l2_cache_header.index("Stream Id")
    for index, sub in enumerate(l2_cache_data):
        key = "{}-{}".format(sub[task_id_inx], sub[stream_id_inx])  # key is task_id-stream_id
        tmp = list(sub)
        tmp.append(op_dict[key] if key in op_dict else Constant.NA)
        l2_cache_data[index] = tmp
    return True


def process_hit_rate(l2_cache_header: list, l2_cache_data: list) -> list:
    if not ChipManager().is_stars_chip():
        return l2_cache_data
    if "Hit Rate" not in l2_cache_header:
        return l2_cache_data
    hit_rate_inx = l2_cache_header.index("Hit Rate")
    return [
        _l2_cache_data[:hit_rate_inx] + (Constant.NA,) + _l2_cache_data[hit_rate_inx + 1 :]
        for _l2_cache_data in l2_cache_data
    ]
