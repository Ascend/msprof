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

import logging

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from mscalculate.hccl.hccl_task import ReportData


class ReportHcclStatisticData:
    """
    class to report hccl op data
    """

    @staticmethod
    def _get_report_sql(table: str) -> str:
        return (
            "select op_type, occurrences, "
            "round(total_time/{NS_TO_US}, {accuracy}) as total_time, "
            "round(min/{NS_TO_US}, {accuracy}) as min, "
            "round(avg/{NS_TO_US}, {accuracy}) as avg, "
            "round(max/{NS_TO_US}, {accuracy}) as max "
            "from {0}"
        ).format(table, NS_TO_US=NumberConstant.NS_TO_US, accuracy=NumberConstant.ROUND_THREE_DECIMAL)

    @classmethod
    def report_hccl_op(cls: any, db_path: str, headers: list) -> tuple:
        """从 TABLE_HCCL_OP_REPORT 和 TABLE_KFC_OP_REPORT 取数据，统一算ratio"""
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not (conn and curs):
            logging.warning("Failed to connect to the database %s.", DBNameConstant.DB_HCCL_SINGLE_DEVICE)
            return [], [], 0

        rows = []
        for table in (DBNameConstant.TABLE_HCCL_OP_REPORT, DBNameConstant.TABLE_KFC_OP_REPORT):
            if DBManager.judge_table_exist(curs, table):
                rows.extend(DBManager.fetch_all_data(curs, cls._get_report_sql(table), dto_class=ReportData))
        DBManager.destroy_db_connect(conn, curs)

        total_time_sum = sum(r.total_time for r in rows)
        result = []
        for r in rows:
            ratio = r.total_time * 100.0 / total_time_sum if total_time_sum else 0
            result.append(
                [
                    r.op_type,
                    r.occurrences,
                    round(r.total_time, NumberConstant.ROUND_THREE_DECIMAL),
                    round(r.min, NumberConstant.ROUND_THREE_DECIMAL),
                    round(r.avg, NumberConstant.ROUND_THREE_DECIMAL),
                    round(r.max, NumberConstant.ROUND_THREE_DECIMAL),
                    round(ratio, NumberConstant.ROUND_THREE_DECIMAL),
                ]
            )
        result.sort(key=lambda x: x[2], reverse=True)
        return headers, result, len(result)
