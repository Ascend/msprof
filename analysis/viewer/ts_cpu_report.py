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

import os

from common_func.config_mgr import ConfigMgr
from common_func.db_manager import DBManager
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import error
from common_func.msvp_constant import MsvpConstant


class TsCpuReport:
    """
    ts cpu report class
    """

    FILE_NAME = os.path.basename(__file__)

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return TsCpuReport.__name__

    @staticmethod
    def _get_ts_cpu_data(cursor: any, result: list) -> list:
        top_function = {}
        total_data = []
        for key, value in result:
            if key in top_function:
                top_function[key] += value
            else:
                top_function[key] = value
        total_count = cursor.execute('select sum(count) from TsOriginalData where event="0x11"').fetchone()[0]
        if not NumberConstant.is_zero(total_count):
            tmp_res = []
            for key, value in list(top_function.items()):
                rate = round(float(value) * NumberConstant.PERCENTAGE / total_count, NumberConstant.ROUND_THREE_DECIMAL)
                tmp_res.append((key, value, rate))
            total_data = sorted(tmp_res, key=lambda x: x[1], reverse=True)
        return total_data

    def get_output_top_function(self: any, db_name: str, result_dir: str) -> tuple:
        """
        get cpu top function data
        """
        sample_config = ConfigMgr.pre_check_sample(result_dir, 'ts_cpu_profiling_events')
        if not sample_config:
            error(self.FILE_NAME, 'Failed to get sample configuration file.')
            return MsvpConstant.MSVP_EMPTY_DATA
        conn, cursor = DBManager.check_connect_db(result_dir, db_name)
        counter_exist = DBManager.judge_table_exist(cursor, "TsOriginalData")
        if not counter_exist:
            return MsvpConstant.MSVP_EMPTY_DATA
        sql = 'select function, count from TsOriginalData where event="0x11" order by count;'
        result = DBManager.fetch_all_data(cursor, sql)
        if not result:
            DBManager.destroy_db_connect(conn, cursor)
            return MsvpConstant.MSVP_EMPTY_DATA
        try:
            total_data = TsCpuReport._get_ts_cpu_data(cursor, result)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError):
            return MsvpConstant.MSVP_EMPTY_DATA
        finally:
            DBManager.destroy_db_connect(conn, cursor)
        headers = [
            'Function',
            'Cycles',
            'Cycles(%)',
        ]
        return headers, total_data[:5], len(total_data[:5])
