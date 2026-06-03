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

import os
import stat

from common_func.ms_constant.number_constant import NumberConstant


class PmuMetricCalculate:
    """
    class for pmu metric calculate
    """

    @staticmethod
    def pmu_metric_calculate_with_freq(float_bit: float, pip_size: float, scalar: float, *base_info: any) -> float:
        """
        calculate pmu metric with freq
        :param float_bit: 1.0
        :param pip_size: pip size
        :param scalar: scalar num
        :param base_info: pmu, task_cyc, freq
        :return: metric value
        """
        pmu, task_cyc, freq = base_info
        if NumberConstant.is_zero(task_cyc) or NumberConstant.is_zero(freq):
            return 0
        return float_bit * pmu * pip_size * scalar / (task_cyc / freq) / 8589934592.0

    @staticmethod
    def pmu_metric_calculate_without_freq(float_bit: float, *base_info: any) -> float:
        """
        calculate pmu metric without freq
        :param float_bit: 1.0
        :param base_info: pmu, task_cyc
        :return: metric value
        """
        pmu, task_cyc = base_info
        if NumberConstant.is_zero(task_cyc):
            return 0
        return float_bit * pmu / task_cyc


class PmuCalculateFunc:
    """
    define diff pmu formula
    """

    @staticmethod
    def mac_fp16_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mac_int8_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mac_fp_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mac_int_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def vec_fp32_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def vec_fp16_128lane_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def vec_fp16_64lane_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def vec_fp16_ratio(pmu1, pmu2, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu1 + pmu2, task_cyc)

    @staticmethod
    def vec_int32_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def vec_misc_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def vec_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mac_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def scalar_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def fixpipe_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mte1_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mte2_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mte3_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mte_preload_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def icache_miss_rate(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def icache_req_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def scalar_waitflag_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def cube_waitflag_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def vector_waitflag_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mte1_waitflag_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mte2_waitflag_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mte3_waitflag_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def scalar_ld_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def scalar_st_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def vec_bankgroup_cflt_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def vec_bank_cflt_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def vec_resc_cflt_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mte1_iq_full_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mte2_iq_full_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def mte3_iq_full_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def cube_iq_full_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def vec_iq_full_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def acess_stack_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def control_flow_prediction_ratio(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def control_flow_mis_prediction_rate(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def stu_pmu_wctl_ub_cflt(pmu, task_cyc):
        return PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc)

    @staticmethod
    def ub_read_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 4.0, pmu, task_cyc, freq)

    @staticmethod
    def ub_write_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 4.0, pmu, task_cyc, freq)

    @staticmethod
    def l1_read_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 16.0, pmu, task_cyc, freq)

    @staticmethod
    def l1_write_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq)

    @staticmethod
    def l2_read_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq)

    @staticmethod
    def l2_write_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq)

    @staticmethod
    def main_mem_read_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 8.0, 8.0, pmu, task_cyc, freq)

    @staticmethod
    def main_mem_write_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 8.0, 8.0, pmu, task_cyc, freq)

    @staticmethod
    def l0a_read_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 16.0, pmu, task_cyc, freq)

    @staticmethod
    def l0a_write_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 16.0, pmu, task_cyc, freq)

    @staticmethod
    def l0b_read_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq)

    @staticmethod
    def l0b_write_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq)

    @staticmethod
    def l0c_read_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq)

    @staticmethod
    def l0c_write_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq)

    @staticmethod
    def l0c_read_bw_cube(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq)

    @staticmethod
    def l0c_write_bw_cube(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq)

    @staticmethod
    def ub_read_bw_mte(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 128.0, 1.0, pmu, task_cyc, freq)

    @staticmethod
    def ub_write_bw_mte(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 128.0, 1.0, pmu, task_cyc, freq)

    @staticmethod
    def ub_read_bw_vector(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 128.0, 2.0, pmu, task_cyc, freq)

    @staticmethod
    def ub_write_bw_vector(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 128.0, 2.0, pmu, task_cyc, freq)

    @staticmethod
    def ub_read_bw_scalar(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 128.0, 1.0, pmu, task_cyc, freq)

    @staticmethod
    def ub_write_bw_scalar(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 128.0, 1.0, pmu, task_cyc, freq)

    @staticmethod
    def fixp2ub_write_bw(pmu, task_cyc, freq):
        return PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq)


