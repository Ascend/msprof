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


class DataParsersConfig(MetaConfig):
    DATA = {
        'CaptureStreamInfoParser': [
            ('path', 'msparser.compact_info.capture_stream_info_parser'),
            ('chip_model', '2,3,4,5,15,16'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'GeLogicStreamParser': [
            ('path', 'msparser.ge.ge_logic_stream_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11'),
            ('position', 'H'),
        ],
        'ParsingRuntimeData': [
            ('path', 'common_func.create_runtime_db'),
            ('chip_model', '0,1,2,3,4,5,7,8,11'),
            ('position', 'D'),
        ],
        'L2CacheParser': [
            ('path', 'msparser.l2_cache.l2_cache_parser'),
            ('chip_model', '1,2,3,4,5,7,8,11,15,16'),
            ('position', 'D'),
        ],
        'TsTimelineRecParser': [
            ('path', 'msparser.iter_rec.ts_timeline_parser'),
            ('chip_model', '0'),
            ('level', '3'),
            ('position', 'D'),
        ],
        'ParsingDDRData': [
            ('path', 'msparser.hardware.ddr_parser'),
            ('chip_model', '0,1,2,3,4,7,8,11'),
            ('position', 'D'),
        ],
        'ParsingPeripheralData': [
            ('path', 'msparser.hardware.dvpp_parser'),
            ('chip_model', '0,1,5,7,8,11,15,16'),
            ('position', 'D'),
        ],
        'ParsingNicData': [('path', 'msparser.hardware.nic_parser'), ('chip_model', '0,1,5,7,8,11'), ('position', 'D')],
        'ParsingRoceData': [('path', 'msparser.hardware.roce_parser'), ('chip_model', '0,1,5'), ('position', 'D')],
        'ParsingTSData': [
            ('path', 'msparser.hardware.tscpu_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11'),
            ('position', 'D'),
        ],
        'ParsingAICPUData': [
            ('path', 'msparser.hardware.ai_cpu_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('position', 'D'),
        ],
        'ParsingCtrlCPUData': [
            ('path', 'msparser.hardware.ctrl_cpu_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('position', 'D'),
        ],
        'ParsingMemoryData': [('path', 'msparser.hardware.sys_mem_parser'), ('chip_model', '0,1,2,3,4,5,7,8,11,15,16')],
        'ParsingCpuUsageData': [
            ('path', 'msparser.hardware.sys_usage_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
        ],
        'ParsingPcieData': [
            ('path', 'msparser.hardware.pcie_parser'),
            ('chip_model', '0,1,2,3,4,5,15,16'),
            ('position', 'D'),
        ],
        'ParsingHBMData': [
            ('path', 'msparser.hardware.hbm_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('position', 'D'),
        ],
        'ParsingQosData': [('path', 'msparser.hardware.qos_parser'), ('chip_model', '5'), ('position', 'D')],
        'NonMiniLLCParser': [
            ('path', 'msparser.hardware.llc_parser'),
            ('chip_model', '1,2,3,4,5,7,8,11,15,16'),
            ('position', 'D'),
        ],
        'MiniLLCParser': [('path', 'msparser.hardware.mini_llc_parser'), ('chip_model', '0'), ('position', 'D')],
        'ParsingHCCSData': [
            ('path', 'msparser.hardware.hccs_parser'),
            ('chip_model', '0,1,2,3,4,5'),
            ('position', 'D'),
        ],
        'ParsingUBData': [('path', 'msparser.hardware.ub_parser'), ('chip_model', '15,16'), ('position', 'D')],
        'TstrackParser': [
            ('path', 'msparser.step_trace.ts_track_parser'),
            ('chip_model', '1,2,3,4,5,7,8,11,15,16'),
            ('level', '2'),
            ('position', 'D'),
        ],
        'ParsingAICoreSampleData': [
            ('path', 'msparser.aic_sample.ai_core_sample_parser'),
            ('chip_model', '0,1,2,3,4'),
            ('position', 'D'),
        ],
        'ParsingAIVectorCoreSampleData': [
            ('path', 'msparser.aic_sample.ai_core_sample_parser'),
            ('chip_model', '2,3,4'),
            ('position', 'D'),
        ],
        'MsprofTxParser': [
            ('path', 'msparser.msproftx.msproftx_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('level', '4'),
            ('position', 'H'),
        ],
        'AicpuBinDataParser': [
            ('path', 'msparser.aicpu.aicpu_bin_data_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11'),
            ('level', '3'),
            ('position', 'D'),
        ],
        'ParsingFftsAICoreSampleData': [
            ('path', 'msparser.aic_sample.ai_core_sample_parser'),
            ('chip_model', '5,7,8,11,15,16'),
            ('position', 'D'),
        ],
        'BiuPerfParser': [('path', 'msparser.biu_perf.biu_perf_parser'), ('chip_model', '5,7,8,11'), ('position', 'D')],
        'SocProfilerParser': [
            ('path', 'msparser.stars.soc_profiler_parser'),
            ('chip_model', '5,7,8,11,15,16'),
            ('position', 'D'),
        ],
        'MsTimeParser': [('path', 'msparser.ms_timer.ms_time_parser'), ('chip_model', '0,1,2,3,4,5')],
        'DataPreparationParser': [('path', 'msparser.aicpu.data_preparation_parser'), ('chip_model', '0,1,2,3,4,5')],
        'HCCLOperatorParser': [
            ('path', 'msparser.parallel.hccl_operator_parser'),
            ('chip_model', '1,2,3,4,5,15,16'),
            ('level', '3'),
        ],
        'ParallelStrategyParser': [
            ('path', 'msparser.parallel.parallel_strategy_parser'),
            ('chip_model', '1,2,3,4,5'),
            ('position', 'D'),
        ],
        'ParallelParser': [
            ('path', 'msparser.parallel.parallel_parser'),
            ('chip_model', '1,2,3,4,5'),
            ('level', '4'),
            ('position', 'D'),
        ],
        'NpuMemParser': [
            ('path', 'msparser.npu_mem.npu_mem_parser'),
            ('chip_model', '0,1,3,4,5,7,8,11,15,16'),
            ('position', 'D'),
        ],
        'NpuModuleMemParser': [
            ('path', 'msparser.npu_mem.npu_module_mem_parser'),
            ('chip_model', '1,2,3,4,5,7,8,11,15,16'),
            ('position', 'D'),
        ],
        'NpuOpMemParser': [
            ('path', 'msparser.npu_mem.npu_op_mem_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('position', 'H'),
        ],
        'FreqParser': [('path', 'msparser.freq.freq_parser'), ('chip_model', '5,7,15,16'), ('position', 'D')],
        'ApiEventParser': [
            ('path', 'msparser.api_event.api_event_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'HashDicParser': [
            ('path', 'msparser.hash_dic.hash_dic_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,9,11,15,16'),
            ('position', 'H'),
        ],
        'TaskTrackParser': [
            ('path', 'msparser.compact_info.task_track_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'MemcpyInfoParser': [
            ('path', 'msparser.compact_info.memcpy_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'HcclInfoParser': [
            ('path', 'msparser.add_info.hccl_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'MultiThreadParser': [
            ('path', 'msparser.add_info.multi_thread_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'AicpuAddInfoParser': [
            ('path', 'msparser.add_info.aicpu_add_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('position', 'D'),
        ],
        'TensorAddInfoParser': [
            ('path', 'msparser.add_info.tensor_add_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'FusionAddInfoParser': [
            ('path', 'msparser.add_info.fusion_add_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'GraphAddInfoParser': [
            ('path', 'msparser.add_info.graph_add_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,9,11,15,16'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'NodeBasicInfoParser': [
            ('path', 'msparser.compact_info.node_basic_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'NodeAttrInfoParser': [
            ('path', 'msparser.compact_info.node_attr_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'MemoryApplicationParser': [
            ('path', 'msparser.add_info.memory_application_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'StaticOpMemParser': [
            ('path', 'msparser.add_info.static_op_mem_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('level', '4'),
            ('position', 'H'),
        ],
        'CtxIdParser': [
            ('path', 'msparser.add_info.ctx_id_parser'),
            ('chip_model', '5,7,8,11'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'CANNCalculator': [
            ('path', 'mscalculate.cann.cann_calculator'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('level', '3'),
            ('position', 'H'),
        ],
        'IterRecParser': [
            ('path', 'msparser.iter_rec.iter_rec_parser'),
            ('chip_model', '1,2,3,4'),
            ('level', '4'),
            ('position', 'D'),
        ],
        'NoGeIterRecParser': [
            ('path', 'msparser.iter_rec.iter_rec_parser'),
            ('chip_model', '1,2,3,4'),
            ('level', '4'),
            ('position', 'D'),
        ],
        'V5StarsParser': [
            ('path', 'msparser.v5.v5_stars_parser'),
            ('chip_model', '9'),
            ('level', '2'),
            ('position', 'D'),
        ],
        'V5DbgParser': [('path', 'msparser.v5.v5_dbg_parser'), ('chip_model', '9'), ('level', '3'), ('position', 'H')],
        'HcclOpInfoParser': [
            ('path', 'msparser.compact_info.hccl_op_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11,15,16'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'Mc2CommInfoParser': [
            ('path', 'msparser.add_info.mc2_comm_info_parser'),
            ('chip_model', '4,5,15,16'),
            ('position', 'H'),
        ],
        'NetDevStatsParser': [
            ('path', 'msparser.hardware.netdev_stats_parser'),
            ('chip_model', '0,1,2,3,4,5,7,8,11'),
            ('position', 'D'),
        ],
        'CCUMissionParser': [
            ('path', 'msparser.hardware.ccu_mission_parser'),
            ('chip_model', '15,16'),
            ('position', 'D'),
        ],
        'CCUChannelParser': [
            ('path', 'msparser.hardware.ccu_channel_parser'),
            ('chip_model', '15,16'),
            ('position', 'D'),
        ],
        'CCUAddInfoParser': [
            ('path', 'msparser.add_info.ccu_add_info_parser'),
            ('chip_model', '15,16'),
            ('position', 'H'),
        ],
        'BiuPerfChip6Parser': [
            ('path', 'msparser.biu_perf.biu_perf_chip6_parser'),
            ('chip_model', '15,16'),
            ('position', 'D'),
        ],
        'SocPmuParser': [('path', 'msparser.l2_cache.soc_pmu_parser'), ('chip_model', '5,15,16'), ('position', 'D')],
        'LpmInfoConvParser': [('path', 'msparser.lpm_info.lpm_info_parser'), ('chip_model', '5'), ('position', 'D')],
        'StreamExpandSpecParser': [
            ('path', 'msparser.compact_info.stream_expand_spec_parser'),
            ('chip_model', '5'),
            ('position', 'H'),
        ],
        'RuntimeOpInfoParser': [
            ('path', 'msparser.add_info.runtime_op_info_parser'),
            ('chip_model', '2,3,4,5,15,16'),
            ('level', '2'),
            ('position', 'H'),
        ],
        'DPUHcclInfoParser': [
            ('path', 'msparser.add_info.dpu_hccl_track_parser'),
            ('chip_model', '15,16'),
            ('level', '4'),
            ('position', 'H'),
        ],
    }
