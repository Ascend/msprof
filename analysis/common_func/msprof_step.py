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

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.empty_class import EmptyClass
from common_func.utils import Utils
from msmodel.interface.base_model import BaseModel
from profiling_bean.db_dto.step_trace_dto import StepTraceDto


class MsprofStep:
    """
    mainly process iteration
    """

    def __init__(self: any, result_dir: str) -> None:
        self._result_dir = result_dir
        self.data = []
        self.model = BaseModel(self._result_dir, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA])

    def __enter__(self):
        if not self.model.check_table():
            return self
        self.get_step_data()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.model.finalize()

    def get_step_data(self: any) -> None:
        """
        get all data from table step trace data
        :return:
        """
        self.model.init()
        all_data_sql = "select * from {}".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        self.data = DBManager.fetch_all_data(self.model.cur, all_data_sql, dto_class=StepTraceDto)

    def get_step_iteration_time(self: any, index_id: int, model_id: int) -> list:
        """
        get iteration time by model id and index id
        :param index_id:
        :param model_id:
        :return:
        """
        step_end_min = None
        step_end_max = None
        iter_id_list = self.get_iter_id(index_id, model_id)
        if not iter_id_list:
            return []
        for data in self.data:
            if data.iter_id == iter_id_list[0]:
                step_end_min = data.step_end
            if data.iter_id == iter_id_list[1]:
                step_end_max = data.step_end
        if not step_end_min or not step_end_max:
            return []
        return [step_end_min, step_end_max]

    def get_mix_op_iter_id(self: any, index_id: int, model_id: int) -> tuple:
        """
        get op single iter id in single op and graph mix scene
        :param index_id:
        :param model_id:
        :return: [min_iter_id - 1, max_iter_id]
        """
        iter_data = None
        iter_id_min = Constant.DEFAULT_COUNT
        iter_id_max = max(data.iter_id for data in self.data)
        for data in self.data:
            if data.model_id == model_id and data.index_id == index_id:
                iter_data = data
                break
        if iter_data is None:
            return ()
        for data in self.data:
            if data.model_id == Constant.GE_OP_MODEL_ID and data.step_start <= iter_data.step_start <= data.step_end:
                iter_id_min = data.iter_id - 1
            if data.model_id == Constant.GE_OP_MODEL_ID and iter_data.step_start < data.step_start:
                iter_id_max = data.iter_id - 1
                break
        return iter_id_min, iter_id_max

    def get_graph_iter_id(self: any, index_id: int, model_id: int) -> tuple:
        """
        get graph iter id in single op scene or graph scene
        :param index_id:
        :param model_id:
        :return:
        """
        iter_id = None
        for data in self.data:
            if data.model_id == model_id and data.index_id == index_id:
                iter_id = data.iter_id
                break
        if not iter_id:
            return ()
        return iter_id - 1, iter_id

    def get_iter_id(self: any, index_id: int, model_id: int) -> tuple:
        """
        get iter id by model id and index id
        :param index_id:
        :param model_id:
        :return:
        """
        if Utils.need_all_model_in_one_iter(self._result_dir, model_id):
            return self.get_mix_op_iter_id(index_id, model_id)
        return self.get_graph_iter_id(index_id, model_id)

    def get_model_and_index_id_by_iter_id(self: any, iter_id: int) -> tuple:
        """
        get model id and index id by iter id
        :param iter_id:
        :return:
        """
        for data in self.data:
            if data.iter_id == iter_id:
                return data.model_id, data.index_id
        return EmptyClass(), EmptyClass()
