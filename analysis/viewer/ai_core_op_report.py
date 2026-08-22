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
from collections import deque

from common_func.constant import Constant
from common_func.data_manager import DataManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import add_aicore_units, float_calculate
from common_func.msvp_common import format_high_precision_for_csv
from common_func.msvp_common import read_cpu_cfg
from common_func.msvp_constant import MsvpConstant
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from common_func.utils import Utils
from msmodel.hccl.hccl_model import HcclViewModel
from viewer.ge_info_report import get_ge_hash_dict
from viewer.ge_info_report import get_ge_model_name_dict
from viewer.chip_model_function.chip_model_decorators import format_pmu_data_by_headers


class AiCoreOpReport:
    """
    report ai core op data
    """

    AI_CORE_UNUSED_COLS = [
        "job_id",
        "host_id",
        "device_id",
        "task_id",
        "stream_id",
        "index_id",
        "model_id",
        "overflow",
        "overflowed_cycles",
        "device_id",
        "batch_id",
        "task_type",
        "core_type",
        "subtask_id",
        "start_time",
        "end_time",
        "ffts_type",
    ]
    UNSUPPORTED_HEADER = {
        "aic_vec_ratio",
        "aic_vec_time",
        "aiv_mac_ratio",
        "aiv_mac_time",
        "aiv_mte1_ratio",
        "aic_ub_read_bw",
        "aiv_mte1_time",
        "aic_ub_write_bw",
        "aiv_l1_read_bw",
        "aiv_l1_write_bw",
        "aic_l0c_read_bw",
        "aic_l0c_write_bw",
        "aiv_l0a_read_bw",
        "aiv_l0a_write_bw",
        "aiv_l0b_read_bw",
        "aiv_l0b_write_bw",
        "aiv_l0c_read_bw_cube",
        "aiv_l0c_write_bw_cube",
        "aic_ub_read_bw_mte",
        "aic_ub_write_bw_mte",
        "aic_ub_read_bw_vector",
        "aic_ub_write_bw_vector",
        "aiv_mac_fp16_ratio",
        'aiv_mac_int8_ratio',
        "aic_vec_fp32_ratio",
        "aic_vec_fp16_ratio",
        "aic_vec_int32_ratio",
        "aic_vec_misc_ratio",
        "aic_vec_fp16_128lane_ratio",
        "aic_vec_fp16_64lane_ratio",
        "aic_vec_bankgroup_cflt_ratio",
        "aic_vec_bank_cflt_ratio",
        "aic_vec_resc_cflt_ratio",
        "aic_vector_fops",
        "aiv_cube_fops",
        "aiv_fixpipe_ratio",
        "aiv_fixpipe_time",
    }
    ADDITION_HEADER = ["Context ID", "Mix Block Num", "aiv_time", "aiv_total_time"]
    TENSOR_HEADERS = [
        "Input Shapes",
        "Input Data Types",
        "Input Formats",
        "Output Shapes",
        "Output Data Types",
        "Output Formats",
    ]
    SPECIAL_AI_CORE_HEAD = ("_extra",)

    OPERATOR_UNUSED_HEADERS = ["Model Name", "Infer ID"]
    HEADERS_WITH_NO_GE_DATA = ["Block Num", "Mix Block Num", "HF32 Eligible"]
    HARDWARE_OP_LIST = ['AI_CPU', 'DSA', 'DVPP']
    TASK_START_TIME = 'Task Start Time(us)'
    TASK_DURATION = 'Task Duration(us)'
    TASK_WAIT_TIME = 'Task Wait Time(us)'
    MODEL_NAME_INDEX = 0
    INFER_ID_INDEX = 4
    AI_CPU_TABLE = "ai_cpu_datas"
    IS_PMU_UNIQUE_ID = True

    @staticmethod
    def _union_task_ge_ai_core_data(data: list, ai_core_group_dict: dict) -> list:
        union_data = []
        if not ai_core_group_dict or not data:
            # 没有pmu数据, 去除HCCL小算子
            return AiCoreOpReport._filter_hccl_op(data)
        ai_core_data_len = len(ai_core_group_dict.get(next(iter(ai_core_group_dict)))[0])
        # 全导和按step导，task type的索引是6; 按子图导，task type的索引是7
        task_type_idx, op_name_idx = (7, 4) if ProfilingScene().is_graph_export() else (6, 3)
        for datum in data:
            if datum[task_type_idx] == Constant.TASK_TYPE_HCCL_AI_CPU:
                # 针对helper场景, 去除运行在AI_CPU的HCCL小算子,
                logging.info("Found ai cpu hccl small op of stream %d, task %d", datum[2], datum[1])
                continue
            #  1-task_id, 2-stream_id, -2-context_id, -1-batch_id, (batch_id, task_id, stream_id, context_id)作为key
            key = (datum[-1],) + datum[1:3] + (datum[-2],)
            if not AiCoreOpReport.IS_PMU_UNIQUE_ID:
                # 不支持唯一id, (task_id, stream_id, context_id)作为key
                key = key[1:]
            datum = datum[:-1]  # 去除batch_id, 不在交付件中展现
            ai_core_queue = ai_core_group_dict.get(key, deque([]))
            if not ai_core_queue:
                # 没有匹配的pmu数据, 包括AI_CPU, DSA, DVPP算子
                logging.debug("No ai core data of stream %d, task %d", datum[2], datum[1])
                union_data.append(datum + (Constant.NA,) * ai_core_data_len)
                continue
            if datum[task_type_idx] in AiCoreOpReport.HARDWARE_OP_LIST:
                # 处理task_id翻转情况下的AI_CPU, DSA, DVPP算子
                logging.debug("Found %s op of stream %d, task %d", datum[task_type_idx], datum[2], datum[1])
                union_data.append(datum + (Constant.NA,) * ai_core_data_len)
                continue
            ai_core_datum = ai_core_queue.popleft()
            if datum[task_type_idx] == Constant.TASK_TYPE_COMMUNICATION and not datum[op_name_idx].endswith(
                StrConstant.AIV_KERNEL
            ):
                # 去除运行在AI_CORE的HCCL小算子
                logging.info("Found ai core hccl small op of stream %d, task %d", datum[2], datum[1])
                continue
            union_data.append(datum + ai_core_datum)

        for stream_task, data_queue in ai_core_group_dict.items():
            if data_queue:
                logging.debug("Losing ge or task time data of stream %d, task %d", stream_task[0], stream_task[1])

        return union_data

    @staticmethod
    def _filter_hccl_op(data: list) -> list:
        filter_data = []
        # 全导和按step导，task type的索引是6; 按子图导，task type的索引是7
        task_type_idx, op_name_idx = (7, 4) if ProfilingScene().is_graph_export() else (6, 3)
        for datum in data:
            if datum[task_type_idx] in (Constant.TASK_TYPE_HCCL_AI_CPU, Constant.TASK_TYPE_COMMUNICATION) and not datum[
                op_name_idx
            ].endswith(StrConstant.AIV_KERNEL):
                logging.info("Found hccl small op of stream %d, task %d", datum[2], datum[1])
                continue
            datum = datum[:-1]  # 去除batch_id, 不在交付件中展现
            filter_data.append(datum)
        return filter_data

    @staticmethod
    def _count_num(table: str, curs: any) -> int:
        """
        count number
        """
        sql = "select count(*) from {}".format(table)
        return curs.execute(sql).fetchone()[0]

    @staticmethod
    def _get_used_headers(curs: any, table_name: str, unused_headers: list) -> list:
        """
        get used headers
        """
        all_headers = []
        for sub in DBManager.fetch_all_data(curs, "PRAGMA table_info({})".format(table_name)):
            if sub[1] not in AiCoreOpReport.UNSUPPORTED_HEADER:
                all_headers.append(sub[1])
        return [sub for sub in all_headers if sub not in unused_headers]

    @staticmethod
    def _get_ai_core_table_sql(ai_core_used_headers: list) -> str:
        """
        get union sql statement from ai core tables
        """

        for index, header in enumerate(ai_core_used_headers):
            if header in AiCoreOpReport.UNSUPPORTED_HEADER:
                ai_core_used_headers[index] = "'N/A'"
        used_headers = ",".join(ai_core_used_headers)
        subtask_id = ",(case when subtask_id={} then 'N/A' else subtask_id end) ".format(
            NumberConstant.DEFAULT_GE_CONTEXT_ID
        )
        if not ChipManager().is_chip_v4():
            subtask_id = ",'N/A'"
        return "select {1}, batch_id, task_id, stream_id {subtask_id} from {0}".format(
            DBNameConstant.TABLE_SUMMARY_METRICS, used_headers, subtask_id=subtask_id
        )

    @staticmethod
    def _get_ai_core_float_cols(columns: list) -> list:
        """
        get ai core columns with float types
        """
        ai_core_events_map = read_cpu_cfg("ai_core", "event2metric")
        all_float_cols = []
        ai_core_float_cols = Utils.generator_to_list(sub for sub in columns)
        if isinstance(ai_core_events_map, dict):
            for val in ai_core_events_map.values():
                all_float_cols.append(val.replace("(GB/s)", ""))
                all_float_cols.append(val.replace("_ratio", "_time"))
            all_float_cols.append("total_time")
            all_float_cols.append("aic_total_time")
            all_float_cols.append("aiv_total_time")
            for index, col in enumerate(columns):
                if 'cycles' not in col:
                    # keep three decimal places for ai core float data
                    ai_core_float_cols[index] = "round({0}, {1})".format(col, NumberConstant.ROUND_THREE_DECIMAL)
        return ai_core_float_cols

    @classmethod
    def get_op_summary_data(cls: any, project_path: str, db_path: str, configs: dict) -> tuple:
        """
        get op summary data
        :param project_path: sqlite file path
        :param db_path: db path
        :param configs: info config
        :return: headers and data
        """
        headers = cls.get_op_summary_header(configs)
        data, headers = cls.get_ai_core_op_summary_data_with_headers(project_path, db_path, headers)
        data.extend(cls.get_hccl_op_data(project_path, len(headers)))
        cls.delete_useless_cols(headers, data)
        cls.sort_summary_data(headers, data)
        task_data = cls._format_summary_data(*format_pmu_data_by_headers(headers, data))
        start_ts, _ = InfoConfReader().get_collect_time()
        task_start_index = headers.index(cls.TASK_START_TIME)
        task_duration_index = headers.index(cls.TASK_DURATION)
        logging.info(
            "There are %d records before op_summary data filtering, timestamp is %s.", len(task_data), start_ts
        )
        filtered_data = Utils.filter_data_by_start_time_condition(
            task_data,
            start_ts,
            lambda d: (d[task_start_index], float_calculate([d[task_start_index], d[task_duration_index]])),
        )
        if 0 < len(filtered_data) != len(task_data):
            filtered_data[0][headers.index(cls.TASK_WAIT_TIME)] = 0
        logging.info("There are %d records after op_summary data filtering.", len(filtered_data))
        add_aicore_units(headers)
        return headers, filtered_data, len(filtered_data)

    @classmethod
    def sort_summary_data(cls, headers, data):
        if StrConstant.TASK_START_TIME in headers:
            task_start_index = headers.index(StrConstant.TASK_START_TIME)
            data.sort(key=lambda x: float(x[task_start_index]))

    @classmethod
    def get_hccl_op_data(cls, project_path: str, header_length: int) -> list:
        """
        get hccl op summary data
        """
        if not os.path.exists(PathManager.get_db_path(project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)):
            return []
        with HcclViewModel(
            project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE, [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE]
        ) as hccl_model:
            if not hccl_model.check_table():
                return []
            hccl_communication_data = hccl_model.get_hccl_op_data_by_group()
        if not hccl_communication_data:
            return []

        model_name = []
        index_id = []
        model_name_and_id_dict = get_ge_model_name_dict(project_path)
        hccl_data = [0] * len(hccl_communication_data)
        # hccl data for op summary
        for index, _hccl_op in enumerate(hccl_communication_data):
            if ProfilingScene().is_graph_export():
                model_name = [model_name_and_id_dict.get(_hccl_op.model_id, Constant.NA)]
                index_id = [_hccl_op.index_id]
            hccl_data[index] = (
                model_name
                + [_hccl_op.model_id, Constant.NA, Constant.NA]
                + index_id
                + [
                    _hccl_op.op_name,
                    _hccl_op.op_type,
                    Constant.NA,
                    _hccl_op.task_type,
                    int(_hccl_op.timestamp),
                    int(_hccl_op.duration),
                    Constant.DEFAULT_VALUE,
                    Constant.DEFAULT_VALUE,
                ]
            )
            hccl_data[index].extend([Constant.NA] * (header_length - len(hccl_data[index])))
        return hccl_data

    @classmethod
    def get_op_summary_header(cls: any, configs: dict) -> list:
        """
        get op summary header
        :param configs: to get headers
        :return: headers
        """
        headers = configs.get(StrConstant.CONFIG_HEADERS)
        if ProfilingScene().is_graph_export():
            return headers
        for head in cls.OPERATOR_UNUSED_HEADERS:
            if head in headers:
                headers.remove(head)
        return headers

    @classmethod
    def clear_no_ge_data_headers(cls: any, headers: list) -> None:
        i = 0
        while i < len(headers):
            if headers[i] in cls.HEADERS_WITH_NO_GE_DATA:
                headers.remove(headers[i])
                continue
            i += 1

    @classmethod
    def get_ai_core_op_summary_data_with_headers(cls: any, project_path: str, db_path: str, headers: list) -> tuple:
        """
        get ai core op summary data
        :param project_path:
        :param db_path:
        :param configs:
        :return:
        """
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not cls._check_op_summary_table(conn, curs, headers):
            cls.clear_no_ge_data_headers(headers)
            headers.append(cls.ADDITION_HEADER[0])
            return [], headers
        try:
            return cls._get_op_summary_data_with_headers(project_path, curs, headers)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as op_err:
            logging.error(str(op_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return [], headers
        finally:
            DBManager.destroy_db_connect(conn, curs)

    @classmethod
    def delete_useless_cols(cls: any, headers: list, summary_data: list) -> list:
        if ChipManager().is_stars_chip():
            return summary_data
        for header in cls.ADDITION_HEADER:
            if header not in headers:
                continue
            index_id = headers.index(header)
            headers.remove(header)
            for index, data in enumerate(summary_data):
                if index_id >= len(data):
                    continue
                tmp_data = list(data)
                tmp_data.pop(index_id)
                summary_data[index] = tmp_data
        return summary_data

    @classmethod
    def delete_special_tag(cls, ai_core_head_list: list) -> list:
        for index, header in enumerate(ai_core_head_list):
            for sp_header in AiCoreOpReport.SPECIAL_AI_CORE_HEAD:
                ai_core_head_list[index] = header.replace(sp_header, "")
        return ai_core_head_list

    @classmethod
    def _format_summary_data(cls, headers: list, device_tasks: list) -> list:
        result = filter(
            lambda x: x not in headers, [StrConstant.TASK_START_TIME, cls.TASK_DURATION, cls.TASK_WAIT_TIME]
        )
        if list(result):
            logging.error("Op_summary_data don't have start time or duration")
        else:
            task_start_index = headers.index(StrConstant.TASK_START_TIME)
            task_duration_index = headers.index(cls.TASK_DURATION)
            task_wait_time_index = headers.index(cls.TASK_WAIT_TIME)
            prev_start_time = 0
            prev_duration = 0
            for i, task in enumerate(device_tasks):
                if i == 0:
                    task[task_wait_time_index] = 0
                    prev_start_time = task[task_start_index]
                    prev_duration = task[task_duration_index]
                else:
                    task[task_wait_time_index] = max(task[task_start_index] - prev_start_time - prev_duration, 0)
                    task[task_wait_time_index] = round(
                        task[task_wait_time_index] / NumberConstant.NS_TO_US, NumberConstant.ROUND_THREE_DECIMAL
                    )
                    prev_start_time = task[task_start_index]
                    prev_duration = task[task_duration_index]
                task[task_start_index] = format_high_precision_for_csv(
                    InfoConfReader().trans_into_local_time(task[task_start_index])
                )
                task[task_duration_index] = round(
                    task[task_duration_index] / NumberConstant.NS_TO_US, NumberConstant.ROUND_THREE_DECIMAL
                )
        return device_tasks

    @classmethod
    def _check_ai_cpu_data(cls: any, conn: any, curs: any) -> bool:
        """
        check ai cpu sqlite data
        Return: True or False
        """
        if not (conn and curs):
            return False
        if not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_SUMMARY_TASK_TIME):
            return False
        if not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_SUMMARY_GE):
            return False
        return True

    @classmethod
    def _check_op_summary_table(cls: any, conn: any, curs: any, headers: list) -> bool:
        """
        1. conn, curs 不存在,返回False
        2. TASK_TIME表不存在,返回False.无device task数据理应不呈现
        3. GE_SUMMARY(TASK_INTO)表不存在,返回False.
        """
        if not (conn and curs):
            return False
        if not DBManager.judge_table_exist(
            curs, DBNameConstant.TABLE_SUMMARY_TASK_TIME
        ) or not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_SUMMARY_GE):
            logging.warning("No device task info or op desc found.")
            return False
        return True

    @classmethod
    def _get_op_summary_data_with_headers(cls: any, project_path: str, curs: any, headers: list) -> tuple:
        union_sql, headers = cls._get_sql_and_headers(headers)
        headers.append(cls.ADDITION_HEADER[0])
        ai_core_group_dict, headers = cls._get_aicore_data(curs, headers)
        filter_params = (Constant.TASK_TYPE_WRITE_BACK, Constant.TASK_TYPE_INVALID)
        data = DBManager.fetch_all_data(curs, union_sql, filter_params)
        if not data:
            return [], headers
        data = cls._union_task_ge_ai_core_data(data, ai_core_group_dict)
        data = cls._update_op_name_from_hash(project_path, data)
        if ProfilingScene().is_graph_export():
            data = cls._update_model_name_and_infer_id(project_path, data)
        DataManager.add_memory_bound(headers, data)
        DataManager.add_cube_usage(headers, data)
        return data, headers

    @classmethod
    def _update_model_name_and_infer_id(cls: any, project_path: str, ai_core_data: list) -> list:
        model_dict = get_ge_model_name_dict(project_path)
        result_data = []
        for _data in ai_core_data:
            _data = list(_data)
            model_name = model_dict.get(_data[0], Constant.NA)
            _data.insert(cls.MODEL_NAME_INDEX, model_name)
            result_data.append(_data)
        return result_data

    @classmethod
    def _update_op_name_from_hash(cls: any, project_path: str, ai_core_data: list) -> list:
        hash_dict = get_ge_hash_dict(project_path)
        result_data = []
        if not hash_dict:
            logging.warning("create op_summary data may be an error, because table GeHashInfo is not found.")
            return [list(_data) for _data in ai_core_data]
        for _data in ai_core_data:
            _data = list(_data)
            _data[3] = hash_dict.get(_data[3], _data[3])  # op_name
            result_data.append(_data)
        return result_data

    @classmethod
    def _group_by_stream_task(cls: any, ai_core_data: list) -> dict:
        if not ai_core_data:
            return {}
        idx = -4  # 唯一id的起始下标, 将最后4个元素（batch_id, task_id, stream_id, subtask_id）作为唯一id
        # 下标-4是batch_id, 当batch_id==-1时, 不支持batch_id匹配pmu数据
        if ai_core_data[0][-4] == -1:
            cls.IS_PMU_UNIQUE_ID = False
            idx = -3  # 不支持唯一id, 将最后3个元素（task_id, stream_id, subtask_id）作为pmu匹配的依据
        ai_core_group_dict = {}
        for ai_core_datum in ai_core_data:
            # the last 4 element is batch_id, task id, stream id, subtask_id
            ai_core_group_value = ai_core_group_dict.setdefault(ai_core_datum[idx:], deque([]))
            ai_core_group_value.append(ai_core_datum[:-4])
        return ai_core_group_dict

    @classmethod
    def _get_aicore_data(cls: any, curs: any, headers: list) -> tuple:
        if DBManager.judge_table_exist(curs, DBNameConstant.TABLE_SUMMARY_METRICS) and cls._count_num(
            DBNameConstant.TABLE_SUMMARY_METRICS, curs
        ):
            ai_core_headers = cls._get_used_headers(curs, DBNameConstant.TABLE_SUMMARY_METRICS, cls.AI_CORE_UNUSED_COLS)
            ai_core_used_cols = cls._get_ai_core_float_cols(ai_core_headers)
            ai_core_data = DBManager.fetch_all_data(curs, cls._get_ai_core_table_sql(ai_core_used_cols))
            ai_core_group_dict = cls._group_by_stream_task(ai_core_data)
            headers.extend(cls.delete_special_tag(ai_core_headers))
            return ai_core_group_dict, headers
        return {}, headers

    @classmethod
    def _get_tensor_table_sql_and_headers(cls: any, headers: list) -> tuple:
        # ge or subtask need modify the context_id or subtask_id so that it should be same.
        sql = (
            "select {1}.model_id, {0}.task_id, {0}.stream_id, {index_info}"
            "{1}.op_name, {1}.op_type, "
            "(case when {1}.op_state is 'N/A' then 'N/A' "
            "when {1}.op_state is '1' then 'dynamic'"
            "when {1}.op_state is '0' then 'static' end), "
            "{1}.task_type,"
            "{0}.start_time, {0}.duration_time,"
            "{0}.wait_time, {1}.block_num, {1}.mix_block_num, {1}.op_flag,"
            "(case when {1}.input_shapes is NULL then 'N/A' else {1}.input_shapes end), "
            "(case when {1}.input_data_types is NULL then 'N/A' else {1}.input_data_types end), "
            "(case when {1}.input_formats is NULL then 'N/A' else {1}.input_formats end), "
            "(case when {1}.output_shapes is NULL then 'N/A' else {1}.output_shapes end), "
            "(case when {1}.output_data_types is NULL then 'N/A' else {1}.output_data_types end), "
            "(case when {1}.output_formats is NULL then 'N/A' else {1}.output_formats end), "
            "(case when {1}.context_id={context_id} then 'N/A' else {1}.context_id end), "
            "{0}.batch_id "
            "from {0} inner join {1} on {0}.task_id={1}.task_id and {0}.stream_id={1}.stream_id "
            "and {1}.task_type != ? and {1}.task_type != ? "
            "and {0}.batch_id={1}.batch_id "
            "and {1}.context_id={0}.subtask_id and {0}.start_time != {2} "
            "order by start_time".format(
                DBNameConstant.TABLE_SUMMARY_TASK_TIME,
                DBNameConstant.TABLE_SUMMARY_GE,
                NumberConstant.INVALID_TASK_TIME,
                context_id=NumberConstant.DEFAULT_GE_CONTEXT_ID,
                index_info=cls._get_index_id_sql_condition(),
            )
        )
        headers += cls.TENSOR_HEADERS
        return sql, headers

    @classmethod
    def _get_index_id_sql_condition(cls):
        """
        whether to append the condition for index id.
        """
        index_info = "{0}.index_id,".format(DBNameConstant.TABLE_SUMMARY_TASK_TIME)
        if ProfilingScene().is_all_export() or ProfilingScene().is_step_export():
            index_info = ''
        return index_info

    @classmethod
    def _get_sql_and_headers(cls: any, headers: list) -> tuple:
        return cls._get_tensor_table_sql_and_headers(headers)


