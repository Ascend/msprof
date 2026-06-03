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
from common_func.db_name_constant import DBNameConstant
from msconfig.meta_config import MetaConfig


class MsProfExportDataConfig(MetaConfig):
    DATA = {
        'task_time': [
            ('handler', '_get_task_time_data'),
            (
                'headers',
                'Time(%),Time(us),Count,Avg(us),Min(us),Max(us),Waiting(us),Running(us),'
                'Pending(us),Type,API,Task ID,Op Name,Stream ID',
            ),
            ('table', DBNameConstant.TABLE_RUNTIME_REPORT_TASK),
            ('db', DBNameConstant.DB_RUNTIME),
        ],
        'ddr': [
            ('handler', '_get_ddr_data'),
            ('headers', 'Metric,Read(MB/s),Write(MB/s)'),
            ('db', DBNameConstant.DB_DDR),
        ],
        'cpu_usage': [
            ('handler', '_get_cpu_usage_data'),
            ('headers', 'Cpu Type,User(%),Sys(%),IoWait(%),Irq(%),Soft(%),Idle(%)'),
            ('table', DBNameConstant.TABLE_SYS_USAGE),
        ],
        'process_cpu_usage': [
            ('handler', '_get_cpu_usage_data'),
            ('headers', 'PID,Name,CPU(%)'),
            ('table', DBNameConstant.TABLE_PID_USAGE),
        ],
        'sys_mem': [
            ('handler', '_get_memory_data'),
            (
                'headers',
                'Memory Total,Memory Free,Buffers,Cached,Share Memory,Commit Limit,'
                'Committed AS,Huge Pages Total(pages),Huge Pages Free(pages)',
            ),
            ('table', DBNameConstant.TABLE_SYS_MEM),
        ],
        'process_mem': [
            ('handler', '_get_memory_data'),
            ('headers', 'PID,Name,Size(pages),Resident(pages),Shared(pages)'),
            ('table', DBNameConstant.TABLE_PID_MEM),
        ],
        'op_summary': [
            ('handler', '_get_op_summary_data'),
            (
                'headers',
                'Model Name,Model ID,Task ID,Stream ID,Infer ID,Op Name,OP Type,OP State,Task Type,'
                'Task Start Time(us),Task Duration(us),Task Wait Time(us),Block Num,Mix Block Num,HF32 Eligible',
            ),
        ],
        'l2_cache': [
            ('handler', '_get_l2_cache_data'),
            ('db', DBNameConstant.DB_L2CACHE),
            ('table', DBNameConstant.TABLE_L2CACHE_SUMMARY),
            ('unused_cols', 'device_id,task_type'),
        ],
        'step_trace': [
            ('handler', '_get_step_trace_data'),
            (
                'headers',
                'Iteration ID,FP Start(us),BP End(us),Iteration End(us),Iteration Time(us),FP to BP Time(us),'
                'Iteration Refresh(us),Data Aug Bound(us)',
            ),
        ],
        'aicpu': [
            ('handler', '_get_aicpu_data'),
            (
                'headers',
                'Timestamp(us),Node,Compute_time(us),Memcpy_time(us),Task_time(us),'
                'Dispatch_time(us),Total_time(us),Stream ID,Task ID',
            ),
        ],
        'dp': [('handler', '_get_dp_data'), ('headers', 'Timestamp(us),Action,Source,Cached Buffer Size')],
        'op_statistic': [
            ('handler', '_get_op_statistic_data'),
            (
                'headers',
                'Model Name,OP Type,Core Type,Count,Total Time(us),Min Time(us),Avg Time(us),Max Time(us),Ratio(%)',
            ),
            ('db', DBNameConstant.DB_OP_COUNTER),
        ],
        'hbm': [('handler', '_get_hbm_data'), ('headers', 'Metric,Read(MB/s),Write(MB/s)')],
        'pcie': [('handler', '_get_pcie_data')],
        'fusion_op': [
            ('handler', '_get_fusion_op_data'),
            (
                'headers',
                'Model Name,Model ID,Fusion Op,Original Ops,Memory Input(KB),Memory Output(KB),'
                'Memory Weight(KB),Memory Workspace(KB),Memory Total(KB)',
            ),
            ('db', DBNameConstant.DB_GE_MODEL_INFO),
            ('table', DBNameConstant.TABLE_MODEL_NAME),
        ],
        'ai_core_utilization': [('handler', '_get_ai_core_sample_based_data'), ('db', DBNameConstant.DB_NAME_AICORE)],
        'ai_vector_core_utilization': [
            ('handler', '_get_aiv_sample_based_data'),
            ('db', DBNameConstant.DB_NAME_AI_VECTOR_CORE),
        ],
        'hccs': [('handler', '_get_hccs_data')],
        'llc_bandwidth': [('handler', '_get_llc_data')],
        'llc_aicpu': [('handler', '_get_llc_data')],
        'llc_ctrlcpu': [('handler', '_get_llc_data')],
        'llc_read_write': [('handler', '_get_llc_data')],
        'dvpp': [
            ('handler', '_get_dvpp_data'),
            ('db', DBNameConstant.DB_PERIPHERAL),
            ('table', DBNameConstant.TABLE_DVPP_REPORT),
            ('headers', 'Dvpp Id,Engine Type,Engine ID,All Time(us),All Frame,All Utilization(%)'),
        ],
        'nic': [
            ('handler', '_get_nic_data'),
            ('db', DBNameConstant.DB_NIC_ORIGIN),
            ('table', DBNameConstant.TABLE_NIC_REPORT),
            (
                'headers',
                'Timestamp(us),Bandwidth(MB/s),Rx Bandwidth efficiency(%),rxPacket/s,rxError rate(%),'
                'rxDropped rate(%),Tx Bandwidth efficiency(%),txPacket/s,txError rate(%),txDropped rate(%),funcId',
            ),
        ],
        'roce': [
            ('handler', '_get_roce_data'),
            ('db', DBNameConstant.DB_ROCE_ORIGIN),
            ('table', DBNameConstant.TABLE_ROCE_REPORT),
            (
                'headers',
                'Timestamp(us),Bandwidth(MB/s),Rx Bandwidth efficiency(%),rxPacket/s,rxError rate(%),'
                'rxDropped rate(%),Tx Bandwidth efficiency(%),txPacket/s,txError rate(%),txDropped rate(%),funcId',
            ),
        ],
        'ctrl_cpu_pmu_events': [
            ('handler', '_get_cpu_pmu_events'),
            ('table', DBNameConstant.TABLE_CPU_ORIGIN),
            ('headers', 'Event,Name,Count'),
            ('db', DBNameConstant.DB_NAME_CTRLCPU),
        ],
        'ctrl_cpu_top_function': [
            ('handler', '_get_cpu_top_funcs'),
            ('table', DBNameConstant.TABLE_TS_CPU_EVENT),
            ('headers', 'Function,Module,Cycles,Cycles(%)'),
            ('db', DBNameConstant.DB_NAME_CTRLCPU),
        ],
        'ai_cpu_pmu_events': [
            ('handler', '_get_cpu_pmu_events'),
            ('table', DBNameConstant.TABLE_CPU_ORIGIN),
            ('headers', 'Event,Name,Count'),
            ('db', DBNameConstant.DB_NAME_AICPU),
        ],
        'ai_cpu_top_function': [
            ('handler', '_get_cpu_top_funcs'),
            ('table', DBNameConstant.TABLE_TS_CPU_EVENT),
            ('headers', 'Function,Module,Cycles,Cycles(%)'),
            ('db', DBNameConstant.DB_NAME_AICPU),
        ],
        'ts_cpu_pmu_events': [
            ('handler', '_get_cpu_pmu_events'),
            ('table', DBNameConstant.TABLE_TSCPU_ORIGIN),
            ('headers', 'Event,Name,Count'),
            ('db', DBNameConstant.DB_NAME_TSCPU),
        ],
        'ts_cpu_top_function': [('handler', '_get_ts_cpu_top_funcs'), ('db', DBNameConstant.DB_NAME_TSCPU)],
        'os_runtime_api': [
            ('handler', '_get_host_runtime_api'),
        ],
        'os_runtime_statistic': [
            ('handler', '_get_host_runtime_api'),
            ('headers', 'Process ID,Thread ID,Name,Time(%),Time(us),Count,Avg(us),Max(us),Min(us)'),
        ],
        'host_cpu_usage': [
            ('handler', '_get_host_cpu_usage_data'),
            ('headers', 'Total Cpu Numbers,Occupied Cpu Numbers,Recommend Cpu Numbers'),
        ],
        'host_mem_usage': [
            ('handler', '_get_host_mem_usage_data'),
            ('headers', 'Total Memory(KB),Peak Used Memory(KB),Recommend Memory(KB)'),
        ],
        'host_network_usage': [
            ('handler', '_get_host_network_usage_data'),
            ('headers', 'Netcard Speed(KB/s),Peak Used Speed(KB/s),Recommend Speed(KB/s)'),
        ],
        'host_disk_usage': [
            ('handler', '_get_host_disk_usage_data'),
            (
                'headers',
                'Peak Disk Read(KB/s),Recommend Disk Read(KB/s),Peak Disk Write(KB/s),Recommend Disk Write(KB/s)',
            ),
        ],
        'msprof': [('handler', '_get_bulk_data')],
        'freq': [('handler', '_get_freq_data')],
        'voltage': [('handler', '_get_voltage_data')],
        'ffts_sub_task_time': [
            ('handler', '_get_task_timeline'),
            ('db', DBNameConstant.DB_SOC_LOG),
            ('table', DBNameConstant.TABLE_FFTS_LOG),
        ],
        'communication': [
            ('handler', '_get_hccl_timeline'),
        ],
        'msprof_tx': [
            ('handler', '_get_msproftx_data'),
            (
                'headers',
                'pid,tid,category,event_type,payload_type,payload_value,Start_time(us),'
                'End_time(us),message_type,message,domain,mark_id',
            ),
        ],
        'sio': [('handler', '_get_sio_data'), ('db', DBNameConstant.DB_SIO), ('table', DBNameConstant.TABLE_SIO)],
        'inter_soc_transmission': [
            ('handler', '_get_inter_soc_summary'),
            ('db', DBNameConstant.DB_STARS_SOC),
            ('table', DBNameConstant.TABLE_SOC_DATA),
            ('headers', 'Metric,L2Buffer_bandwidth_level,MATA_bandwidth_level'),
        ],
        'acc_pmu': [
            ('handler', '_get_acc_pmu'),
            ('db', DBNameConstant.DB_ACC_PMU),
            ('table', DBNameConstant.TABLE_ACC_PMU_DATA),
            (
                'headers',
                'task_id,stream_id,acc_id,block_id,read_bandwidth,write_'
                'bandwidth,read_ost,write_ost,time_stamp(us),start_time,dur_time',
            ),
        ],
        'stars_soc': [
            ('handler', '_get_stars_soc_data'),
            ('db', DBNameConstant.DB_STARS_SOC),
            ('table', DBNameConstant.TABLE_SOC_DATA),
        ],
        'stars_chip_trans': [('handler', '_get_stars_chip_trans_data'), ('db', DBNameConstant.DB_STARS_CHIP_TRANS)],
        'low_power': [
            ('handler', '_get_low_power_data'),
            ('db', DBNameConstant.DB_LOW_POWER),
            ('table', DBNameConstant.TABLE_LOWPOWER),
        ],
        'instr': [('handler', '_get_biu_perf_timeline')],
        'npu_mem': [
            ('handler', '_get_npu_mem_data'),
            ('headers', 'event,ddr(KB),hbm(KB),memory(KB),timestamp(us)'),
        ],
        'memory_record': [
            ('handler', '_get_mem_record_data'),
            ('headers', 'Component,Timestamp(us),Total Allocated(KB),Total Reserved(KB),Device'),
        ],
        'npu_module_mem': [
            ('handler', '_get_npu_module_mem_data'),
            ('headers', 'Component,Timestamp(us),Total Reserved(KB),Device'),
        ],
        'operator_memory': [
            ('handler', '_get_operator_memory_data'),
            (
                'headers',
                'Name,Size(KB),Allocation Time(us),Duration(us),'
                'Allocation Total Allocated(KB),Allocation Total Reserved(KB),'
                'Release Total Allocated(KB),Release Total Reserved(KB),Device',
            ),
        ],
        'api': [
            ('handler', '_get_api_data'),
        ],
        'communication_statistic': [
            ('handler', '_get_hccl_statistic_data'),
            ('headers', 'OP Type,Count,Total Time(us),Min Time(us),Avg Time(us),Max Time(us),Ratio(%)'),
            ('db', DBNameConstant.DB_HCCL_SINGLE_DEVICE),
        ],
        'api_statistic': [
            ('handler', '_get_api_statistic_data'),
            ('headers', 'Level,API Name,Time(us),Count,Avg(us),Min(us),Max(us),Variance'),
        ],
        'aicpu_mi': [
            ('handler', '_get_aicpu_mi_data'),
            ('headers', 'Node Name,Start Time(us),End Time(us),Queue Size'),
        ],
        'qos': [('handler', '_get_qos_data'), ('db', DBNameConstant.DB_QOS)],
        'static_op_mem': [
            ('handler', '_get_static_op_mem_data'),
            ('headers', 'Op Name,Model Name,Graph ID,Node Index Start,Node Index End,Size(KB)'),
        ],
        'block_detail': [
            ('handler', '_get_block_detail_data'),
            ('db', DBNameConstant.DB_SOC_LOG),
            ('table', DBNameConstant.TABLE_BLOCK_LOG),
        ],
        'ub': [
            ('handler', '_get_ub_data'),
            ('db', DBNameConstant.DB_UB),
            ('table', DBNameConstant.TABLE_UB_BW),
            ('headers', 'PortId,TimeStamp,UBRxPortBandWidth(MB/s),UBTxPortBandWidth(MB/s)'),
        ],
        'ccu_mission': [
            ('handler', '_get_ccu_mission_data'),
            ('db', DBNameConstant.DB_CCU),
            ('table', DBNameConstant.TABLE_CCU_MISSION),
            (
                'headers',
                'Stream ID,Task Id,Instruction ID,Instruction Start Time(us),Instruction Duration(us),'
                'Notify Instruction ID,Notify Rank ID,Notify Duration(us)',
            ),
        ],
        'ccu_channel': [
            ('handler', '_get_ccu_channel_data'),
            ('db', DBNameConstant.DB_CCU),
            ('table', DBNameConstant.TABLE_CCU_CHANNEL),
            ('headers', 'Channel Id,Timestamp(us),Max Bandwidth(MB/s),Min Bandwidth(MB/s),Avg Bandwidth(MB/s)'),
        ],
        'soc_pmu': [
            ('handler', '_get_soc_pmu_data'),
            ('db', DBNameConstant.DB_SOC_PMU),
            ('table', DBNameConstant.TABLE_SOC_PMU_SUMMARY),
            ('headers', 'Stream Id,Task Id,TLB Hit Rate,TLB Miss Rate'),
        ],
        'dpu': [
            ('handler', '_get_dpu_data'),
            ('db', DBNameConstant.DB_DPU),
            ('table', [DBNameConstant.TABLE_DPU_TASK_TRACK, DBNameConstant.TABLE_DPU_HCCL_TRACK]),
        ],
    }
