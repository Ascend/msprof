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

from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant, PmuMetricCalculate
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.ai_core_metrics_manager import AiCoreMetricsManager
from common_func.platform.chip_manager import ChipManager


class CalculateAiCoreData:
    """
    calculate aicore data
    """

    def __init__(self: any, project_path: str) -> None:
        self.project_path = project_path

    @staticmethod
    def get_vector_num() -> list:
        """
        get vector fops index coefficient
        :return: vector_num
        """
        if ChipManager().is_chip_v3():
            vector_num = [64.0, 16.0, 16.0]
        else:
            vector_num = [64.0, 64.0, 32.0]
        return vector_num

    @staticmethod
    def get_exclude_calculation_formula_pmu_list() -> list:
        """
        get get_exclude_calculation_formula_pmu_list
        :return: formula pmu list which is no need to calculate with task_cyc or freq,
                 only use for the formula that is different from different chips
        """
        if ChipManager().is_chip_v6():
            return ["vec_resc_cflt_ratio"]
        return []

    @staticmethod
    def add_pipe_time(pmu_dict: dict, total_time: float, metrics_type: str):
        """
        calculate pipe time data
        :return: pmu dict
        """
        if metrics_type not in {
            AiCoreMetricsManager.PMU_PIPE,
            AiCoreMetricsManager.PMU_PIPE_EXCT,
            AiCoreMetricsManager.PMU_PIPE_EXECUT,
            AiCoreMetricsManager.PMU_SCALAR_RATIO,
        }:
            return pmu_dict
        res_dict = {}
        valid_metrics_set = CalculateAiCoreData.get_pmu_valid_metrics_set()
        for pmu_key, pmu_value in pmu_dict.items():
            res_dict[pmu_key] = pmu_value
            if pmu_key not in valid_metrics_set:
                continue
            if pmu_key.endswith(StrConstant.RATIO_NAME):
                res_dict["{}time".format(pmu_key[: -NumberConstant.RATIO_NAME_LEN])] = [total_time * pmu_value[0]]
            elif pmu_key.endswith(StrConstant.RATIO_EXTRA_NAME):
                res_dict["{}time".format(pmu_key[: -NumberConstant.EXTRA_RATIO_NAME_LEN])] = [total_time * pmu_value[0]]
        return res_dict

    @staticmethod
    def get_pmu_valid_metrics_set() -> set:
        return AiCoreMetricsManager.VALID_METRICS_SET

    @staticmethod
    def update_fops_data(field: str, algo: str) -> str:
        """
        update fops data in sample_based
        :return: algo
        """
        if field != "vector_fops":
            return algo
        if ChipManager().is_chip_v1():
            algo = algo.replace("r4b_num", "32.0")
            algo = algo.replace("r4e_num", "64.0")
            algo = algo.replace("r4f_num", "32.0")
        elif ChipManager().is_chip_v3():
            algo = algo.replace("r4b_num", "64.0")
            algo = algo.replace("r4e_num", "16.0")
            algo = algo.replace("r4f_num", "16.0")
        else:
            algo = algo.replace("r4b_num", "64.0")
            algo = algo.replace("r4e_num", "64.0")
            algo = algo.replace("r4f_num", "32.0")
        return algo

    @classmethod
    def _calculate_vec_fp16_ratio(cls, events_name_list: list, ai_core_profiling_events: dict) -> dict:
        names = ["vec_fp16_128lane_ratio", "vec_fp16_64lane_ratio"]
        if all(map(lambda name: name in events_name_list, names)):
            vec_fp16_ratio = (
                ai_core_profiling_events["vec_fp16_128lane_ratio"][-1]
                + ai_core_profiling_events["vec_fp16_64lane_ratio"][-1]
            )
            ai_core_profiling_events.setdefault("vec_fp16_ratio", []).append(vec_fp16_ratio)
        return ai_core_profiling_events

    @classmethod
    def _calculate_cube_fops(cls, events_name_list: list, ai_core_profiling_events: dict, task_cyc: int) -> dict:
        names = ["mac_fp16_ratio", "mac_int8_ratio"]
        if all(map(lambda name: name in events_name_list, names)):
            cube_fops = (
                ai_core_profiling_events["mac_fp16_ratio"][-1] * task_cyc * 16 * 16 * 16 * 2
                + ai_core_profiling_events["mac_int8_ratio"][-1] * task_cyc * 16 * 16 * 32 * 2
            )
            ai_core_profiling_events.setdefault("cube_fops", []).append(cube_fops)
        return ai_core_profiling_events

    @classmethod
    def _calculate_mac_ratio_extra(cls, events_name_list: list, ai_core_profiling_events: dict) -> dict:
        names = ["mac_fp16_ratio", "mac_int8_ratio", "fixpipe_ratio"]
        if all(map(lambda name: name in events_name_list, names)):
            mac_ratio = ai_core_profiling_events["mac_fp16_ratio"][-1] + ai_core_profiling_events["mac_int8_ratio"][-1]
            ai_core_profiling_events.pop("cube_fops")
            ai_core_profiling_events.pop("mac_fp16_ratio")
            ai_core_profiling_events.pop("mac_int8_ratio")
            ai_core_profiling_events.setdefault("mac_ratio_extra", []).append(mac_ratio)

        names = ["mac_fp_ratio", "mac_int_ratio"]
        if all(map(lambda name: name in events_name_list, names)):
            mac_ratio = ai_core_profiling_events["mac_fp_ratio"][-1] + ai_core_profiling_events["mac_int_ratio"][-1]
            ai_core_profiling_events.pop("mac_fp_ratio")
            ai_core_profiling_events.pop("mac_int_ratio")
            ai_core_profiling_events.setdefault("mac_ratio_extra", []).append(mac_ratio)
        return ai_core_profiling_events

    @classmethod
    def _calculate_iq_full_ratio(cls, events_name_list: list, ai_core_profiling_events: dict) -> dict:
        names = [
            "mte1_iq_full_ratio",
            "mte2_iq_full_ratio",
            "mte3_iq_full_ratio",
            "cube_iq_full_ratio",
            "vec_iq_full_ratio",
        ]
        if all(map(lambda name: name in events_name_list, names)):
            iq_full_ratio = (
                ai_core_profiling_events["mte1_iq_full_ratio"][-1]
                + ai_core_profiling_events["mte2_iq_full_ratio"][-1]
                + ai_core_profiling_events["mte3_iq_full_ratio"][-1]
                + ai_core_profiling_events["cube_iq_full_ratio"][-1]
                + ai_core_profiling_events["vec_iq_full_ratio"][-1]
            )
            ai_core_profiling_events.setdefault("iq_full_ratio", []).append(iq_full_ratio)
        return ai_core_profiling_events

    @classmethod
    def _calculate_icache_miss_rate(cls, events_name_list: list, ai_core_profiling_events: dict) -> dict:
        names = ["icache_miss_rate", "icache_req_ratio"]
        if all(map(lambda name: name in events_name_list, names)):
            if ai_core_profiling_events["icache_req_ratio"][-1] != 0:
                icache_miss_rate = (
                    ai_core_profiling_events["icache_miss_rate"][-1] / ai_core_profiling_events["icache_req_ratio"][-1]
                )

                ai_core_profiling_events.get("icache_miss_rate").pop()
                ai_core_profiling_events.setdefault("icache_miss_rate", []).append(icache_miss_rate)
        return ai_core_profiling_events

    @classmethod
    def _calculate_memory_bandwidth(cls, events_name_list: list, ai_core_profiling_events: dict) -> dict:
        names = ["main_mem_read_bw(GB/s)", "main_mem_write_bw(GB/s)", "mte2_ratio", "mte3_ratio"]
        if all(map(lambda name: name in events_name_list, names)):
            if ai_core_profiling_events["mte2_ratio"][-1] == 0 or ai_core_profiling_events["mte3_ratio"][-1] == 0:
                logging.error("mte_cyc is 0, please check the data!")
                return ai_core_profiling_events

            main_mem_read_bw = (
                ai_core_profiling_events["main_mem_read_bw(GB/s)"][-1] / ai_core_profiling_events["mte2_ratio"][-1]
            )
            del ai_core_profiling_events["mte2_ratio"]
            ai_core_profiling_events.get("main_mem_read_bw(GB/s)").pop()
            ai_core_profiling_events.setdefault("main_mem_read_bw(GB/s)", []).append(main_mem_read_bw)

            main_mem_write_bw = (
                ai_core_profiling_events["main_mem_write_bw(GB/s)"][-1] / ai_core_profiling_events["mte3_ratio"][-1]
            )
            del ai_core_profiling_events["mte3_ratio"]
            ai_core_profiling_events.get("main_mem_write_bw(GB/s)").pop()
            ai_core_profiling_events.setdefault("main_mem_write_bw(GB/s)", []).append(main_mem_write_bw)
        return ai_core_profiling_events

    @classmethod
    def _calculate_control_flow_mis_prediction_rate(
        cls, events_name_list: list, ai_core_profiling_events: dict
    ) -> dict:
        names = ["control_flow_prediction_ratio", "control_flow_mis_prediction_rate"]
        if all(map(lambda name: name in events_name_list, names)):
            if ai_core_profiling_events["control_flow_prediction_ratio"][-1] != 0:
                miss_rate = (
                    ai_core_profiling_events["control_flow_mis_prediction_rate"][-1]
                    / ai_core_profiling_events["control_flow_prediction_ratio"][-1]
                )

                ai_core_profiling_events.get("control_flow_mis_prediction_rate").pop()
                ai_core_profiling_events.setdefault("control_flow_mis_prediction_rate", []).append(miss_rate)
        return ai_core_profiling_events

    @classmethod
    def _calculate_ub_read_bw_vector(cls, events_name_list: list, ai_core_profiling_events: dict) -> dict:
        if ChipManager().is_chip_v1_1() and "ub_read_bw_vector(GB/s)" in events_name_list:
            ai_core_profiling_events["ub_read_bw_vector(GB/s)"] = [
                sum(ai_core_profiling_events["ub_read_bw_vector(GB/s)"])
            ]
        return ai_core_profiling_events

    @classmethod
    def _calculate_vec_bank_cflt_ratio(cls, events_name_list: list, ai_core_profiling_events: dict) -> dict:
        if ChipManager().is_chip_v6():
            names = ["vec_bank_cflt_ratio", "stu_pmu_wctl_ub_cflt"]
            if all(map(lambda name: name in events_name_list, names)):
                vec_bank_cflt_ratio = (
                    ai_core_profiling_events["vec_bank_cflt_ratio"][-1]
                    + ai_core_profiling_events["stu_pmu_wctl_ub_cflt"][-1]
                )
                ai_core_profiling_events["vec_bank_cflt_ratio"] = [vec_bank_cflt_ratio]
        return ai_core_profiling_events

    @classmethod
    def _calculate_vec_resc_cflt_ratio(cls, events_name_list: list, ai_core_profiling_events: dict) -> dict:
        if ChipManager().is_chip_v6():
            names = ["vec_resc_cflt_ratio", "pmu_idc_aic_vec_instr_vf_busy_o"]
            if all(map(lambda name: name in events_name_list, names)):
                if not ai_core_profiling_events["pmu_idc_aic_vec_instr_vf_busy_o"][-1]:
                    logging.warning("pmu_idc_aic_vec_instr_vf_busy_o value is zero!")
                    ai_core_profiling_events["vec_resc_cflt_ratio"] = [0]
                else:
                    vec_resc_cflt_ratio = (
                        ai_core_profiling_events["vec_resc_cflt_ratio"][-1]
                        / ai_core_profiling_events["pmu_idc_aic_vec_instr_vf_busy_o"][-1]
                    )
                    ai_core_profiling_events["vec_resc_cflt_ratio"] = [vec_resc_cflt_ratio]
        return ai_core_profiling_events

    @classmethod
    def _calculate_ub_read_bw(cls, events_name_list: list, ai_core_profiling_events: dict) -> dict:
        if ChipManager().is_chip_v6():
            names = ["ub_read_bw(GB/s)", "ub_read_bw_vector(GB/s)"]
            if all(map(lambda name: name in events_name_list, names)):
                ub_read_bw = (
                    ai_core_profiling_events["ub_read_bw(GB/s)"][-1]
                    + ai_core_profiling_events["ub_read_bw_vector(GB/s)"][-1]
                )
                ai_core_profiling_events["ub_read_bw(GB/s)"] = [ub_read_bw]
        return ai_core_profiling_events

    @classmethod
    def _calculate_vec_ub_write_bw(cls, events_name_list: list, ai_core_profiling_events: dict) -> dict:
        if ChipManager().is_chip_v6():
            names = ["ub_write_bw(GB/s)", "stu_pmu_wctl_ub_cflt"]
            if all(map(lambda name: name in events_name_list, names)):
                ub_write_bw = (
                    ai_core_profiling_events["ub_write_bw(GB/s)"][-1]
                    + ai_core_profiling_events["ub_write_bw_vector(GB/s)"][-1]
                )
                ai_core_profiling_events["ub_write_bw(GB/s)"] = [ub_write_bw]
        return ai_core_profiling_events

    def add_fops_header(self: any, metric_key: str, metrics: list) -> None:
        """
        add fops index item to metrics in ArithmeticUtilization
        :param metric_key: metric key
        :param metrics: metrics
        :return: None
        """
        sample_config = ConfigMgr.read_sample_config(self.project_path)
        cube_ratio_metrics = {"mac_fp16_ratio", "mac_int8_ratio"}
        vector_ratio_metrics = {"vec_fp16_ratio", "vec_fp32_ratio", "vec_int32_ratio", "vec_misc_ratio"}
        if sample_config.get(metric_key) == "ArithmeticUtilization":
            if all(map(lambda name: name in metrics, cube_ratio_metrics)):
                metrics.append("cube_fops")
            if all(map(lambda name: name in metrics, vector_ratio_metrics)):
                metrics.append("vector_fops")

    def compute_ai_core_data(
        self: any, events_name_list: list, ai_core_profiling_events: dict, task_cyc: int, pmu_data: list
    ) -> tuple:
        """
        compute ai core pmu event
        :return: events_name_list, ai_core_profiling_events
        """
        try:
            self._cal_pmu_metrics(ai_core_profiling_events, events_name_list, pmu_data, task_cyc)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s", str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return events_name_list, ai_core_profiling_events
        ai_core_profiling_events = self.__cal_addition(events_name_list, ai_core_profiling_events, task_cyc)
        return events_name_list, ai_core_profiling_events

    def add_vector_data(self: any, events_name_list: list, ai_core_profiling_events: dict, task_cyc: int) -> None:
        """
        calculate vector fops index data
        :return:
        """
        names = [
            "vec_fp16_128lane_ratio",
            "vec_fp16_64lane_ratio",
            "vec_fp32_ratio",
            "vec_int32_ratio",
            "vec_misc_ratio",
        ]
        if all(map(lambda name: name in events_name_list, names)):
            vector_num = self.get_vector_num()
            vec_fp16_128lane_ratio = ai_core_profiling_events["vec_fp16_128lane_ratio"][-1] * task_cyc * 128
            vec_fp16_64lane_ratio = ai_core_profiling_events["vec_fp16_64lane_ratio"][-1] * task_cyc * 64
            vec_fp32_ratio = ai_core_profiling_events["vec_fp32_ratio"][-1] * task_cyc * vector_num[0]
            vec_int32_ratio = ai_core_profiling_events["vec_int32_ratio"][-1] * task_cyc * vector_num[1]
            vec_misc_ratio = ai_core_profiling_events["vec_misc_ratio"][-1] * task_cyc * vector_num[2]
            vector_fops = (
                vec_fp16_128lane_ratio + vec_fp16_64lane_ratio + vec_fp32_ratio + vec_int32_ratio + vec_misc_ratio
            )
            ai_core_profiling_events.setdefault("vector_fops", []).append(vector_fops)

    @staticmethod
    def _cal_bandwidth(pmu_name: str, pmu_value: any, task_cyc: int, freq: any) -> float:
        """
        calculate bandwidth metric, chip v6 main_mem uses new coefficient 128.0,
        other chips keep 8.0
        :return: bandwidth value
        """
        if pmu_name in ('main_mem_read_bw(GB/s)', 'main_mem_write_bw(GB/s)') and ChipManager().is_chip_v6():
            return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 128.0, 8.0, pmu_value, task_cyc, freq)
        return Constant.AI_CORE_CALCULATION_FORMULA.get(pmu_name)(pmu_value, task_cyc, freq)

    def _cal_pmu_metrics(
        self: any, ai_core_profiling_events: dict, events_name_list: list, pmu_data: list, task_cyc: int
    ) -> None:
        freq = InfoConfReader().get_freq(StrConstant.AIC)
        exclude_calculation_formula_pmu_list = self.get_exclude_calculation_formula_pmu_list()
        for pmu_name, pmu_value in zip(events_name_list, pmu_data):
            if task_cyc and pmu_name in [
                item
                for item in list(Constant.AI_CORE_CALCULATION_FORMULA.keys())
                if item not in exclude_calculation_formula_pmu_list
            ]:
                if StrConstant.BANDWIDTH in pmu_name:
                    ai_core_profiling_events.setdefault(pmu_name, []).append(
                        self._cal_bandwidth(pmu_name, pmu_value, task_cyc, freq)
                    )
                else:
                    ai_core_profiling_events.setdefault(pmu_name, []).append(
                        Constant.AI_CORE_CALCULATION_FORMULA.get(pmu_name)(pmu_value, task_cyc)
                    )
            else:
                ai_core_profiling_events.setdefault(pmu_name, []).append(pmu_value)

    def __cal_addition(self: any, events_name_list: list, ai_core_profiling_events: dict, task_cyc: int) -> dict:
        """
        calculate additional ai core metrics
        :return:
        """
        ai_core_profiling_events = self._calculate_ub_read_bw_vector(events_name_list, ai_core_profiling_events)
        ai_core_profiling_events = self._calculate_vec_fp16_ratio(events_name_list, ai_core_profiling_events)
        ai_core_profiling_events = self._calculate_cube_fops(events_name_list, ai_core_profiling_events, task_cyc)
        ai_core_profiling_events = self._calculate_mac_ratio_extra(events_name_list, ai_core_profiling_events)
        ai_core_profiling_events = self._calculate_iq_full_ratio(events_name_list, ai_core_profiling_events)
        ai_core_profiling_events = self._calculate_icache_miss_rate(events_name_list, ai_core_profiling_events)
        ai_core_profiling_events = self._calculate_memory_bandwidth(events_name_list, ai_core_profiling_events)
        ai_core_profiling_events = self._calculate_control_flow_mis_prediction_rate(
            events_name_list, ai_core_profiling_events
        )
        ai_core_profiling_events = self._calculate_vec_bank_cflt_ratio(events_name_list, ai_core_profiling_events)
        ai_core_profiling_events = self._calculate_vec_resc_cflt_ratio(events_name_list, ai_core_profiling_events)
        ai_core_profiling_events = self._calculate_ub_read_bw(events_name_list, ai_core_profiling_events)
        ai_core_profiling_events = self._calculate_vec_ub_write_bw(events_name_list, ai_core_profiling_events)

        self.add_vector_data(events_name_list, ai_core_profiling_events, task_cyc)
        return ai_core_profiling_events
