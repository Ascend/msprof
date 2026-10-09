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
import sqlite3
from abc import ABC
from abc import abstractmethod
from collections import defaultdict

from common_func.ai_stack_data_check_manager import AiStackDataCheckManager
from common_func.platform.ai_core_metrics_manager import AiCoreMetricsManager
from common_func.common_prof_rule import CommonProfRule
from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import is_number
from common_func.msvp_constant import MsvpConstant
from common_func.path_manager import PathManager
from common_func.section_calculator import SectionCalculator
from msconfig.config_manager import ConfigManager
from msmodel.stars.op_summary_model import OpSummaryModel
from viewer.ai_core_op_report import AiCoreOpReport
from viewer.ai_core_report import AiCoreReport
from viewer.runtime_report import get_task_based_core_data


class BaseTuningDataHandle(ABC):
    """
    move to a new file in the future
    """

    TAG_KEY = ""

    @staticmethod
    @abstractmethod
    def load_data(*args):
        pass

    @staticmethod
    @abstractmethod
    def get_result(ids: any, operator_data: list):
        pass

    @staticmethod
    @abstractmethod
    def print_format(data: any):
        pass


class OpParallelTuningDataHandle(BaseTuningDataHandle):
    """
    delete the file in the future
    """

    TAG_KEY = "AI CPU Execution Time(us)"

    @staticmethod
    def load_data(*args) -> list:
        (param,) = args
        op_parallel_data = []
        project_path = param.get(StrConstant.PARAM_RESULT_DIR, '')
        sample_config = {
            'result_dir': project_path,
            'iter_id': Constant.DEFAULT_INVALID_VALUE,
            'model_id': Constant.DEFAULT_INVALID_VALUE,
        }
        device_id = param.get(StrConstant.PARAM_DEVICE_ID, '')
        if not AiStackDataCheckManager.contain_op_summary_data(project_path, device_id):
            return []
        with OpSummaryModel(sample_config) as _model:
            ai_core_data = _model.get_operator_data_by_task_type((Constant.TASK_TYPE_AI_CORE,))
            ai_cpu_data = _model.get_operator_data_by_task_type((Constant.TASK_TYPE_AI_CPU,))
        if not ai_core_data or not ai_cpu_data:
            return op_parallel_data
        ai_core_data = SectionCalculator.merge_continuous_intervals(ai_core_data)
        ai_cpu_data = SectionCalculator.merge_continuous_intervals(ai_cpu_data)
        ai_core_overlap_data = SectionCalculator.compute_overlap_time(ai_core_data, ai_cpu_data)
        ai_core_time, ai_cpu_time, overlap_time = Constant.DEFAULT_VALUE, Constant.DEFAULT_VALUE, Constant.DEFAULT_VALUE
        for ai_core_task in ai_core_overlap_data:
            overlap_time += ai_core_task.overlap_time
            ai_core_time += ai_core_task.end_time - ai_core_task.start_time
        for ai_cpu_task in ai_cpu_data:
            ai_cpu_time += ai_cpu_task.end_time - ai_cpu_task.start_time
        op_parallel_data.append(
            {
                "AI Core Execution Time(us)": (ai_core_time - overlap_time) / 1000.0,
                "AI CPU Execution Time(us)": (ai_cpu_time - overlap_time) / 1000.0,
                "Concurrent AI Core and AI CPU Execution Time(us)": overlap_time / 1000.0,
            }
        )
        return op_parallel_data

    @staticmethod
    def get_result(ids: any, operator_data: list):
        if not ids:
            return {}
        return operator_data[0]

    @staticmethod
    def print_format(data: any):
        cpu_time_in_total = sum(data.values())
        ai_cpu_ratio = data.get("AI CPU Execution Time(us)", 0.0) / cpu_time_in_total if cpu_time_in_total else 0
        return f"Percentage of AI CPU Execution Time is {ai_cpu_ratio:.2%}, Exceed the experience threshold 5%."


