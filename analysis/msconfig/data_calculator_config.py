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

from msconfig.meta_config import MetaConfig


class DataCalculatorConfig(MetaConfig):
    DATA = {
        'BlockNumCalculator': [
            ('path', 'mscalculate.tiling_block_num.block_num_calculator'),
            ('chip_model', '1,2,3,4,5,7,15,16'),
            ('position', 'D'),
        ],
        'SubTaskCalculator': [
            ('path', 'mscalculate.stars.sub_task_calculator'),
            ('chip_model', '5,15,16'),
            ('level', '5'),
            ('position', 'D'),
        ],
        'L2CacheCalculator': [
            ('path', 'mscalculate.l2_cache.l2_cache_calculator'),
            ('chip_model', '1,2,3,4,5,7,8,11,15,16'),
            ('position', 'D'),
        ],
        'HwtsCalculator': [
            ('path', 'mscalculate.hwts.hwts_calculator'),
            ('chip_model', '1,2,3,4'),
            ('level', '3'),
            ('position', 'D'),
        ],
        'HwtsAivCalculator': [
            ('path', 'mscalculate.hwts.hwts_aiv_calculator'),
            ('chip_model', '2,3,4'),
            ('level', '3'),
            ('position', 'D'),
        ],
        'AicCalculator': [
            ('path', 'mscalculate.aic.aic_calculator'),
            ('chip_model', '1,2,3,4'),
            ('level', '3'),
            ('position', 'D'),
        ],
        'V5AicCalculator': [
            ('path', 'mscalculate.aic.aic_calculator'),
            ('chip_model', '9'),
            ('level', '3'),
            ('position', 'D'),
        ],
        'MiniAicCalculator': [('path', 'mscalculate.aic.mini_aic_calculator'), ('chip_model', '0'), ('position', 'D')],
        'AivCalculator': [
            ('path', 'mscalculate.aic.aiv_calculator'),
            ('chip_model', '2,3,4'),
            ('level', '3'),
            ('position', 'D'),
        ],
        'MemcpyCalculator': [
            ('path', 'mscalculate.memory_copy.memcpy_calculator'),
            ('chip_model', '0,1,2,3,4,5'),
            ('position', 'D'),
        ],
        'StarsLogCalCulator': [
            ('path', 'msparser.stars.stars_log_parser'),
            ('chip_model', '5,7,8,11,15,16'),
            ('level', '3'),
            ('position', 'D'),
        ],
        'BiuPerfCalculator': [
            ('path', 'mscalculate.biu_perf.biu_perf_calculator'),
            ('chip_model', '5,7,8,11'),
            ('position', 'D'),
        ],
        'FftsPmuCalculator': [
            ('path', 'mscalculate.stars.ffts_pmu_calculator'),
            ('chip_model', '5,7,8,11'),
            ('level', '3'),
            ('position', 'D'),
        ],
        'Chip6PmuCalculator': [
            ('path', 'mscalculate.stars.chip6_pmu_calculator'),
            ('chip_model', '15,16'),
            ('level', '3'),
            ('position', 'D'),
        ],
        'AICpuFromTsCalculator': [
            ('path', 'mscalculate.ts_task.ai_cpu.aicpu_from_ts'),
            ('chip_model', '1'),
            ('position', 'D'),
        ],
        'TaskSchedulerCalculator': [
            ('path', 'mscalculate.data_analysis.task_scheduler_calculator'),
            ('chip_model', '0'),
            ('level', '5'),
            ('position', 'D'),
        ],
        'OpTaskSchedulerCalculator': [
            ('path', 'mscalculate.data_analysis.op_task_scheduler_calculator'),
            ('chip_model', '0'),
            ('level', '5'),
            ('position', 'D'),
        ],
        'ParseAiCoreOpSummaryCalculator': [
            ('path', 'mscalculate.data_analysis.parse_aicore_op_summary_calculator'),
            ('chip_model', '0,1,2,3,4,5,7,8,9,11,15,16'),
            ('level', '14'),
            ('position', 'D'),
        ],
        'OpSummaryOpSceneCalculator': [
            ('path', 'mscalculate.data_analysis.op_summary_op_scene_calculator'),
            ('chip_model', '0,1,2,3,4,5,7,8,9,11,15,16'),
            ('level', '14'),
            ('position', 'D'),
        ],
        'MergeOpCounterCalculator': [
            ('path', 'mscalculate.data_analysis.merge_op_counter_calculator'),
            ('chip_model', '0,1,2,3,4,5,7,8,9,11,15,16'),
            ('level', '14'),
            ('position', 'D'),
        ],
        'OpCounterOpSceneCalculator': [
            ('path', 'mscalculate.data_analysis.op_counter_op_scene_calculator'),
            ('chip_model', '0,1,2,3,4,5,7,8,9,11,15,16'),
            ('level', '14'),
            ('position', 'D'),
        ],
        'AscendTaskCalculator': [
            ('path', 'mscalculate.ascend_task.ascend_task_calculator'),
            ('chip_model', '0,1,2,3,4,5,7,8,9,11,15,16'),
            ('level', '6'),
            ('position', 'D'),
        ],
        'HcclCalculator': [
            ('path', 'mscalculate.hccl.hccl_calculator'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('level', '7'),
            ('position', 'D'),
        ],
        'NpuOpMemCalculator': [
            ('path', 'mscalculate.npu_mem.npu_op_mem_calculator'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('position', 'H'),
        ],
        'StarsIterRecCalculator': [
            ('path', 'mscalculate.stars.stars_iter_rec_calculator'),
            ('chip_model', '5,7,8,11'),
            ('level', '1'),
            ('position', 'D'),
        ],
        'KfcCalculator': [
            ('path', 'mscalculate.hccl.kfc_calculator'),
            ('chip_model', '4,5,15,16'),
            ('level', '7'),
            ('position', 'D'),
        ],
        'SocPmuCalculator': [
            ('path', 'mscalculate.l2_cache.soc_pmu_calculator'),
            ('chip_model', '5,15,16'),
            ('position', 'D'),
        ],
    }
