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

import logging
import os
from typing import List
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.utils import Utils
from common_func.profiling_scene import ProfilingScene
from msmodel.interface.base_model import BaseModel
from msmodel.step_trace.ts_track_model import TsTrackModel
from profiling_bean.db_dto.ge_task_dto import GeTaskDto


class GeInfoModel(BaseModel):
    """
    class used to operate ge info db
    """

    MODEL_INDEX_KEY_FMT = "{0}-{1}"
    STREAM_TASK_KEY_FMT = "{0}-{1}"
    STREAM_TASK_BATCH_KEY_FMT = "{0}-{1}-{2}"

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_GE_INFO, [DBNameConstant.TABLE_GE_TASK])

    def check_table(self: any, table_name=DBNameConstant.TABLE_GE_TASK) -> bool:
        """
        check table of ge task
        :return:
        """
        if not self.conn or not self.cur or not DBManager.judge_table_exist(self.cur, table_name):
            logging.warning(
                "No ge data starting with framework is found, please check the result_dir directory: %s",
                os.path.join(os.path.basename(self.result_dir), 'data'),
            )
            return False
        return True

    def map_model_to_iter(self: any) -> dict:
        """
        map (model_id, index_id) to iter_id
        :return: model_to_iter_dict
        """
        model_to_iter_dict = {}
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.create_connect_db(db_path)
        sql = "select model_id, index_id, iter_id from {0}".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        map_data = DBManager.fetch_all_data(trace_curs, sql)
        DBManager.destroy_db_connect(trace_conn, trace_curs)

        if not map_data:
            return model_to_iter_dict
        for map_datum in map_data:
            model_to_iter_dict.setdefault((map_datum[0], map_datum[1]), map_datum[2])
        return model_to_iter_dict

    def get_batch_dict(self: any) -> dict:
        """
        get batch data
        :return: dict of iter id, stream id, task_id, batch id
        """
        ge_sql = (
            "select model_id, index_id, stream_id, task_id, batch_id "
            "from {0} inner join( "
            "select min(timestamp) as timestamp "
            "from {0} where index_id != 0 and (task_type = '{1}' or task_type = '{2}') "
            "group by model_id, index_id, stream_id) as min_time_table "
            "on {0}.timestamp = min_time_table.timestamp".format(
                DBNameConstant.TABLE_GE_TASK, Constant.TASK_TYPE_AI_CORE, Constant.TASK_TYPE_COMMUNICATION
            )
        )
        ge_data = DBManager.fetch_all_data(self.cur, ge_sql)

        if Utils.is_single_op_scene(self.result_dir):
            return {
                (NumberConstant.INVALID_ITER_ID, stream_id): (task_id, batch_id)
                for _, _, stream_id, task_id, batch_id in ge_data
            }
        else:
            model_to_iter_dict = self.map_model_to_iter()

            batch_dict = {}
            for model_id, index_id, stream_id, task_id, batch_id in ge_data:
                if (model_id, index_id) in model_to_iter_dict:
                    iter_id = model_to_iter_dict.get((model_id, index_id))
                    batch_dict.setdefault((iter_id, stream_id), (task_id, batch_id))
            return batch_dict

    def get_ge_data(self: any, is_static_shape: str) -> any:
        result_data = [{}, {}] if is_static_shape == Constant.GE_STATIC_SHAPE else {}
        if not Utils.is_step_scene(self.result_dir):
            return result_data
        step_trace_data = self.get_step_trace_data()
        task_data = self.get_ge_task_data(is_static_shape)
        if not step_trace_data or not task_data:
            return result_data

        if is_static_shape == Constant.GE_STATIC_SHAPE:
            iter_model_dict = {}
            for step_trace in step_trace_data:
                if step_trace.model_id in task_data:
                    iter_model_dict[step_trace.iter_id] = step_trace.model_id
            result_data[0] = iter_model_dict
            result_data[1] = task_data
        else:
            for step_trace in step_trace_data:
                model_index = self.MODEL_INDEX_KEY_FMT.format(step_trace.model_id, step_trace.index_id)
                if model_index in task_data:
                    result_data[step_trace.iter_id] = task_data.pop(model_index)
        return result_data

    def get_step_trace_data(self: any) -> list:
        ts_model = TsTrackModel(self.result_dir, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA])
        if not ts_model.check_table():
            return []

        with ts_model:
            step_trace_data = ts_model.get_step_trace_data(DBNameConstant.TABLE_STEP_TRACE_DATA)
        return step_trace_data

    def get_ge_task_data(self: any, is_static_shape: str) -> dict:
        # ge task ai core data contains AI_CORE and HCCL type
        if is_static_shape == Constant.GE_STATIC_SHAPE:
            sql = (
                "select model_id, GROUP_CONCAT(stream_id||'-'||task_id||'-'||batch_id) from {0} "
                "where index_id=0 and (task_type = '{1}' or task_type = '{2}') "
                "group by model_id".format(
                    DBNameConstant.TABLE_GE_TASK, Constant.TASK_TYPE_AI_CORE, Constant.TASK_TYPE_COMMUNICATION
                )
            )
        else:
            sql = (
                "select model_id||'-'||index_id, "
                "GROUP_CONCAT(stream_id||'-'||task_id||'-'||batch_id) from {0} "
                "where index_id<>0 and (task_type = '{1}' or task_type = '{2}') "
                "group by model_id||'-'||index_id".format(
                    DBNameConstant.TABLE_GE_TASK, Constant.TASK_TYPE_AI_CORE, Constant.TASK_TYPE_COMMUNICATION
                )
            )
        if ProfilingScene().is_step_export():
            if is_static_shape == Constant.GE_STATIC_SHAPE:
                sql = (
                    "select {model_id}, GROUP_CONCAT(stream_id||'-'||task_id||'-'||batch_id) from {0} "
                    "where index_id=0 and (task_type = '{1}' or task_type = '{2}') "
                    "group by {model_id}".format(
                        DBNameConstant.TABLE_GE_TASK,
                        Constant.TASK_TYPE_AI_CORE,
                        Constant.TASK_TYPE_COMMUNICATION,
                        model_id=NumberConstant.INVALID_MODEL_ID,
                    )
                )
            else:
                sql = (
                    "select {model_id}||'-'||index_id, "
                    "GROUP_CONCAT(stream_id||'-'||task_id||'-'||batch_id) from {0} "
                    "where index_id<>0 and (task_type = '{1}' or task_type = '{2}') "
                    "group by {model_id}||'-'||index_id".format(
                        DBNameConstant.TABLE_GE_TASK,
                        Constant.TASK_TYPE_AI_CORE,
                        Constant.TASK_TYPE_COMMUNICATION,
                        model_id=NumberConstant.INVALID_MODEL_ID,
                    )
                )
        task_data = DBManager.fetch_all_data(self.cur, sql)
        task_data_dict = {}
        if task_data:
            for task in task_data:
                task_data_dict[task[0]] = set(task[1].split(','))
        return task_data_dict

    def get_task_info(self, device_ids: List[int]) -> list:
        if not self.check_table():
            return []
        sql = f"SELECT op_name, task_type, stream_id, task_id, batch_id, context_id from {DBNameConstant.TABLE_GE_TASK}"
        if device_ids:
            sql += f" WHERE device_id in ({','.join(map(str, device_ids))})"
        task_info = DBManager.fetch_all_data(self.cur, sql, dto_class=GeTaskDto)
        return task_info
