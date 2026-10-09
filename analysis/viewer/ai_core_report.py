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

from collections import OrderedDict

from common_func.data_manager import DataManager
from common_func.db_manager import DBManager
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import is_number
from common_func.msvp_constant import MsvpConstant
from common_func.platform.chip_manager import ChipManager
from common_func.utils import Utils
from viewer.ai_core_op_report import AiCoreOpReport
from viewer.chip_model_function.chip_model_decorators import format_pmu_data_by_headers


class AiCoreReport:
    @staticmethod
    def remove_redundant(headers: OrderedDict) -> None:
        """
        delete unnecessary headers
        """
        if not ChipManager().is_chip_v1() and "ub_read_bw_mte(GB/s)" in headers and "ub_write_bw_mte(GB/s)" in headers:
            headers.pop("ub_read_bw_mte(GB/s)")
            headers.pop("ub_write_bw_mte(GB/s)")

        if (
            not (ChipManager().is_chip_v1() or ChipManager().is_chip_v4())
            and "l2_read_bw(GB/s)" in headers
            and "l2_write_bw(GB/s)" in headers
        ):
            headers.pop("l2_read_bw(GB/s)")
            headers.pop("l2_write_bw(GB/s)")

    @classmethod
    def get_core_sample_data(cls, result_dir: str, db_name: str, params: dict) -> tuple:
        """
        get AIC/AIV sample-based data
        """
        conn, curs = DBManager.check_connect_db(result_dir, db_name)
        if not (conn and curs):
            return MsvpConstant.MSVP_EMPTY_DATA
        if (
            params.get(StrConstant.CORE_DATA_TYPE) == StrConstant.AI_CORE_PMU_EVENTS
            or params.get(StrConstant.CORE_DATA_TYPE) == StrConstant.AI_VECTOR_CORE_PMU_EVENTS
        ):
            headers, data = format_pmu_data_by_headers(*cls.get_output_event_counter(curs))
            DBManager.destroy_db_connect(conn, curs)
            return headers, data, len(data)
        DBManager.destroy_db_connect(conn, curs)
        return MsvpConstant.MSVP_EMPTY_DATA

    @classmethod
    def get_output_event_counter(cls, cursor: any) -> tuple:
        """
        get ai core event count data
        """
        if cursor is None:
            return [], []
        is_exist = DBManager.judge_table_exist(cursor, "MetricSummary")
        if not is_exist:
            return [], []
        sql_result = (
            "select metric, value from MetricSummary where metric not like '%total_time%' and value is not null;"
        )
        sql_core_id = "select distinct(coreid) from MetricSummary where value is not null order by coreid;"
        result = DBManager.fetch_all_data(cursor, sql_result)
        core_id = DBManager.fetch_all_data(cursor, sql_core_id)

        if not result or not core_id:
            return [], []
        return cls._get_output_event_counter(result, core_id)

    @classmethod
    def _get_output_event_counter(cls, result: list, core_id: list) -> tuple:
        tmp = OrderedDict()
        for i in result:
            i_data = i[-1]
            if is_number(str(i[-1])):
                i_data = round(float(i[-1]), NumberConstant.ROUND_THREE_DECIMAL)
            if i[0] in tmp:
                tmp[i[0]].append(i_data)
            else:
                tmp[i[0]] = [i_data]
        cls.remove_redundant(tmp)
        headers = ["Core ID"] + list(tmp.keys())
        headers = AiCoreOpReport.delete_special_tag(headers)
        result = [Utils.generator_to_list("Core{}".format(i[0]) for i in core_id) + ["Average"]]
        try:
            result.extend(
                (value + [round(float(sum(value) / len(value)), NumberConstant.ROUND_THREE_DECIMAL)])
                for value in tmp.values()
            )
        except ZeroDivisionError:
            return [], []
        else:
            # reverse rows and columns
            result = Utils.generator_to_list(list(x) for x in zip(*result))
            DataManager.add_memory_bound(headers, result)
            return headers, result
        finally:
            pass