class ReportOPCounter:
    """
    class to report op counter data
    """

    OPERATOR_UNUSED_HEADERS = ["Model Name", "Infer ID"]

    @staticmethod
    def check_param(conn: any, curs: any) -> bool:
        """
        check exist of db table
        """
        if not (conn and curs) or not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_OP_COUNTER_OP_REPORT):
            return False
        return True

    @staticmethod
    def _get_op_report_sql_operator_scene() -> str:
        sql = (
            "select op_type, core_type, occurrences, total_time, "
            "min, avg, max, ratio from {0} "
            "where op_type != 'N/A' and core_type!=? and core_type!=? order by total_time desc".format(
                DBNameConstant.TABLE_OP_COUNTER_OP_REPORT,
            )
        )
        return sql

    @staticmethod
    def _get_op_report_sql_network_scene() -> str:
        sql = (
            "select model_name, op_type, core_type, occurrences, total_time, "
            "min, avg, max, ratio from {0} "
            "where op_type != 'N/A' and core_type!=? and core_type!=? order by model_name asc, "
            "total_time desc".format(DBNameConstant.TABLE_OP_COUNTER_OP_REPORT)
        )
        return sql

    @classmethod
    def report_op(cls: any, db_path: str, headers: list) -> tuple:
        """
        report op counter
        :param db_path: DB path
        :param headers: table headers
        :return: headers, data, data length
        """
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not cls.check_param(conn, curs):
            DBManager.destroy_db_connect(conn, curs)
            return MsvpConstant.MSVP_EMPTY_DATA
        sql = cls._get_op_report_sql_network_scene()
        if ProfilingScene().is_all_export() or ProfilingScene().is_step_export():
            sql = cls._get_op_report_sql_operator_scene()
            cls._clear_unused_headers(headers)
        filter_params = (Constant.TASK_TYPE_WRITE_BACK, Constant.TASK_TYPE_INVALID)
        data = DBManager.fetch_all_data(curs, sql, filter_params)
        data = cls._format_statistic_data(data, headers)
        DBManager.destroy_db_connect(conn, curs)
        return headers, data, len(data)

    @classmethod
    def _clear_unused_headers(cls: any, headers: list) -> None:
        for head in cls.OPERATOR_UNUSED_HEADERS:
            if head in headers:
                headers.remove(head)

    @classmethod
    def _format_statistic_data(cls: any, statistic_data: list, headers: list) -> list:
        headers_dict = {value: index for index, value in enumerate(headers)}
        total_time_index = headers_dict.get('Total Time(us)')
        min_time_index = headers_dict.get('Min Time(us)')
        avg_time_index = headers_dict.get('Avg Time(us)')
        max_time_index = headers_dict.get('Max Time(us)')
        ratio_index = headers_dict.get('Ratio(%)')
        check_list = [total_time_index, min_time_index, avg_time_index, max_time_index, ratio_index]
        if any(item is None for item in check_list):
            return statistic_data
        for i, data in enumerate(statistic_data):
            data = list(data)
            data[total_time_index] = round(
                data[total_time_index] / NumberConstant.NS_TO_US, NumberConstant.ROUND_THREE_DECIMAL
            )
            data[min_time_index] = round(
                data[min_time_index] / NumberConstant.NS_TO_US, NumberConstant.ROUND_THREE_DECIMAL
            )
            data[avg_time_index] = round(
                data[avg_time_index] / NumberConstant.NS_TO_US, NumberConstant.ROUND_THREE_DECIMAL
            )
            data[max_time_index] = round(
                data[max_time_index] / NumberConstant.NS_TO_US, NumberConstant.ROUND_THREE_DECIMAL
            )
            data[ratio_index] = round(float(data[ratio_index]), NumberConstant.ROUND_THREE_DECIMAL)
            statistic_data[i] = data
        return statistic_data