class Constant:
    """
    constant class
    """

    TRACE_BACK_SWITCH = False
    L2_CACHE_EVENTS = [
        "0x59",
        "0x5b",
        "0x5c",
        "0x62",
        "0x6a",
        "0x6c",
        "0x71",
        "0x74",
        "0x77",
        "0x78",
        "0x79",
        "0x7c",
        "0x7d",
        "0x7e",
        "0xf6",
        "0xfb",
        "0xfc",
        "0x90",
        "0x91",
        "0x9c",
        "0x9d",
        "0xbf",
        "0x00",
        "0x88",
        "0x89",
        "0x8a",
        "0x74",
        "0x75",
        "0x97",
        "0x81",
        "0x82",
        "0x83",
    ]
    SOC_PMU_EVENTS = ["0x2", "0x8a", "0x8b", "0x8b", "0x8c", "0x8d"]

    INVALID_INDEX_DICT = {"coefficient": -1, "index": -1}

    AI_CORE_CALCULATION_FORMULA = {
        "mac_fp16_ratio": PmuCalculateFunc.mac_fp16_ratio,
        "mac_int8_ratio": PmuCalculateFunc.mac_int8_ratio,
        "vec_fp32_ratio": PmuCalculateFunc.vec_fp32_ratio,
        "vec_fp16_128lane_ratio": PmuCalculateFunc.vec_fp16_128lane_ratio,
        "vec_fp16_64lane_ratio": PmuCalculateFunc.vec_fp16_64lane_ratio,
        "vec_fp16_ratio": PmuCalculateFunc.vec_fp16_ratio,
        "vec_int32_ratio": PmuCalculateFunc.vec_int32_ratio,
        "vec_misc_ratio": PmuCalculateFunc.vec_misc_ratio,
        "vec_ratio": PmuCalculateFunc.vec_ratio,
        "vec_exe_ratio": PmuCalculateFunc.vec_ratio,
        "mac_ratio": PmuCalculateFunc.mac_ratio,
        "mac_ratio_extra": PmuCalculateFunc.mac_ratio,
        "mac_exe_ratio": PmuCalculateFunc.mac_ratio,
        "mac_fp_ratio": PmuCalculateFunc.mac_fp_ratio,
        "mac_int_ratio": PmuCalculateFunc.mac_int_ratio,
        "scalar_ratio": PmuCalculateFunc.scalar_ratio,
        "scalar_exe_ratio": PmuCalculateFunc.scalar_ratio,
        "fixpipe_ratio": PmuCalculateFunc.fixpipe_ratio,
        "fixpipe_exe_ratio": PmuCalculateFunc.fixpipe_ratio,
        "mte1_ratio": PmuCalculateFunc.mte1_ratio,
        "mte1_exe_ratio": PmuCalculateFunc.mte1_ratio,
        "mte1_ratio_extra": PmuCalculateFunc.mte1_ratio,
        "mte2_ratio": PmuCalculateFunc.mte2_ratio,
        "mte2_exe_ratio": PmuCalculateFunc.mte2_ratio,
        "mte3_ratio": PmuCalculateFunc.mte3_ratio,
        "mte3_exe_ratio": PmuCalculateFunc.mte3_ratio,
        "mte_preload_ratio": PmuCalculateFunc.mte_preload_ratio,
        "icache_miss_rate": PmuCalculateFunc.icache_miss_rate,
        "icache_req_ratio": PmuCalculateFunc.icache_req_ratio,
        "scalar_waitflag_ratio": PmuCalculateFunc.scalar_waitflag_ratio,
        "cube_waitflag_ratio": PmuCalculateFunc.cube_waitflag_ratio,
        "vector_waitflag_ratio": PmuCalculateFunc.vector_waitflag_ratio,
        "mte1_waitflag_ratio": PmuCalculateFunc.mte1_waitflag_ratio,
        "mte2_waitflag_ratio": PmuCalculateFunc.mte2_waitflag_ratio,
        "mte3_waitflag_ratio": PmuCalculateFunc.mte3_waitflag_ratio,
        "scalar_ld_ratio": PmuCalculateFunc.scalar_ld_ratio,
        "scalar_st_ratio": PmuCalculateFunc.scalar_st_ratio,
        "vec_bankgroup_cflt_ratio": PmuCalculateFunc.vec_bankgroup_cflt_ratio,
        "vec_bank_cflt_ratio": PmuCalculateFunc.vec_bank_cflt_ratio,
        "vec_resc_cflt_ratio": PmuCalculateFunc.vec_resc_cflt_ratio,
        "mte1_iq_full_ratio": PmuCalculateFunc.mte1_iq_full_ratio,
        "mte2_iq_full_ratio": PmuCalculateFunc.mte2_iq_full_ratio,
        "mte3_iq_full_ratio": PmuCalculateFunc.mte3_iq_full_ratio,
        "cube_iq_full_ratio": PmuCalculateFunc.cube_iq_full_ratio,
        "vec_iq_full_ratio": PmuCalculateFunc.vec_iq_full_ratio,
        "acess_stack_ratio": PmuCalculateFunc.acess_stack_ratio,
        "control_flow_prediction_ratio": PmuCalculateFunc.control_flow_prediction_ratio,
        "control_flow_mis_prediction_rate": PmuCalculateFunc.control_flow_mis_prediction_rate,
        "ub_read_bw(GB/s)": PmuCalculateFunc.ub_read_bw,
        "ub_write_bw(GB/s)": PmuCalculateFunc.ub_write_bw,
        "l1_read_bw(GB/s)": PmuCalculateFunc.l1_read_bw,
        "l1_write_bw(GB/s)": PmuCalculateFunc.l1_write_bw,
        "l2_read_bw(GB/s)": PmuCalculateFunc.l2_read_bw,
        "l2_write_bw(GB/s)": PmuCalculateFunc.l2_write_bw,
        "main_mem_read_bw(GB/s)": PmuCalculateFunc.main_mem_read_bw,
        "main_mem_write_bw(GB/s)": PmuCalculateFunc.main_mem_write_bw,
        "l0a_read_bw(GB/s)": PmuCalculateFunc.l0a_read_bw,
        "l0a_write_bw(GB/s)": PmuCalculateFunc.l0a_write_bw,
        "l0b_read_bw(GB/s)": PmuCalculateFunc.l0b_read_bw,
        "l0b_write_bw(GB/s)": PmuCalculateFunc.l0b_write_bw,
        "l0c_read_bw(GB/s)": PmuCalculateFunc.l0c_read_bw,
        "l0c_write_bw(GB/s)": PmuCalculateFunc.l0c_write_bw,
        "l0c_read_bw_cube(GB/s)": PmuCalculateFunc.l0c_read_bw_cube,
        "l0c_write_bw_cube(GB/s)": PmuCalculateFunc.l0c_write_bw_cube,
        "ub_read_bw_mte(GB/s)": PmuCalculateFunc.ub_read_bw_mte,
        "ub_write_bw_mte(GB/s)": PmuCalculateFunc.ub_write_bw_mte,
        "ub_read_bw_mte2(GB/s)": PmuCalculateFunc.ub_read_bw_mte,
        "ub_write_bw_mte2(GB/s)": PmuCalculateFunc.ub_write_bw_mte,
        "ub_read_bw_vector(GB/s)": PmuCalculateFunc.ub_read_bw_vector,
        "ub_write_bw_vector(GB/s)": PmuCalculateFunc.ub_write_bw_vector,
        "ub_read_bw_scalar(GB/s)": PmuCalculateFunc.ub_read_bw_scalar,
        "ub_write_bw_scalar(GB/s)": PmuCalculateFunc.ub_write_bw_scalar,
        "fixp2ub_write_bw(GB/s)": PmuCalculateFunc.fixp2ub_write_bw,
        "stu_pmu_wctl_ub_cflt": PmuCalculateFunc.stu_pmu_wctl_ub_cflt,
    }

    AICORE_PIPE_LIST = ["vec_time", "mac_time", "scalar_time", "mte1_time", "mte2_time", "mte3_time"]

    # add default limit for reader buffer size ->8196  * 1024 Byte
    MAX_READ_LINE_BYTES = 8196 * 1024
    MAX_READ_FILE_BYTES = 64 * 1024 * 1024
    MAX_READ_DB_FILE_BYTES = 8 * 1024 * 1024 * 1024
    MAX_HWTS_FILE_PARSE_BYTES = 650 * 1024 * 1024

    DEFAULT_START = 1

    PLATFORM_VERSION = "platform_version"
    CHIP_V1_1_0 = "0"
    CHIP_V2_1_0 = "1"
    CHIP_V3_1_0 = "2"
    CHIP_V3_2_0 = "3"
    CHIP_V3_3_0 = "4"
    CHIP_V4_1_0 = "5"
    CHIP_V1_1_1 = "7"
    CHIP_V1_1_2 = "8"
    CHIP_V5_1_0 = "9"
    CHIP_V1_1_3 = "11"
    CHIP_V6_1_0 = "15"
    CHIP_V6_2_0 = "16"

    MIX_OP_AND_GRAPH = "mix_operator_and_graph"
    STEP_INFO = "step_info"
    TRAIN = "train"
    SINGLE_OP = "single_op"
    DONE_TAG = ".done"
    ZIP_TAG = ".zip"
    COMPLETE_TAG = ".complete"
    FOLDER_MASK = 0o750
    LINE_LEN = 3
    SAMPLE_FILE = "sample.json"
    DEFAULT_REPLAY = 0
    DEFAULT_DEVICE = 0
    DEFAULT_COUNT = 0
    KILOBYTE = 1024.0
    TIME_RATE = 1000000000.0
    THOUSAND = 1000
    BYTE_NS_TO_MB_S = TIME_RATE / KILOBYTE / KILOBYTE
    BYTE_US_TO_MB_S = TIME_RATE / THOUSAND / KILOBYTE / KILOBYTE
    MILLION = 1000000
    BIT_TO_BYTE = 8
    MBPS_TO_BYTES = MILLION // BIT_TO_BYTE
    L2_CACHE_ITEM = 8
    SOC_PMU_ITEM = 8
    HEX_NUMBER = 16
    DVPP_TYPE_NAME = ['VDEC', 'JPEGD', 'PNGD', 'JPEGE', 'VPC']
    FILTER_DIRS = [
        ".profiler",
        "HCCL_PROF",
        "timeline",
        "query",
        'sqlite',
        'log',
        'analyze',
        'mindstudio_profiler_log',
        'mindstudio_profiler_output',
    ]
    NA = 'N/A'
    TASK_TYPE_OTHER = "Other"
    TASK_TYPE_AI_CORE = "AI_CORE"
    TASK_TYPE_AI_CPU = "AI_CPU"
    TASK_TYPE_AIV = "AI_VECTOR_CORE"
    TASK_TYPE_MIX_AIV = "MIX_AIV"
    TASK_TYPE_MIX_AIC = "MIX_AIC"
    TASK_TYPE_FFTS_PLUS = "FFTS_PLUS"
    DATA_PROCESS_AI_CPU = "AICPU"
    TASK_TYPE_WRITE_BACK = "WRITE_BACK"
    TASK_TYPE_INVALID = "INVALID"
    TASK_TYPE_HCCL = "HCCL"
    TASK_TYPE_COMMUNICATION = "COMMUNICATION"
    TASK_TYPE_HCCL_AI_CPU = "HCCL_AI_CPU"
    TASK_TYPE_DSA = "DSA_SQE"
    TASK_TYPE_DVPP = "DVPP"
    TASK_TYPE_UNKNOWN = "UNKNOWN"
    DATA_PROCESS_DP = "DP"
    DATA_QUEUE = "AICPUMI"
    TASK_TYPE = 'task_type'
    WRITE_MODES = stat.S_IWUSR | stat.S_IRUSR | stat.S_IRGRP
    WRITE_FLAGS = os.O_WRONLY | os.O_CREAT | os.O_TRUNC

    # hash dict flag
    NO_HASH_DICT_FLAG = 1
    HASH_DICT_FLAG = 0

    # default value
    DEFAULT_VALUE = 0
    DEFAULT_INVALID_VALUE = -1
    DEFAULT_TURE_VALUE = 1
    DEFAULT_FALSE_VALUE = 0

    # ge timeline
    GE_TIMELINE_MODEL_ID_INDEX = 0
    GE_TIMELINE_MODEL_ID_INDEX_NAME_INDEX = 6
    GE_TIMELINE_TID_INDEX = 8

    GE_OP_MODEL_ID = 4294967295
    GE_STATIC_SHAPE = 1
    GE_DYNAMIC_SHAPE = 0

    # hccl
    TYPE_RDMA = "RDMA"
    TYPE_SDMA = "SDMA"
    ILLEGAL_RANK = 4294967295
    LINK_TYPE_LIST = [TYPE_SDMA, TYPE_RDMA]

    # Host
    RECOMMEND_PERCENTILE = 0.98
    RATIO_FOR_BEST_PERFORMANCE = 0.8

    ROOT_LEVEL = 0
    ACL_LEVEL = 1
    MODEL_LEVEL = 2
    NODE_LEVEL = 3
    HCCL_LEVEL = 4  # ge node sub level
    TASK_LEVEL = 5

    # Host Task Type
    PROFILING_ENABLE = "PROFILING_ENABLE"
    PROFILING_DISABLE = "PROFILING_DISABLE"
    KERNEL_FFTS_PLUS_TASK_TYPE = "FFTS_PLUS"
    KERNEL_AICPU = "KERNEL_AICPU"
    KERNEL_AICORE = "KERNEL_AICORE"
    KERNEL_AIVEC = "KERNEL_AIVEC"
    KERNEL_MIX_AIC = "KERNEL_MIX_AIC"
    KERNEL_MIX_AIV = "KERNEL_MIX_AIV"
    COMP_TASK_TYPE = [KERNEL_AICORE, KERNEL_AIVEC, KERNEL_FFTS_PLUS_TASK_TYPE, KERNEL_MIX_AIC, KERNEL_MIX_AIV]

    # data type
    UINT16_MAX = 65535
    UINT32_MAX = 4294967295

    def get_constant_class_name(self: any) -> any:
        """
        get constant class name
        """
        return self.__class__.__name__

    def get_constant_class_member(self: any) -> any:
        """
        get constant class member num
        """
        return self.__dict__
