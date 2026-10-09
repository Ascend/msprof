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

# pylint: disable=duplicate-code
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

# DB name
DB_ACL_MODULE = "acl_module.db"
DB_AICORE_OP_SUMMARY = "ai_core_op_summary.db"
DB_AI_CPU = "ai_cpu.db"
DB_GE_INFO = "ge_info.db"
DB_GE_MODEL_INFO = "ge_model_info.db"
DB_HASH = "hash_mapping.db"
DB_HOST_CPU_USAGE = "host_cpu_usage.db"
DB_HOST_DISK_USAGE = "host_disk_usage.db"
DB_HOST_MEM_USAGE = "host_mem_usage.db"
DB_HOST_NETWORK_USAGE = "host_network_usage.db"
DB_HOST_RUNTIME_API = "host_runtime_api.db"
DB_HWTS = "hwts.db"
DB_HWTS_REC = "hwts-rec.db"
DB_L2CACHE = "l2cache.db"
DB_LLC = "llc.db"
DB_OP_COUNTER = "op_counter.db"
DB_PERIPHERAL = "peripheral.db"
DB_RTS_TRACK = "rts_track.db"
DB_RUNTIME = "runtime.db"
DB_TRACE = "trace.db"
DB_TIME = "time.db"
DB_STEP_TRACE = "step_trace.db"
DB_SOC_LOG = "stars_soc_log.db"
DB_FFTS_PMU = "ffts_pmu.db"

# DB tables
TABLE_ACL_DATA = "acl_data"
TABLE_METRIC_SUMMARY = "MetricSummary"
TABLE_AIV_METRIC_SUMMARY = "AivMetricSummary"
TABLE_AI_CORE_REC = "AiCoreRec"
TABLE_AI_CPU = "ai_cpu_datas"
TABLE_ALL_REDUCE = "all_reduce"
TABLE_EVENT_COUNTER = "EventCounter"
TABLE_GE_TASK = "ge_task_data"
TABLE_GE_GRAPH = "ge_graph_data"
TABLE_GE_INFER = "GEInfer"
TABLE_GE_LOAD_TABLE = "GELoad"
TABLE_GE_STEP_INFO = "step_info"
TABLE_GE_STEP_INFO_DATA = "step_info_data"
TABLE_GE_TASK_DATA = "ge_task_data"
TABLE_HASH_ACL = "hash_acl_dict"
TABLE_HWTS_SYS_RANGE = "hwts_sys_cnt_range"
TABLE_HWTS_ITER_SYS = "hwts_iter_sys"
TABLE_HWTS_TASK = "HwtsTask"
TABLE_HWTS_TASK_TIME = "TaskTime"
TABLE_LLC_BANDWIDTH = "LLCMetricData"
TABLE_LLC_DSID = "LLCDsidData"
TABLE_OP_COUNTER_GE_MERGE = "ge_task_merge"
TABLE_OP_COUNTER_OP_REPORT = "op_report"
TABLE_OP_COUNTER_RTS_TASK = "rts_task"
TABLE_RUNTIME_REPORT_TASK = "ReportTask"
TABLE_RUNTIME_TASK_TIME = "TaskTime"
TABLE_RUNTIME_TIMELINE = "TimeLine"
TABLE_RUNTIME_TRACK = "RuntimeTrack"
TABLE_SUMMARY_GE = "ge_summary"
TABLE_SUMMARY_METRICS = "ai_core_metrics"
TABLE_SUMMARY_TASK_TIME = "task_time"
TABLE_STEP_TRACE = "step_trace"
TABLE_STEP_TRACE_DATA = "step_trace_data"
TABLE_TIME = 'time'
TABLE_TRACE_FILES = 'files'
TABLE_TRAINING_TRACE = 'training_trace'
# cpu usage
TABLE_HOST_CPU_INFO = "CpuInfo"
TABLE_HOST_CPU_USAGE = "CpuUsage"
# disk usage
TABLE_HOST_DISK_USAGE = "DiskUsage"
# mem usage
TABLE_HOST_MEM_USAGE = "MemUsage"
# network usage
TABLE_HOST_NETWORK_USAGE = "NetworkUsage"
TABLE_HOST_PROCESS_USAGE = "ProcessUsage"
# host syscall
TABLE_HOST_RUNTIME_API = "Syscall"
# ffts_log
TABLE_FFTS_THREAD_LOG = "FftsThreadLog"
TABLE_THREAD_TASK = "ThreadTime"
TABLE_SUBTASK_TIME = "SubtaskTime"
TABLE_FFTS_PMU = 'SubTaskPmu'
