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
from collections import namedtuple
from dataclasses import dataclass

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_object import CustomizedNamedtupleFactory
from common_func.db_manager import DBManager
from mscalculate.flip.flip_calculator import FlipCalculator
from mscalculate.hccl.hccl_task import HcclTask, KfcOps
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel


class KfcInfoModel(ParserModel):
    """
    kfc info model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_KFC_INFO, table_list)

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_KFC_INFO) -> None:
        """
        insert data to table
        :param data_list: hccl information data
        :param table_name: table name
        :return:
        """
        self.insert_data_to_db(table_name, data_list)


class KfcInfoViewModel(ViewModel):
    KFC_HCCL_INFO_TYPE = CustomizedNamedtupleFactory.enhance_namedtuple(
        namedtuple(
            "KfcInfo",
            [
                "timestamp",
                "op_name",
                "ccl_tag",
                "group_name",
                "local_rank",
                "remote_rank",
                "rank_size",
                "work_flow_mode",
                "plane_id",
                "context_id",
                "notify_id",
                "stage",
                "role",
                "duration_estimated",
                "src_addr",
                "dst_addr",
                "size",
                "op_type",
                "data_type",
                "link_type",
                "transport_type",
                "rdma_type",
                "stream_id",
                "task_id",
                "batch_id",
                "start_time",
                "duration",
                "bandwidth",
                "device_task_type",
                "ts_virtual_batch_id",
            ],
        ),
        {},
    )
    KFC_COMM_TURN_TYPE = CustomizedNamedtupleFactory.enhance_namedtuple(
        namedtuple(
            "KfcCommTurn",
            [
                "device_id",
                "stream_id",
                "task_id",
                "comm_turn",
                "current_turn",
                "server_start_time",
                "wait_msg_start_time",
                "kfc_alg_exe_start_time",
                "send_task_start_time",
                "send_sqe_finish_time",
                "rtsq_exe_end_time",
                "server_end_time",
            ],
        ),
        {},
    )
    KFC_COMPUTE_TURN_TYPE = CustomizedNamedtupleFactory.enhance_namedtuple(
        namedtuple(
            "KfcComputeTurn",
            [
                "device_id",
                "stream_id",
                "task_id",
                "compute_turn",
                "current_turn",
                "wait_compute_start_time",
                "compute_start_time",
                "compute_exe_end_time",
            ],
        ),
        {},
    )

    HCCL_OP_MASTER_STREAM_TYPE = CustomizedNamedtupleFactory.enhance_namedtuple(
        namedtuple(
            "HcclOpMasterStreamType",
            [
                "timestamp",
                "stream_id",
                "task_id",
                "hccl_stream_id",
                "hccl_task_id",
                "batch_id",
                "hccl_batch_id",
                "task_type",
            ],
        ),
        {},
    )

    MASTER_STREAM_HCCL_TASK_TYPE = CustomizedNamedtupleFactory.enhance_namedtuple(
        namedtuple(
            "MasterStreamHcclTaskType",
            [
                "timestamp",
                "aicpu_stream_id",
                "aicpu_task_id",
                "stream_id",
                "task_id",
                "aicpu_batch_id",
                "batch_id",
                "task_type",
            ],
        ),
        {},
    )

    def __init__(self, result_dir: str, table_list: list):
        super().__init__(result_dir, DBNameConstant.DB_KFC_INFO, table_list)

    def get_kfc_info_data(self: any) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_INFO):
            return []
        sql = (
            "select timestamp, op_name, ccl_tag, group_name, local_rank, remote_rank, rank_size, work_flow_mode, "
            "plane_id, context_id, notify_id, stage, role, duration_estimated, src_addr, dst_addr, size, op_type, "
            "data_type, link_type, transport_type, rdma_type, stream_id, task_id, "
            "0 as batch_id, 0 as start_time, 0 as duration, 0 as bandwidth, '{NA}' as device_task_type, "
            "-1 as ts_virtual_batch_id from {}".format(DBNameConstant.TABLE_KFC_INFO, NA=Constant.NA)
        )
        kfc_info_data = self.get_sql_data(sql)
        return [self.KFC_HCCL_INFO_TYPE(*data) for data in kfc_info_data]

    def get_kfc_info_with_task(self: any) -> list:
        """KFC_INFO JOIN ASCEND_TASK，通过四元组(stream_id, task_id, context_id, batch_id)关联，直接返回HcclTask列表"""
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_INFO):
            return []
        if not self.attach_to_db(DBNameConstant.DB_ASCEND_TASK):
            logging.error("Attach to db %s failed, task data not found.", DBNameConstant.DB_ASCEND_TASK)
            return []

        sql = (
            "SELECT a.op_name as hccl_name, a.group_name, a.local_rank, a.remote_rank, "  # nosec B608
            "a.rank_size, a.plane_id, a.context_id, a.notify_id, "
            "a.size, a.op_type, a.data_type, a.link_type, a.transport_type, a.rdma_type, "
            "a.stream_id, a.task_id, a.batch_id, "
            "b.start_time as timestamp, b.duration as duration, 0 as bandwidth "
            "from {0} as a "
            "inner join {1} as b on "
            "a.stream_id = b.stream_id "
            "and a.task_id = b.task_id "
            "and a.batch_id = b.batch_id "
            "and a.context_id = b.context_id "
            "and b.start_time != {invalid_start} "
            "order by b.start_time".format(
                DBNameConstant.TABLE_KFC_INFO,
                DBNameConstant.TABLE_ASCEND_TASK,
                invalid_start=NumberConstant.INVALID_TASK_TIME,
            )
        )
        return self.get_sql_data(sql, dto_class=HcclTask)

    def get_kfc_info_with_task_by_stream_ids(self: any, stream_ids: tuple) -> list:
        """KFC_INFO JOIN ASCEND_TASK，按stream_id过滤"""
        if not stream_ids:
            return []
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_INFO):
            return []
        if not self.attach_to_db(DBNameConstant.DB_ASCEND_TASK):
            logging.error("Attach to db %s failed, task data not found.", DBNameConstant.DB_ASCEND_TASK)
            return []

        stream_condition = "b.stream_id in ({})".format(",".join(map(str, stream_ids)))
        sql = (
            "SELECT a.op_name as hccl_name, a.local_rank, a.remote_rank, "  # nosec B608
            "a.rank_size, a.plane_id, a.context_id, a.notify_id, "
            "a.size, a.op_type, a.data_type, a.link_type, a.transport_type, a.rdma_type, "
            "a.stream_id, a.task_id, a.batch_id, "
            "b.start_time as timestamp, b.duration as duration "
            "from {0} as a "
            "inner join {1} as b on "
            "a.stream_id = b.stream_id "
            "and a.task_id = b.task_id "
            "and a.batch_id = b.batch_id "
            "and a.context_id = b.context_id "
            "and b.start_time != {invalid_start} "
            "and {stream_condition} "
            "order by b.start_time".format(
                DBNameConstant.TABLE_KFC_INFO,
                DBNameConstant.TABLE_ASCEND_TASK,
                invalid_start=NumberConstant.INVALID_TASK_TIME,
                stream_condition=stream_condition,
            )
        )
        return self.get_sql_data(sql, dto_class=HcclTask)

    def get_ascend_task_with_kfc_defaults(self: any, stream_ids: tuple) -> list:
        """仅查ASCEND_TASK，KFC字段用默认值补齐，返回与get_kfc_info_with_task一致的HcclTask结构"""
        if not stream_ids:
            return []
        if not self.attach_to_db(DBNameConstant.DB_ASCEND_TASK):
            logging.error("Attach to db %s failed, task data not found.", DBNameConstant.DB_ASCEND_TASK)
            return []

        stream_condition = "stream_id in ({})".format(",".join(map(str, stream_ids)))
        sql = (
            "SELECT '{NA}' as op_name, "  # nosec B608
            "-1 as local_rank, -1 as remote_rank, -1 as rank_size, "
            "0 as plane_id, context_id, '{NA}' as notify_id, "
            "-1 as size, '{NA}' as op_type, '{NA}' as data_type, "
            "'{NA}' as link_type, '{NA}' as transport_type, '{NA}' as rdma_type, "
            "stream_id, task_id, batch_id, "
            "start_time as timestamp, duration, 0 as bandwidth "
            "from {0} where {stream_condition}".format(
                DBNameConstant.TABLE_ASCEND_TASK, stream_condition=stream_condition, NA=Constant.NA
            )
        )
        return self.get_sql_data(sql, dto_class=HcclTask)

    def get_kfc_op_data(self):
        if not self.attach_to_db(DBNameConstant.DB_ASCEND_TASK):
            logging.error("Attach to db %s failed, task data not found.", DBNameConstant.DB_ASCEND_TASK)
            return []

        if not self.attach_to_db(DBNameConstant.DB_GE_INFO):
            logging.error("Attach to db %s failed, task data not found.", DBNameConstant.DB_GE_INFO)
            return []

        device_id = InfoConfReader().get_device_id()
        if device_id == Constant.NA:
            logging.error("No device id found.")
            return []

        sql = (
            "SELECT b.model_id as model_id, b.index_id as index_id, b.stream_id as stream_id, b.task_id as task_id, "  # nosec B608
            "b.context_id as context_id, b.batch_id as batch_id, "
            "b.start_time as start, b.start_time + b.duration as end, "
            "b.connection_id as kfc_connection_id, a.op_name as op_name "
            "from {0} as a inner join {1} as b "
            "on a.stream_id = b.stream_id "
            "and a.task_id = b.task_id "
            "and a.batch_id = b.batch_id "
            "and a.context_id = b.context_id "
            "and a.device_id = {device_id} "
            "and b.start_time != {invalid_start} "
            "and (a.op_name like '%{pattern}') "
            "order by b.start_time".format(
                DBNameConstant.TABLE_GE_TASK,
                DBNameConstant.TABLE_ASCEND_TASK,
                invalid_start=NumberConstant.INVALID_TASK_TIME,
                device_id=device_id,
                pattern=StrConstant.AICPU_KERNEL,
            )
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=KfcOps)

    def get_kfc_comm_turn_data(self: any) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_COMM_TURN):
            return []
        sql = "select * from {}".format(DBNameConstant.TABLE_KFC_COMM_TURN)
        kfc_info_data = self.get_sql_data(sql)
        return [self.KFC_COMM_TURN_TYPE(*data) for data in kfc_info_data]

    def get_kfc_compute_turn_data(self: any) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_COMPUTE_TURN):
            return []
        sql = "select * from {}".format(DBNameConstant.TABLE_KFC_COMPUTE_TURN)
        kfc_info_data = self.get_sql_data(sql)
        return [self.KFC_COMPUTE_TURN_TYPE(*data) for data in kfc_info_data]

    def get_aicpu_master_stream_hccl_task(self: any) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_AICPU_MASTER_STREAM_HCCL_TASK):
            return []
        sql = (
            "select timestamp, aicpu_stream_id, aicpu_task_id, stream_id, task_id, "
            "0 as aicpu_batch_id, batch_id, type "
            "from {} order by timestamp".format(DBNameConstant.TABLE_AICPU_MASTER_STREAM_HCCL_TASK)
        )
        aicpu_master_stream_hccl_task = self.get_sql_data(sql)
        aicpu_master_stream_hccl_task = [
            self.HCCL_OP_MASTER_STREAM_TYPE(*data) for data in aicpu_master_stream_hccl_task
        ]
        aicpu_master_stream_hccl_task = FlipCalculator.set_device_batch_id(
            aicpu_master_stream_hccl_task, self.result_dir
        )
        return [self.MASTER_STREAM_HCCL_TASK_TYPE(*data) for data in aicpu_master_stream_hccl_task]

    def get_kfc_info_dict(self: any) -> dict:
        kfc_info = self.get_kfc_info_data()
        return {item.task_id: item.stream_id for item in kfc_info}


@dataclass
class KfcTurnData:
    op_name: str
    stream_id: int
    task_id: int
    start_time: str
    duration: float
