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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel


class LowPowerModel(ParserModel):
    """
    lowpower sample model class
    """

    def flush(self: any, data_list: list) -> None:
        """
        insert lowpower sample data into database
        这里存的timestamp不是sys cnt,而是调用time_from_syscnt返回的sys time
        """
        self.insert_data_to_db(DBNameConstant.TABLE_LOWPOWER, data_list)


class LowPowerViewModel(ViewModel):
    """
    lowpower sample view model class
    """

    def get_timeline_data(self):
        """
        get lowpower sample timeline data from database
        """
        return self.get_all_data(DBNameConstant.TABLE_LOWPOWER)

    def get_aicore_avg_freq_data(self: any) -> list:
        """
        从lowPower表中获取AIC平均频率数据（die_id=0和1的频率均值）
        col_0=sys_time, col_1=die_id, col_12=AIC_freq(MHz)
        :return: [(sys_time, avg_freq_MHz), ...] 按时间排序
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_LOWPOWER):
            return []
        sql = "SELECT timestamp, AVG(data0_soft) FROM {} WHERE die_id IN (0, 1) GROUP BY timestamp ORDER BY timestamp".format(
            DBNameConstant.TABLE_LOWPOWER
        )
        rows = DBManager.fetch_all_data(self.cur, sql)
        if not rows:
            return []
        return [(row[0], row[1]) for row in rows]
