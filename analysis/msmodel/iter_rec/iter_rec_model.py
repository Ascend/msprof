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
from common_func.ms_constant.number_constant import NumberConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.db_dto.time_section_dto import TimeSectionDto


class HwtsIterModel(ParserModel):
    """
    class used to operate hwts iter db
    """

    AI_CORE_TYPE = 'ai_core'
    TASK_TYPE = "task"

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(
            result_dir,
            DBNameConstant.DB_HWTS_REC,
            [DBNameConstant.TABLE_HWTS_ITER_SYS, DBNameConstant.TABLE_HWTS_BATCH],
        )

    def flush(self: any, data_list: list, table_name: str) -> None:
        """
        flush data to db
        :param data_list:
        :return:
        """
        self.insert_data_to_db(table_name, data_list)

    def clear_table(self: any):
        """
        clear tables
        """
        for table in self.table_list:
            DBManager.clear_table(self.conn, table)

    def get_task_offset_and_sum(self: any, iteration: IterationRange, data_type: str) -> (int, int):
        """
        Get the number of hwts tasks in all previous iterations and the number of tasks in this round of iteration
        :param data_type:
        :param iteration:
        :return: offset_count is the number of tasks in all previous iterations
        sum_count is the number of tasks in this round of iteration
        """
        offset_count_dict = {
            self.AI_CORE_TYPE: f"select min(ai_core_offset) as ai_core_offset, "
            f"max(ai_core_offset + ai_core_num) - min(ai_core_offset) as ai_core_num "
            f"from {DBNameConstant.TABLE_HWTS_ITER_SYS} "
            f"where model_id=? and index_id>=? and index_id<=?",
            self.TASK_TYPE: f"select min(task_offset) as task_offset, "
            f"max(task_offset + task_count) - min(task_offset) as task_count "
            f"from {DBNameConstant.TABLE_HWTS_ITER_SYS} "
            f"where model_id=? and index_id>=? and index_id<=?",
        }

        return self._get_task_num(iteration, offset_count_dict.get(data_type))

    def check_table(self: any, table_name=DBNameConstant.TABLE_HWTS_ITER_SYS) -> bool:
        """
        check whether the table exists.
        :return: exits or not
        """
        if not self.conn or not self.cur or not DBManager.judge_table_exist(self.cur, table_name):
            return False
        return True

    def check_iter_data_in_db(self, table_name: str):
        conn, cur = DBManager.check_connect_db(self.result_dir, self.db_name)
        if not all(
            [conn, cur, DBManager.judge_table_exist(cur, table_name), DBManager.judge_row_exist(cur, table_name)]
        ):
            return False
        return True

    def get_batch_list(self: any, table_name, iter_range: list) -> list:
        """
        get batch list from hwts batch table
        :return: batch list
        """
        sql = "select batch_id from {0} where iter_id>=? and iter_id<=? order by end_time".format(table_name)
        return DBManager.fetch_all_data(self.cur, sql, iter_range)

    def _get_task_num(self: any, iteration: IterationRange, sql: str) -> (int, int):
        task_num = DBManager.fetchone(self.cur, sql, (iteration.model_id, *iteration.get_iteration_range()))
        if task_num and len(task_num) == 2:
            return task_num
        return (0, 0)


class HwtsIterViewModel(ViewModel):
    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_HWTS_REC, [])

    def get_ai_core_op_data(self: any) -> list:
        sql = "select start_time, end_time from {} where is_ai_core=1 and start_time<>{}".format(
            DBNameConstant.TABLE_HWTS_BATCH, NumberConstant.INVALID_OP_EXE_TIME
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=TimeSectionDto)