class OpSummaryTuningDataHandle(BaseTuningDataHandle):
    TAG_KEY = "op_name"

    @staticmethod
    def get_result(ids: any, operator_data: list):
        return ids

    @staticmethod
    def print_format(data: any):
        return "[{0}]".format(",".join(list(map(str, data))))

    @staticmethod
    def get_memory_workspace(memory_workspaces: list, operator_dict: dict) -> None:
        """
        get data for memory workspace
        """
        if memory_workspaces:
            for memory_workspace in memory_workspaces:
                if (
                    operator_dict.get("stream_id") == memory_workspace[0]
                    and str(operator_dict.get("task_id")) == memory_workspace[1]
                ):
                    operator_dict["memory_workspace"] = memory_workspace[2]
                    return
        operator_dict["memory_workspace"] = 0

    @staticmethod
    def is_network(project: str, device_id: any) -> bool:
        """
        check the scene of network
        """
        run_for_network = True
        conn, cur = DBManager.check_connect_db(project, DBNameConstant.DB_ACL_MODULE)
        try:
            if conn and cur:
                acl_op_data = cur.execute(
                    "select count(api_name) from {} where api_name=?".format(DBNameConstant.TABLE_ACL_DATA),
                    ("aclopExecute",),
                ).fetchone()
                if acl_op_data[0]:
                    run_for_network = False
            return run_for_network
        except sqlite3.Error as error:
            logging.error(error)
            return False
        finally:
            DBManager.destroy_db_connect(conn, cur)

    @staticmethod
    def _process_headers(headers: list) -> list:
        result_headers = []
        for header in headers:
            result_headers.append(header.replace(" ", "_").split("(")[0].lower())
        return result_headers

    @classmethod
    def load_data(cls: any, *args) -> list:
        pass

    @classmethod
    def get_data_by_infer_id(cls: any, para: dict) -> list:
        """
        get data by iter id.
        """
        project_path = para.get(StrConstant.PARAM_RESULT_DIR, '')
        device_id = para.get(StrConstant.PARAM_DEVICE_ID, '')
        op_data = []
        memory_workspaces = cls.select_memory_workspace(project_path, device_id)
        raw_headers, datas = cls._get_base_data(device_id, project_path)
        headers = cls._process_headers(raw_headers)
        for data in datas:
            operator_dict = {}
            for key, value in zip(headers, data):
                operator_dict[key] = value
            cls._get_extend_data(operator_dict)
            cls.get_memory_workspace(memory_workspaces, operator_dict)
            op_data.append(operator_dict)
        return op_data

    @classmethod
    def get_vector_bound(cls: any, extend_data_dict: dict, operator_dict: dict) -> None:
        """
        get data for vector bound
        """
        if (
            is_number(operator_dict.get("vec_ratio"))
            and is_number(operator_dict.get("mte2_ratio"))
            and is_number(operator_dict.get("mac_ratio"))
        ):
            extend_data_dict["vector_bound"] = 0
            if max(operator_dict.get("mte2_ratio"), operator_dict.get("mac_ratio")):
                extend_data_dict["vector_bound"] = StrConstant.ACCURACY % float(
                    operator_dict.get("vec_ratio")
                    / max(operator_dict.get("mte2_ratio"), operator_dict.get("mac_ratio"))
                )

    @classmethod
    def get_core_number(cls: any, extend_data_dict: dict) -> None:
        """
        get core number
        """
        extend_data_dict["core_num"] = InfoConfReader().get_data_under_device("ai_core_num")

    @classmethod
    def select_memory_workspace(cls: any, project: str, device_id: any) -> list:
        """
        query memory workspace from db
        """
        memory_workspaces = []
        if not cls.is_network(project, device_id):
            return memory_workspaces
        conn, cur = DBManager.check_connect_db(project, DBNameConstant.DB_GE_MODEL_INFO)
        if conn and cur and DBManager.judge_table_exist(cur, DBNameConstant.TABLE_GE_LOAD_TABLE):
            sql = "select stream_id, task_ids, memory_workspace from GELoad where memory_workspace>0 and device_id=?"
            memory_workspaces = DBManager.fetch_all_data(cur, sql, (device_id,))
        DBManager.destroy_db_connect(conn, cur)
        return memory_workspaces

    @classmethod
    def _get_base_data(cls: any, device_id: any, project_path: str) -> tuple:
        if cls.is_network(project_path, device_id):
            headers = ConfigManager.get(ConfigManager.MSPROF_EXPORT_DATA).get('op_summary', 'headers').split(",")
            configs = {StrConstant.CONFIG_HEADERS: headers}
            if not AiStackDataCheckManager.contain_op_summary_data(project_path, device_id):
                return [], []
            db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_AICORE_OP_SUMMARY)
            headers, data, _ = AiCoreOpReport.get_op_summary_data(project_path, db_path, configs)
        else:
            param = {}
            headers, data, _ = MsvpConstant.MSVP_EMPTY_DATA
            sample_config = ConfigMgr.read_sample_config(project_path)

            param[StrConstant.DATA_TYPE] = StrConstant.AI_CORE_PMU_EVENTS
            if sample_config.get(StrConstant.AICORE_PROFILING_MODE) == StrConstant.AIC_TASK_BASED_MODE:
                headers, data, _ = get_task_based_core_data(project_path, DBNameConstant.DB_RUNTIME, param)
            elif sample_config.get(StrConstant.AICORE_PROFILING_MODE) == StrConstant.AIC_SAMPLE_BASED_MODE:
                param[StrConstant.CORE_DATA_TYPE] = StrConstant.AI_CORE_PMU_EVENTS
                headers, data, _ = AiCoreReport.get_core_sample_data(project_path, DBNameConstant.DB_NAME_AICORE, param)

            if not headers or not data:
                param[StrConstant.DATA_TYPE] = StrConstant.AI_VECTOR_CORE_PMU_EVENTS
                if sample_config.get(StrConstant.AIV_PROFILING_MODE) == StrConstant.AIC_TASK_BASED_MODE:
                    headers, data, _ = get_task_based_core_data(project_path, DBNameConstant.DB_RUNTIME, param)
                elif sample_config.get(StrConstant.AIV_PROFILING_MODE) == StrConstant.AIC_SAMPLE_BASED_MODE:
                    headers, data, _ = AiCoreReport.get_core_sample_data(
                        project_path, DBNameConstant.DB_NAME_AI_VECTOR_CORE, param
                    )
        return headers, data

    @classmethod
    def _get_extend_data(cls: any, operator_dict: dict) -> dict:
        extend_data_dict = {}
        cls.get_core_number(extend_data_dict)
        cls.get_vector_bound(extend_data_dict, operator_dict)
        operator_dict.update(extend_data_dict)
        return extend_data_dict


