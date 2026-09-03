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
import os
from dataclasses import dataclass
from typing import Union
from typing import List

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_iteration import MsprofIteration
from common_func.msvp_common import format_high_precision_for_csv
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel
from profiling_bean.db_dto.ge_task_dto import GeTaskDto
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.db_dto.task_time_dto import TaskTimeDto


@dataclass
class AiCpuData:
    stream_id: int = None
    task_id: int = None
    sys_start: int = None
    sys_end: int = None
    node_name: str = None
    compute_time: float = None
    memcpy_time: float = None
    task_time: float = None
    dispatch_time: float = None
    total_time: float = None
    batch_id: int = None


class ParseAiCpuData:
    """
    class for parse aicpu data
    """

    @staticmethod
    def analysis_aicpu(project_path: str, iter_range: IterationRange) -> list:
        """
        parse and analysis AI CPU related dp data
        :return: ai cpu data , headers
        """
        ai_cpu_results = ParseAiCpuData.get_ai_cpu_data(project_path, iter_range)
        ascend_task_results = ParseAiCpuData.get_ascend_task_ai_cpu_data(project_path)
        res = ParseAiCpuData.get_aicpu_batch_id(ai_cpu_results, ascend_task_results)

        ge_results = ParseAiCpuData.get_ge_summary_aicpu_data(project_path)
        res = ParseAiCpuData.match_aicpu_with_ge_summary(res, ge_results)
        res.sort(key=lambda x: x[0])
        return res

    @staticmethod
    def analysis_aicpu_by_sqe_id(project_path: str, iter_range: IterationRange) -> list:
        """
        parse and analysis AI CPU related dp data by chip v6
        :return: ai cpu data , headers
        """
        ai_cpu_results = ParseAiCpuData.get_ai_cpu_data(project_path, iter_range)
        ge_results = ParseAiCpuData.get_ge_summary_aicpu_data(project_path, True)
        res = ParseAiCpuData.match_aicpu_with_ge_summary(ai_cpu_results, ge_results)
        res.sort(key=lambda x: x[0])
        return res

    @staticmethod
    def get_aicpu_batch_id(ai_cpu_data: List[AiCpuData], ascend_task_data: List[TaskTimeDto]) -> List[AiCpuData]:
        """
        get ai cpu batch_id from ascend_task data
        """
        sep_ai_cpu_dict = ParseAiCpuData._sep_task_by_stream_task(ai_cpu_data)
        seq_ascend_task_dict = ParseAiCpuData._sep_task_by_stream_task(ascend_task_data)

        res = []
        # ascend_task's tasks num more than aicpu's tasks num,
        # and csv's data displayed mainly by aicpu data
        stream_task_set = sep_ai_cpu_dict.keys()
        for key in stream_task_set:
            ai_cpu_t = sep_ai_cpu_dict.get(key, [])
            ascend_task_t = seq_ascend_task_dict.get(key, [])
            match_res = ParseAiCpuData._match_aicpu_data_by_task_time(ai_cpu_t, ascend_task_t)
            res.extend([*match_res])
        return res

    @staticmethod
    def match_aicpu_with_ge_summary(ai_cpu_data: List[AiCpuData], ge_summary_data: List[GeTaskDto]) -> list:
        """
        get ai cpu op_name from ge_summary data
        """
        start_ts_float = float(InfoConfReader().get_collect_time()[0])
        ge_summary_dic = {}
        for dto in ge_summary_data:
            ge_summary_dic.setdefault((dto.stream_id, dto.task_id, dto.batch_id), dto.op_name)
        index = 0
        res = [[] for _ in range(len(ai_cpu_data))]
        for i, dto in enumerate(ai_cpu_data):
            sys_start = InfoConfReader().trans_into_local_time(dto.sys_start, use_us=True)
            if start_ts_float > float(sys_start):
                continue
            sys_start = format_high_precision_for_csv(sys_start)
            compute_time = round(dto.compute_time, NumberConstant.ROUND_THREE_DECIMAL)
            memcpy_time = round(dto.memcpy_time, NumberConstant.ROUND_THREE_DECIMAL)
            task_time = round(dto.task_time, NumberConstant.ROUND_THREE_DECIMAL)
            dispatch_time = round(dto.dispatch_time, NumberConstant.ROUND_THREE_DECIMAL)
            total_time = round(dto.total_time, NumberConstant.ROUND_THREE_DECIMAL)
            node_name = ge_summary_dic.get((dto.stream_id, dto.task_id, dto.batch_id), dto.node_name)
            res[index] = [
                sys_start,
                node_name,
                compute_time,
                memcpy_time,
                task_time,
                dispatch_time,
                total_time,
                dto.stream_id,
                dto.task_id,
            ]
            index = index + 1
        return res[:index]

    @staticmethod
    def get_ge_summary_aicpu_data(project_path: str, use_sqe_id: bool = False) -> list:
        """
        get ge_summary data
        """
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_AICORE_OP_SUMMARY)
        ge_summary_conn, ge_summary_curs = DBManager.check_connect_db_path(db_path)
        if not ge_summary_conn or not ge_summary_curs:
            logging.warning("Can't connect ai_core_op_summary.db!")
            return []

        if not use_sqe_id:
            ge_results = ParseAiCpuData._get_ge_summary_aicpu_data(ge_summary_conn)
        else:
            ge_results = ParseAiCpuData._get_ge_summary_data(ge_summary_conn)
        DBManager.destroy_db_connect(ge_summary_conn, ge_summary_curs)
        return ge_results

    @staticmethod
    def get_ascend_task_ai_cpu_data(project_path: str) -> list:
        """
        get ascend_task_data
        """
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_ASCEND_TASK)
        ascend_task_conn, ascend_task_curs = DBManager.check_connect_db_path(db_path)
        if not ascend_task_conn or not ascend_task_curs:
            logging.warning("Can't connect ascend_task.db!")
            return []

        ascend_task_results = ParseAiCpuData._get_ascend_task_aicpu_data(ascend_task_conn)
        DBManager.destroy_db_connect(ascend_task_conn, ascend_task_curs)
        return ascend_task_results

    @staticmethod
    def get_ai_cpu_data(project_path: str, iter_range: IterationRange) -> list:
        """
        get ai cpu data
        """
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_AI_CPU)
        ai_cpu_conn, ai_cpu_curs = DBManager.check_connect_db_path(db_path)
        if not ai_cpu_conn or not ai_cpu_curs:
            logging.warning("Can't connect ai_cpu.db!")
            return []

        ai_cpu_results = ParseAiCpuData._get_aicpu_data(ai_cpu_conn, iter_range, project_path)
        DBManager.destroy_db_connect(ai_cpu_conn, ai_cpu_curs)
        return ai_cpu_results

    @staticmethod
    def get_aicpu_mi_data(project_path: str) -> list:
        """
        get ai cpu mi data
        """
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_CLUSTER_DATA_PREPROCESS)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not conn or not curs:
            logging.warning("Can't connect %s", DBNameConstant.DB_CLUSTER_DATA_PREPROCESS)
            return []
        sql = "select node_name, start_time, end_time, queue_size from {0} order by start_time".format(
            DBNameConstant.TABLE_DATA_QUEUE
        )
        aicpu_mi_data = DBManager.fetch_all_data(conn.cursor(), sql)
        DBManager.destroy_db_connect(conn, curs)
        return aicpu_mi_data

    @staticmethod
    def get_ai_cpu_from_ts(project_path: str) -> list:
        """
        get ai cpu query sql
        """
        aicpu_model = AiCpuModel(project_path)
        if not os.path.exists(PathManager.get_db_path(project_path, DBNameConstant.DB_AI_CPU)):
            logging.info("no aicpu db found")
            return []
        with aicpu_model:
            aicpu_data = aicpu_model.get_ai_cpu_data_from_ts()
        return aicpu_data

    @staticmethod
    def _sep_task_by_stream_task(
        tasks: Union[List[TaskTimeDto], List[AiCpuData]],
    ) -> dict:
        ret = {}
        for task in tasks:
            ret.setdefault((task.stream_id, task.task_id), []).append(task)
        return ret

    @staticmethod
    def _match_aicpu_data_by_task_time(ai_cpu_data: List[AiCpuData], ascend_task_data: List[TaskTimeDto]) -> List:
        aicpu_index = 0
        task_index = 0
        # 不强制校验aicpu和ascendTask的aicpu算子数量。aicpu执行时间必定包含于stars调度时间，以此判定aicpu合法性
        # 无法匹配的aicpu batch_id不做刷新，默认为None，后续默认赋NA
        while aicpu_index < len(ai_cpu_data) and task_index < len(ascend_task_data):
            aicpu = ai_cpu_data[aicpu_index]
            task = ascend_task_data[task_index]
            if task.start_time <= aicpu.sys_end * NumberConstant.NS_TO_US <= task.end_time:
                ai_cpu_data[aicpu_index] = aicpu.replace(batch_id=task.batch_id)
                aicpu_index += 1
            elif aicpu.sys_end * NumberConstant.NS_TO_US < task.start_time:
                aicpu_index += 1
            elif aicpu.sys_end * NumberConstant.NS_TO_US > task.end_time:
                task_index += 1

        return ai_cpu_data

    @staticmethod
    def _get_ge_summary_data(ge_summary_conn):
        sql = "select op_name, stream_id, task_id from {0} ".format(DBNameConstant.TABLE_SUMMARY_GE)
        return DBManager.fetch_all_data(ge_summary_conn.cursor(), sql, dto_class=GeTaskDto)

    @staticmethod
    def _get_ge_summary_aicpu_data(ge_summary_conn):
        sql = "select op_name, stream_id, task_id, batch_id from {0} where task_type = '{1}' order by timestamp".format(
            DBNameConstant.TABLE_SUMMARY_GE, Constant.TASK_TYPE_AI_CPU
        )
        return DBManager.fetch_all_data(ge_summary_conn.cursor(), sql, dto_class=GeTaskDto)

    @staticmethod
    def _get_ascend_task_aicpu_data(ascend_task_conn):
        sql = (
            "select batch_id, stream_id, task_id, start_time, start_time + duration AS end_time "
            "from {0} where host_task_type = 'KERNEL_AICPU' "
            "order by start_time".format(DBNameConstant.TABLE_ASCEND_TASK)
        )
        return DBManager.fetch_all_data(ascend_task_conn.cursor(), sql, dto_class=TaskTimeDto)

    @staticmethod
    def _get_aicpu_data(ai_cpu_conn, iter_range, project_path):
        where_condition = ""
        if not ProfilingScene().is_all_export():
            iter_time = MsprofIteration(project_path).get_iter_interval(iter_range)
            if iter_time:
                # AiCpuData 落盘为 ns，iter 区间同样是 ns
                where_condition = "where sys_start>={0} and sys_end<={1}".format(iter_time[0], iter_time[1])
        # DB 为 ns，导出 CSV 为 us
        sql = (
            "select sys_start/{NS_TO_US} as sys_start, sys_end/{NS_TO_US} as sys_end, "
            "'{node_name}' as node_name,"
            "compute_time/{NS_TO_US} as compute_time, memcpy_time/{NS_TO_US} as memcpy_time,"
            "task_time/{NS_TO_US} as task_time,dispatch_time/{NS_TO_US} as dispatch_time,"
            "total_time/{NS_TO_US} as total_time, stream_id, task_id from {0} {where_condition} "
            "order by sys_start".format(
                DBNameConstant.TABLE_AI_CPU,
                NS_TO_US=NumberConstant.NS_TO_US,
                node_name=Constant.NA,
                where_condition=where_condition,
            )
        )
        return DBManager.fetch_all_data(ai_cpu_conn.cursor(), sql, dto_class=AiCpuData)
