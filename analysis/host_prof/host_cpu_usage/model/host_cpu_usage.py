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
from common_func.msvp_common import is_number
from host_prof.host_prof_base.host_prof_data_base import HostProfDataBase


class HostCpuUsage(HostProfDataBase):
    """
    host cpu usage model
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(
            result_dir,
            DBNameConstant.DB_HOST_CPU_USAGE,
            [
                DBNameConstant.TABLE_HOST_CPU_INFO,
                DBNameConstant.TABLE_HOST_PROCESS_USAGE,
                DBNameConstant.TABLE_HOST_CPU_USAGE,
            ],
        )

    def insert_cpu_info_data(self: any, cpu_info: list) -> None:
        """
        insert cpu info to table
        :param cpu_info: cpu info
        :return: None
        """
        cpu_info_sql = "INSERT INTO {} VALUES(?, ?)".format(DBNameConstant.TABLE_HOST_CPU_INFO)
        DBManager.execute_sql(self.conn, cpu_info_sql, cpu_info)

    def insert_process_usage_data(self: any, process_usage_info: list) -> None:
        """
        insert process usage info to table
        :param process_usage_info: process usage info
        :return: None
        """
        process_usage_sql = "INSERT INTO {} VALUES(?, ?, ?, ?, ?, ?, ?)".format(DBNameConstant.TABLE_HOST_PROCESS_USAGE)
        DBManager.executemany_sql(self.conn, process_usage_sql, process_usage_info)

    def insert_cpu_usage_data(self: any, cpu_usage_info: list) -> None:
        """
        insert cpu usage info to table
        :param cpu_usage_info:cpu usage info
        :return: None
        """
        cpu_usage_sql = "INSERT INTO {} VALUES(?, ?, ?, ?)".format(DBNameConstant.TABLE_HOST_CPU_USAGE)
        DBManager.executemany_sql(self.conn, cpu_usage_sql, cpu_usage_info)

    def has_cpu_usage_data(self: any) -> bool:
        """
        check has cpu usage data
        :return: True or False
        """
        return DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_HOST_CPU_USAGE)

    def has_cpu_info_data(self: any) -> bool:
        """
        check has cpu info data
        :return: True or False
        """
        return DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_HOST_CPU_INFO)

    def get_cpu_list(self: any) -> list:
        """
        cpu list
        :return: cpu info list
        """
        cpu_list_sql = "SELECT DISTINCT(CPU_NO) FROM {} ORDER BY CAST(CPU_NO AS DECIMAL)".format(
            DBNameConstant.TABLE_HOST_CPU_USAGE
        )
        return DBManager.fetch_all_data(self.cur, cpu_list_sql)

    def get_cpu_info_data(self: any) -> dict:
        """
        cpu info data
        :return: cpu number
        """
        cpu_info_sql = "SELECT * FROM {}".format(DBNameConstant.TABLE_HOST_CPU_INFO)
        cpu_info = self.cur.execute(cpu_info_sql).fetchone()
        if cpu_info is None:
            return {"cpu_num": 0}
        return {"cpu_num": cpu_info[0]}

    def get_cpu_usage_data(self: any) -> list:
        """
        cpu usage data
        :return: cpu usage data
        """
        result = []
        cpu_usage_sql = "SELECT * FROM {0} order by CAST(cpu_no AS DECIMAL)".format(DBNameConstant.TABLE_HOST_CPU_USAGE)
        per_cpu_usage = DBManager.fetch_all_data(self.cur, cpu_usage_sql)
        for item in per_cpu_usage:
            if is_number(item[0]):
                time_cpu = [
                    "CPU " + str(item[2]),
                    InfoConfReader().trans_into_local_time(item[0], is_host=True),
                    {"Usage(%)": item[3]},
                ]
                result.append(time_cpu)

        return result

    def get_num_of_used_cpus(self: any) -> list:
        """
        get occupied cpu numbers and used cpu numbers
        """
        cpu_used_sql = "SELECT cpu_no, MAX(usage) FROM {0} where cpu_no != 'Avg' group by cpu_no".format(
            DBNameConstant.TABLE_HOST_CPU_USAGE
        )
        cpu_used_data = DBManager.fetch_all_data(self.cur, cpu_used_sql)
        num_of_occupied_cpus = len(cpu_used_data)
        num_of_used_cpus = len([cpu_no_data for cpu_no_data in cpu_used_data if cpu_no_data[1] > 0])
        return [num_of_occupied_cpus, num_of_used_cpus]

    @abstractmethod
    def flush_data(self: any) -> any:
        """
        flush all host cpu data
        """
