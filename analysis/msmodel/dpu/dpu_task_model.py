# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
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
from profiling_bean.db_dto.dpu_track_dto import DPUTrackDto


class DPUTaskModel(ParserModel):
    """
    class used to operate all dpu track
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(
            result_dir,
            DBNameConstant.DB_DPU,
            [DBNameConstant.TABLE_DPU_TASK_TRACK, DBNameConstant.TABLE_DPU_HCCL_TRACK],
        )

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_DPU_TASK_TRACK) -> None:
        """
        flush to db
        :param data_list: data
        :param table_name: table name
        :return:
        """
        self.insert_data_to_db(table_name, data_list)


class DPUTaskViewModel(ViewModel):
    """
    class for dpu task viewer
    """

    def get_timeline_data(self: any) -> tuple:
        task_track = []
        hccl_track = []
        if DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_DPU_TASK_TRACK):
            sql = (
                "select dpu_device_id, thread_id, start_time, end_time, task_type, stream_id, task_id, "
                "kernel_name AS op_name "
                "from {} ".format(DBNameConstant.TABLE_DPU_TASK_TRACK)  # nosec B608
            )
            task_track = DBManager.fetch_all_data(self.cur, sql, dto_class=DPUTrackDto)

        if DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_DPU_HCCL_TRACK):
            sql = (
                "select npu_device_id, dpu_device_id, thread_id, start_time, end_time, op_name, group_name, "
                "group_name_id, local_rank, remote_rank, rank_size, duration_estimated, src_addr, dst_addr, "
                "data_size,  stream_id, task_id, aicpu_task_id, plane_id, op_type, data_type, link_type, "
                "transport_type, rdma_type, role, ccl_tag, notify_id, work_flow_mode, stage "
                "from {} ".format(DBNameConstant.TABLE_DPU_HCCL_TRACK)  # nosec B608
            )
            hccl_track = DBManager.fetch_all_data(self.cur, sql, dto_class=DPUTrackDto)
        return task_track, hccl_track
