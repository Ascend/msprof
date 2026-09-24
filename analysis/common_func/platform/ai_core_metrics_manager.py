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
from profiling_bean.prof_enum.chip_model import ChipModel


class AiCoreMetricsManager:
    """
    class for aicore metrics configure manage
    """

    PMU_PIPE = 'PipeUtilization'
    PMU_PIPE_EXCT = 'PipeUtilizationExct'
    PMU_PIPE_EXECUT = 'PipelineExecuteUtilization'
    PMU_ARITH = 'ArithmeticUtilization'
    PMU_MEM = 'Memory'
    PMU_MEM_L0 = 'MemoryL0'
    PMU_RESOURCE = 'ResourceConflictRatio'
    PMU_MEM_UB = 'MemoryUB'
    PMU_L2_CACHE = 'L2Cache'
    PMU_MEM_ACCESS = 'MemoryAccess'
    PMU_PIPE_STALL_CYCLE = 'PipeStallCycle'
    PMU_SCALAR_RATIO = 'ScalarRatio'

    AICORE_METRICS_LIST_DEFAULT = {
        PMU_PIPE_EXCT: "mac_ratio_extra,scalar_ratio,mte1_ratio_extra,mte2_ratio,fixpipe_ratio,icache_miss_rate",
        PMU_PIPE_EXECUT: "vec_exe_ratio,mac_exe_ratio,scalar_exe_ratio,mte1_exe_ratio,"
        "mte2_exe_ratio,mte3_exe_ratio,fixpipe_exe_ratio",
        PMU_ARITH: "mac_fp16_ratio,mac_int8_ratio,vec_fp32_ratio,vec_fp16_ratio,vec_int32_ratio,vec_misc_ratio",
        PMU_PIPE: "vec_ratio,mac_ratio,scalar_ratio,mte1_ratio,mte2_ratio,mte3_ratio,icache_miss_rate",
        PMU_MEM: "ub_read_bw(GB/s),ub_write_bw(GB/s),l1_read_bw(GB/s),"
        "l1_write_bw(GB/s),l2_read_bw(GB/s),l2_write_bw(GB/s),"
        "main_mem_read_bw(GB/s),main_mem_write_bw(GB/s)",
        PMU_MEM_L0: "l0a_read_bw(GB/s),l0a_write_bw(GB/s),l0b_read_bw(GB/s),"
        "l0b_write_bw(GB/s),l0c_read_bw(GB/s),l0c_write_bw(GB/s),"
        "l0c_read_bw_cube(GB/s),l0c_write_bw_cube(GB/s)",
        PMU_RESOURCE: "vec_bankgroup_cflt_ratio,vec_bank_cflt_ratio,vec_resc_cflt_ratio",
        PMU_MEM_UB: "ub_read_bw_mte(GB/s),ub_write_bw_mte(GB/s),"
        "ub_read_bw_vector(GB/s),ub_write_bw_vector(GB/s),"
        "ub_read_bw_scalar(GB/s),ub_write_bw_scalar(GB/s)",
        PMU_L2_CACHE: "write_cache_hit,write_cache_miss_allocate,"
        "r0_read_cache_hit,r0_read_cache_miss_allocate,"
        "r1_read_cache_hit,r1_read_cache_miss_allocate",
        PMU_MEM_ACCESS: "read_main_memory_datas(KB),write_main_memory_datas(KB),gm_to_l1_datas(KB),"
        "l0c_to_l1_datas(KB),l0c_to_gm_datas(KB),gm_to_ub_datas(KB),ub_to_gm_datas(KB)",
    }

    AICORE_METRICS_LIST = AICORE_METRICS_LIST_DEFAULT

    AICORE_METRICS_CHIP_V5 = {
        PMU_PIPE: "vec_exe_ratio,mac_exe_ratio,scalar_ratio,mte1_ratio,mte2_ratio,"
        "mte3_ratio,fixpipe_ratio,mte_preload_ratio,icache_miss_rate",
        PMU_MEM: "main_mem_read_bw(GB/s),main_mem_write_bw(GB/s)",
        PMU_MEM_UB: "ub_read_bw_mte2(GB/s),ub_write_bw_mte2(GB/s),"
        "ub_read_bw_mte(GB/s),ub_write_bw_mte(GB/s),"
        "ub_read_bw_vector(GB/s),ub_write_bw_vector(GB/s),"
        "ub_read_bw_scalar(GB/s),ub_write_bw_scalar(GB/s)",
        PMU_PIPE_STALL_CYCLE: "cube_self_stall_cycles,vec_self_stall_cycles,"
        "cube_mte2_cflt_stall_cycles,cube_mte3_cflt_stall_cycles,"
        "cube_mte1_cflt_stall_cycles,vec_mte2_cflt_stall_cycles,"
        "vec_mte3_cflt_stall_cycles,vec_mte1_cflt_stall_cycles,"
        "mte1_mte2_stall_cycles,mte1_mte3_stall_cycles",
        PMU_SCALAR_RATIO: "acess_stack_ratio,control_flow_mis_prediction_rate",
    }

    AICORE_METRICS_CHIP_V6 = {
        PMU_PIPE: "vec_ratio,mac_ratio,scalar_ratio,mte1_ratio,mte2_ratio,mte3_ratio,fixpipe_ratio,icache_miss_rate",
        PMU_MEM: "ub_read_bw(GB/s),ub_write_bw(GB/s),l1_read_bw(GB/s),"
        "l1_write_bw(GB/s),main_mem_read_bw(GB/s),main_mem_write_bw(GB/s)",
        PMU_MEM_L0: "l0a_read_bw(GB/s),l0a_write_bw(GB/s),l0b_read_bw(GB/s),l0b_write_bw(GB/s),"
        "l0c_read_bw(GB/s),l0c_read_bw_cube(GB/s),l0c_write_bw_cube(GB/s)",
        PMU_MEM_UB: "ub_read_bw_mte(GB/s),ub_write_bw_mte(GB/s),"
        "ub_read_bw_vector(GB/s),ub_write_bw_vector(GB/s),"
        "ub_read_bw_scalar(GB/s),ub_write_bw_scalar(GB/s),fixp2ub_write_bw(GB/s)",
        PMU_ARITH: "mac_fp16_ratio,mac_int8_ratio",
        PMU_L2_CACHE: "read_local_l2_hit,read_local_l2_miss,"
        "read_local_l2_victim,write_local_l2_hit,"
        "write_local_l2_miss,write_local_l2_victim",
        PMU_RESOURCE: "vec_bank_cflt_ratio,vec_resc_cflt_ratio",
    }

    # set AICORE_METRICS_LIST by chip_id
    AICORE_METRICS_MAP = {
        ChipModel.CHIP_V5_1_0: AICORE_METRICS_CHIP_V5,
        ChipModel.CHIP_V6_1_0: AICORE_METRICS_CHIP_V6,
        ChipModel.CHIP_V6_1_1: AICORE_METRICS_CHIP_V6,
        ChipModel.CHIP_V6_2_0: AICORE_METRICS_CHIP_V6,
    }

    VALID_METRICS_SET_DEFAULT = {
        'mac_exe_ratio',
        'vec_exe_ratio',
        'vec_ratio',
        'mte2_ratio',
        'scalar_exe_ratio',
        'mac_ratio_extra',
        'mte1_ratio_extra',
        'mte1_exe_ratio',
        'fixpipe_exe_ratio',
        'mte2_exe_ratio',
        'mte3_exe_ratio',
        'scalar_ratio',
        'fixpipe_ratio',
        'mte1_ratio',
        'mte3_ratio',
        'mac_ratio',
    }

    VALID_METRICS_SET = VALID_METRICS_SET_DEFAULT

    VALID_METRICS_MAP = {
        ChipModel.CHIP_V5_1_0: {
            'acess_stack_ratio',
            'fixpipe_ratio',
            'mac_exe_ratio',
            'mte1_ratio',
            'mte2_ratio',
            'mte3_ratio',
            'mte_preload_ratio',
            'scalar_ratio',
            'vec_exe_ratio',
        },
        ChipModel.CHIP_V6_1_0: {
            'mac_ratio',
            'mte1_ratio',
            'mte2_ratio',
            'mte3_ratio',
            'scalar_ratio',
            'vec_ratio',
            'fixpipe_ratio',
        },
        ChipModel.CHIP_V6_1_1: {
            'mac_ratio',
            'mte1_ratio',
            'mte2_ratio',
            'mte3_ratio',
            'scalar_ratio',
            'vec_ratio',
            'fixpipe_ratio',
        },
        ChipModel.CHIP_V6_2_0: {
            'mac_ratio',
            'mte1_ratio',
            'mte2_ratio',
            'mte3_ratio',
            'scalar_ratio',
            'vec_ratio',
            'fixpipe_ratio',
        },
    }

    @classmethod
    def set_aicore_metrics_list(cls, chip_id):
        cls.AICORE_METRICS_LIST = cls.AICORE_METRICS_MAP.get(chip_id, cls.AICORE_METRICS_LIST_DEFAULT)
        cls.VALID_METRICS_SET = cls.VALID_METRICS_MAP.get(chip_id, cls.VALID_METRICS_SET_DEFAULT)
