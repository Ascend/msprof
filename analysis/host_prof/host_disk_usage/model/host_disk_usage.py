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

from abc import abstractmethod

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from host_prof.host_prof_base.host_prof_data_base import HostProfDataBase


class HostDiskUsage(HostProfDataBase):
    """
    host disk usage model
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_HOST_DISK_USAGE, [DBNameConstant.TABLE_HOST_DISK_USAGE])

    def insert_disk_usage_data(self: any, disk_usage_info: list) -> None:
        """
        insert disk usage info to table
        :param disk_usage_info: disk usage info
        :return: None
        """
        disk_usage_sql = "INSERT INTO {} VALUES(?, ?, ?, ?, ?, ?)".format(DBNameConstant.TABLE_HOST_DISK_USAGE)
        DBManager.executemany_sql(self.conn, disk_usage_sql, disk_usage_info)

    def has_disk_usage_data(self: any) -> bool:
        """
        check has disk usage data
        :return: check result
        """
        return DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_HOST_DISK_USAGE)

    def get_disk_usage_data(self: any) -> dict:
        """
        disk usage data
        :return: disk usage data
        """
        disk_info_sql = "SELECT * from {}".format(DBNameConstant.TABLE_HOST_DISK_USAGE)
        disk_info_list = DBManager.fetch_all_data(self.cur, disk_info_sql)

        result = []
        for disk_item in disk_info_list:
            time_mem = {
                "start": InfoConfReader().trans_into_local_time(disk_item[0], is_host=True),
                "end": InfoConfReader().trans_into_local_time(disk_item[1], is_host=True),
                "usage": disk_item[5],
            }
            result.append(time_mem)

        return {"data": result}

    @abstractmethod
    def flush_data(self: any) -> any:
        """
        flush all host disk data
        """
