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
# pylint: skip-file

import logging

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.profiling_scene import ProfilingScene
from common_func.hccl_info_common import DeviceHcclSource
from mscalculate.hccl.hccl_task import HcclOps
from mscalculate.hccl.hccl_task import HcclTask
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.time_section_dto import CommunicationTimeSection


class HCCLModel(ParserModel):
    """
    acsq task model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_HCCL_SINGLE_DEVICE, table_list)

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE) -> None:
        """
        insert data to table
        :param data_list: hccl data
        :param table_name: table to insert hccl data
        :return:
        """
        self.insert_data_to_db(table_name, data_list)

    def get_hccl_data(self: any) -> list:
        """
        get hccl data
        :return:
        """
        sql = "select * from {}".format(DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE)  # nosec B608
        data = DBManager.fetch_all_data(self.cur, sql, dto_class=HcclTask)
        return data


class HcclViewModel(ViewModel):
    def __init__(self, result_dir: str, db_name: str, table_list: list):
        super().__init__(result_dir, db_name, table_list)

    @classmethod
    def get_task_time_sql(cls):
        select_sql = (
            "(select {0}.model_id, {0}.index_id, {0}.stream_id, {0}.task_id, "  # nosec B608
            "{0}.batch_id, {0}.context_id, {0}.start_time as running, "
            "{0}.start_time + {0}.duration as complete from {0} )".format(DBNameConstant.TABLE_ASCEND_TASK)
        )
        return select_sql

    def rebuild_hccl_task_table(self):
        self.create_table_by_name(DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE)

    def rebuild_hccl_op_table(self):
        self.create_table_by_name(DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE)

    def rebuild_hccl_op_report_table(self):
        self.create_table_by_name(DBNameConstant.TABLE_HCCL_OP_REPORT)

    def get_hccl_task_data(self):
        if not self.attach_to_db(DBNameConstant.DB_ASCEND_TASK):
            logging.error("Attach to db %s failed, task data not found.", DBNameConstant.DB_ASCEND_TASK)
            return []

        if not self.attach_to_db(DBNameConstant.DB_HCCL):
            logging.error("Attach to db %s failed, task data not found.", DBNameConstant.DB_HCCL)
            return []

        device_id = InfoConfReader().get_device_id()
        if device_id == Constant.NA:
            logging.error("No device id found.")
            return []

        sql = (
            "SELECT a.model_id as model_id, a.index_id as index_id, a.name as hccl_name, a.plane_id as plane_id, "  # nosec B608
            "a.timestamp as host_timestamp,a.group_name as group_name, b.start_time as timestamp, "
            "a.is_master as is_master, a.stream_id as stream_id, a.task_id as task_id, "
            "a.duration as duration_estimated, a.local_rank as local_rank, a.remote_rank as remote_rank, "
            "a.transport_type as transport_type, a.size as size, a.data_type as data_type, "
            "a.link_type as link_type, a.thread_id as thread_id, "
            "a.context_id as context_id, a.notify_id as notify_id, a.batch_id as batch_id, "
            "a.rdma_type as rdma_type, b.connection_id as connection_id, b.duration as duration, a.rank_size as rank_size from {0} as a "
            "inner join {1} as b on "
            "a.stream_id = b.stream_id "
            "and a.task_id = b.task_id "
            "and a.batch_id = b.batch_id "
            "and a.context_id = b.context_id "
            "and a.device_id = {device_id} "
            "and b.start_time != {invalid_start} "
            "order by host_timestamp, timestamp".format(
                DBNameConstant.TABLE_HCCL_TASK,
                DBNameConstant.TABLE_ASCEND_TASK,
                device_id=device_id,
                invalid_start=NumberConstant.INVALID_TASK_TIME,
            )
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HcclTask)

    def get_hccl_ops(self, model_id: int, index_id: int):
        device_id = InfoConfReader().get_device_id()
        if device_id == Constant.NA:
            logging.error("No device id found.")
            return []

        where_condition = ""
        if ProfilingScene().is_graph_export():
            where_condition = f'and model_id={model_id} and (index_id={index_id} or index_id=0)'

        sql = (
            "SELECT model_id, index_id, op_name, task_type, op_type, connection_id, begin as timestamp, "  # nosec B608
            "end - begin as duration, is_dynamic, thread_id, "
            "relay, retry, data_type, alg_type, count, group_name, kfc_connection_id from {0} "
            "WHERE device_id = {device_id} "
            "{where_condition} "
            "order by timestamp".format(
                DBNameConstant.TABLE_HCCL_OP, device_id=device_id, where_condition=where_condition
            )
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HcclOps)

    def get_hccl_op_data_by_group(self):
        """
        get the real execution of the communication op
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE):
            return []
        sql = (
            f"select model_id, index_id, op_name, group_name, min(timestamp) as timestamp, "  # nosec B608
            f"max(timestamp + duration) - min(timestamp) as duration, task_type, op_type, connection_id, rank_size "
            f"from {DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE} "
            f"WHERE is_master = 1 "
            f"group by op_name, first_timestamp"
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HcclTask)

    def get_kfc_op_data_by_group(self):
        """
        get the real execution of the kfc op
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_TASK):
            return []
        sql = (
            f"select model_id, index_id, op_name, group_name, min(timestamp) as timestamp, "  # nosec B608
            f"max(timestamp + duration) - min(timestamp) as duration, connection_id "
            f"from {DBNameConstant.TABLE_KFC_TASK} "
            f"WHERE is_master = 1 and source != {DeviceHcclSource.HCCL.value} "
            f"group by op_name, first_timestamp"
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HcclTask)

    def get_hccl_op_info_from_table(self: any, table_name: str = DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE):
        """
        get hccl op info from HCCLOpSingleDevice
        """
        if not DBManager.judge_table_exist(self.cur, table_name):
            return []
        sql = f"select relay, retry, data_type, alg_type, count, group_name, connection_id, rank_size from {table_name}"  # nosec B608
        hccl_op_data = DBManager.fetch_all_data(self.cur, sql, dto_class=HcclOps)
        return {hccl_op.connection_id: hccl_op for hccl_op in hccl_op_data}

    def get_hccl_op_time_section(self: any, table_name: str = DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE):
        if not DBManager.judge_table_exist(self.cur, table_name):
            return []
        sql = (
            f'select min(timestamp) as start_time, max(timestamp + duration) as end_time '  # nosec B608
            f'from {table_name} '
            f"WHERE is_master = 1 "
            f'group by op_name, first_timestamp'
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=CommunicationTimeSection)

    def get_kfc_op(self: any):
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_OP):
            return []
        sql = f'select * from {DBNameConstant.TABLE_KFC_OP} WHERE source != {DeviceHcclSource.HCCL.value}'  # nosec B608
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HcclOps)

    def get_kfc_task(self: any) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_TASK):
            return []
        sql = f'select * from {DBNameConstant.TABLE_KFC_TASK} where source != {DeviceHcclSource.HCCL.value}'  # nosec B608
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HcclTask)

    def get_hccl_task(self: any) -> list:
        hccl_data = self.get_all_data(DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE, dto_class=HcclTask)
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_TASK):
            return hccl_data
        sql = f"select * from {DBNameConstant.TABLE_KFC_TASK} where source = {DeviceHcclSource.HCCL.value}"  # nosec B608
        hccl_data += DBManager.fetch_all_data(self.cur, sql, dto_class=HcclTask)
        return hccl_data

    def create_table_by_name(self, table_name):
        if DBManager.judge_table_exist(self.cur, table_name):
            self.drop_table(table_name)
        table_map = "{0}Map".format(table_name)
        sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
        DBManager.execute_sql(self.conn, sql)
