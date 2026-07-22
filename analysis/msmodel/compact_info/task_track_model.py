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
from profiling_bean.db_dto.task_track_dto import TaskTrackV2Dto


class TaskTrackModel(ParserModel):
    """
    db operator for task track
    """

    def __init__(self: any, result_dir: str, table_list: list):
        super().__init__(result_dir, DBNameConstant.DB_RTS_TRACK, table_list)

    def flush(self: any, data_list: list) -> None:
        """
        insert data into database
        """
        if not self.table_list:
            return
        self.insert_data_to_db(self.table_list[0], data_list)


class TaskTraceV2ViewModel(ViewModel):
    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_RTS_TRACK, [])

    def get_model_id_info_data(self):
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_MODEL_INFO):
            return {}

        sql = ("SELECT device_id, stream_id, task_id, batch_id, model_id FROM {}").format(
            DBNameConstant.TABLE_MODEL_INFO
        )
        model_id_info_data = DBManager.fetch_all_data(self.cur, sql, dto_class=TaskTrackV2Dto)
        return {
            (item.device_id, item.stream_id, item.task_id, item.batch_id): item.model_id for item in model_id_info_data
        }
