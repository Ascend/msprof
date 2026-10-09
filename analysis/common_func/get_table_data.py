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


class GetTableData:
    """
    class to manage DB operation
    """

    @staticmethod
    def get_table_data_for_device(cursor: any, table_name: str, device_id: str) -> list:
        """
        get table data for certain device
        """
        if not cursor:
            return []
        search_data_sql = (
            "select duration, bandwidth, "
            "rxBandwidth, rxPacket, rxErrorRate, "
            "rxDroppedRate, txBandwidth, txPacket, txErrorRate, txDroppedRate, funcId "
            "from {0} where device_id={1} order by rowid".format(table_name, device_id)
        )
        result = DBManager.fetch_all_data(cursor, search_data_sql)
        if result:
            return result
        return []

    @staticmethod
    def get_data_count_for_device(cursor: any, table_name: str, device_id: str) -> any:
        """
        get table data for certain device
        """
        if not cursor:
            return 0
        sql = "select count(*) from {} where device_id={}".format(table_name, device_id)
        return cursor.execute(sql).fetchone()[0]
