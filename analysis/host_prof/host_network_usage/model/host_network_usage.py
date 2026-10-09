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
from common_func.info_conf_reader import InfoConfReader
from host_prof.host_prof_base.host_prof_data_base import HostProfDataBase


class HostNetworkUsage(HostProfDataBase):
    """
    host network usage model
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_HOST_NETWORK_USAGE, [DBNameConstant.TABLE_HOST_NETWORK_USAGE])

    def flush_data(self: any) -> None:
        """
        flush all cache data
        :return: None
        """
        self.insert_network_usage(self.cache_data)

    def insert_network_usage(self: any, network_usage_info: list) -> None:
        """
        insert network usage info to table
        :param network_usage_info: network usage info
        :return: None
        """
        insert_sql = "INSERT INTO {} VALUES(?, ?, ?, ?)".format(DBNameConstant.TABLE_HOST_NETWORK_USAGE)
        DBManager.executemany_sql(self.conn, insert_sql, network_usage_info)

    def has_network_usage_data(self: any) -> bool:
        """
        check has network usage data
        :return: check result
        """
        return DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_HOST_NETWORK_USAGE)

    def get_network_usage_data(self: any) -> dict:
        """
        get network usage data
        :return: network usage data
        """
        network_info_sql = "SELECT * from {}".format(DBNameConstant.TABLE_HOST_NETWORK_USAGE)
        network_info_list = DBManager.fetch_all_data(self.cur, network_info_sql)

        result = []
        for network_item in network_info_list:
            time_network = {
                "start": InfoConfReader().trans_into_local_time(network_item[0], is_host=True),
                "end": InfoConfReader().trans_into_local_time(network_item[1], is_host=True),
                "usage": network_item[2],
            }
            result.append(time_network)

        return {"data": result}