class ModelSummaryTuningDataHandle(OpSummaryTuningDataHandle):
    TAG_KEY = StrConstant.CUBE_UTILIZATION
    BOUND_TYPE = {
        StrConstant.CUBE_UTILIZATION: [StrConstant.MAC_RATIO, StrConstant.AIC_MAC_RATIO],
        StrConstant.VECTOR_UTILIZATION: [StrConstant.VEC_RATIO, StrConstant.AIV_VEC_RATIO],
        StrConstant.SCALAR_UTILIZATION: [
            StrConstant.SCALAR_RATIO,
            StrConstant.AIC_SCALAR_RATIO,
            StrConstant.AIV_SCALAR_RATIO,
        ],
        StrConstant.MTE_UTILIZATION: [
            StrConstant.MTE1_RATIO,
            StrConstant.MTE2_RATIO,
            StrConstant.MTE3_RATIO,
            StrConstant.AIC_MTE1_RATIO,
            StrConstant.AIC_MTE2_RATIO,
            StrConstant.AIC_MTE3_RATIO,
            StrConstant.AIV_MTE1_RATIO,
            StrConstant.AIV_MTE2_RATIO,
            StrConstant.AIV_MTE3_RATIO,
        ],
    }

    @staticmethod
    def get_result(ids: any, operator_data: list):
        if not ids:
            return {}
        return operator_data[0]

    @staticmethod
    def print_format(data: any):
        total_duration = sum(data.values())
        if not total_duration:
            return "Total Duration Time equals to 0. Please check Aicore operators."
        model_cube_ratio = round(data.get(StrConstant.CUBE_UTILIZATION, 0) / total_duration, 3)
        model_vec_ratio = round(data.get(StrConstant.VECTOR_UTILIZATION, 0) / total_duration, 3)
        model_scalar_ratio = round(data.get(StrConstant.SCALAR_UTILIZATION, 0) / total_duration, 3)
        model_mte_ratio = round(data.get(StrConstant.MTE_UTILIZATION, 0) / total_duration, 3)

        return (
            f"\n\t\ta. Cube utilization rate in the model is {model_cube_ratio}. \n"
            f"\t\tb. Vector utilization rate in the model is {model_vec_ratio}. \n"
            f"\t\tc. Scalar utilization rate in the model is {model_scalar_ratio}. \n"
            f"\t\td. MTE utilization rate in the model is {model_mte_ratio}. \n"
        )

    @classmethod
    def load_data(cls: any, *args) -> list:
        """
        first load data as opSummaryDataHandle does
        then process every op_dict, finding bound type for every opR
        then summary task duration time by different bound type
        """
        param, op_data = args
        ai_core_metrics_set = {AiCoreMetricsManager.PMU_PIPE, AiCoreMetricsManager.PMU_PIPE_EXCT}
        sample_config = param.get(StrConstant.SAMPLE_CONFIG, {})
        if sample_config.get(StrConstant.AI_CORE_PROFILING_METRICS, '') not in ai_core_metrics_set:
            return []
        if not op_data:
            return []
        bound_dur_dict = defaultdict(float)
        for operator_dict in op_data:
            bound_type = cls.get_operator_bound_type(operator_dict)
            if bound_type == Constant.NA:
                continue
            task_duration = operator_dict.get(StrConstant.TASK_DURATION, 0)
            if is_number(task_duration):
                bound_dur_dict[bound_type] += task_duration
        for bound_type in cls.BOUND_TYPE:
            bound_dur_dict[bound_type] = round(bound_dur_dict[bound_type], 3)
        return [bound_dur_dict]

    @classmethod
    def get_operator_bound_type(cls: any, operator_dict: dict) -> str:
        """
        for each operator, compute vector ratio, cube ratio, scalar ratio and mte ratio
        then return bound type with the max ratio
        """
        if operator_dict.get(Constant.TASK_TYPE) == Constant.TASK_TYPE_AI_CPU:
            return Constant.NA
        bound_dict = defaultdict(float)
        for bound_type, ratio_type_list in cls.BOUND_TYPE.items():
            for ratio_type in ratio_type_list:
                ratio_tmp = operator_dict.get(ratio_type, 0)
                if is_number(ratio_tmp):
                    bound_dict[bound_type] = max(bound_dict.get(bound_type, 0), ratio_tmp)
        bound = max(bound_dict, key=bound_dict.get)
        if bound_dict[bound] <= 0:
            logging.debug("There exists an operator has no pmu info!")
            return Constant.NA
        return bound


class DataManager:
    """
    manage different types of tuning data
    """

    HANDLE_MAP = {
        CommonProfRule.TUNING_OPERATOR: OpSummaryTuningDataHandle,
        CommonProfRule.TUNING_OP_PARALLEL: OpParallelTuningDataHandle,
        CommonProfRule.TUNING_MODEL: ModelSummaryTuningDataHandle,
    }

    def __init__(self: any, param: dict) -> None:
        self.data = {}
        self._load_data(param)

    def get_data(self: any, data_type: str) -> list:
        return self.data.get(data_type, [])

    def _load_data(self: any, param: dict):
        op_data = OpSummaryTuningDataHandle.get_data_by_infer_id(param)
        self.data[CommonProfRule.TUNING_OPERATOR] = op_data
        self.data[CommonProfRule.TUNING_OP_PARALLEL] = OpParallelTuningDataHandle.load_data(param)
        self.data[CommonProfRule.TUNING_MODEL] = ModelSummaryTuningDataHandle.load_data(param, op_data)
