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

from common_func.ms_constant.number_constant import NumberConstant


class SqlWhereCondition:
    @staticmethod
    def get_interval_intersection_condition(
        start: float, end: float, table_name: str, start_col_name: str, end_col_name: str
    ) -> str:
        """
        This function generates a condition to filter the data that intersects the interval.
        """
        condition = ""
        if start != float("-inf"):
            condition += f"{table_name}.{end_col_name} < {start}"
            if end != float("inf"):
                condition += f" or {table_name}.{start_col_name} > {end}"
        elif end != float("inf"):
            condition += f"{table_name}.{start_col_name} > {end}"
        if condition:
            return f"where not ({condition})"
        return condition

    @staticmethod
    def get_host_select_condition(is_all_model_iter: bool, model_id: int, iter_id: int, device_id: int):
        if is_all_model_iter:
            return f"where device_id={device_id}"
        condition = (
            f"where model_id={model_id} and (index_id={iter_id} or"
            f" index_id={NumberConstant.STATIC_GRAPH_INDEX}) and device_id={device_id}"
        )
        return condition
