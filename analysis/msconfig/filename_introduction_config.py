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


class FilenameIntroductionConfig(MetaConfig):
    DATA = {
        'timeline': [
            ('begin', "Timeline file description:\n"),
            (
                'msprof',
                "Timeline report.\n"
                "\t1 CPU Layer: Data at the application layer, including the time "
                "consumption information of upper-layer application operators. "
                "The data needs to be collected only in msproftx or PyTorch scenarios.\n"
                "\t2 CANN Layer: Data at the CANN layer, including the time consumption data of "
                "components (such as AscendCL and Runtime) and nodes (operators).\n"
                "\t3 Ascend Hardware Layer: Bottom-layer NPU data, including the time consumption data and "
                "iteration trace data of each task stream under Ascend Hardware, "
                "Communication and Overlap Analysis communication data, and other system data.\n"
                "\t4 Overlap Analysis Layer: In cluster or multi-device scenarios, computation and "
                "communication are sometimes parallel. You can check the pipeline overlapping time "
                "(time when computation and communication are parallel) to determine "
                "the computation and communication efficiencies.\n"
                "\t\tCommunication Layer: Communication time.\n"
                "\t\tCommunication(Not Overlaopped) Layer: Communication time that is not overlapped.\n"
                "\t\tComputing: Computation time.\n"
                "\t\tFree: Interval.\n",
            ),
            (
                'step_trace',
                "You can determine the iteration that takes the longest time based on "
                "the iteration length.\n"
                "\tIteration ID: Iteration ID.\n"
                "\tBP End: BP end time (ns).\n"
                "\tFP_BP Time: FP/BP elapsed time (= BP End – FP Start). The unit is ns.\n"
                "\tIteration Refresh: Iteration refresh hangover time (= Iteration End – BP End).\n"
                "\tData_aug Bound: Data augmentation hangover time "
                "(= Current FP Start – Previous Iteration End). The elapsed time of iteration 0 is "
                "N/A because the previous Iteration End is absent.\n"
                "\tReduce: Collective communication elapsed time (may involve groups of iterations). "
                "If there is only one device, no Reduce data is output.\n",
            ),
            (
                'msprof_tx',
                "msproftx timeline data. The collected host msproftx timeline data is combined "
                "by thread for associated performance display.\n"
                "\tevent_type: Event type.\n",
            ),
        ],
        'summary': [
            ('begin', "Summary file description:\n"),
            (
                'host_cpu_usage',
                "Host-side CPU utilization.\n"
                "\tTotal Cpu Numbers: Total number of CPU cores in the system.\n"
                "\tOccupied Cpu Numbers: Number of CPU cores occupied by processes.\n"
                "\tRecommend Cpu Numbers: Number of CPU cores in use. In virtualization scenarios, "
                "the value is the recommended number of CPU cores.\n",
            ),
            (
                'host_mem_usage',
                "Host-side memory utilization.\n"
                "\tTotal Memory: Total system memory (KB).\n"
                "\tPeak Used Memory: Peak memory utilization (KB).\n"
                "\tRecommend Memory: Recommended memory allocation in virtualization scenarios (KB).\n",
            ),
            (
                'host_disk_usage',
                "Host-side disk I/O utilization.\n"
                "\tPeak Disk Read: Recommended memory allocation in virtualization scenarios (KB).\n"
                "\tRecommend Disk Read: Recommended memory allocation in "
                "virtualization scenarios (KB).\n"
                "\tPeak Disk Write: Recommended memory allocation in virtualization scenarios (KB).\n"
                "\tRecommend Disk Write: Recommended disk write rate in "
                "virtualization scenarios (KB/s).",
            ),
            (
                'host_network_usage',
                "Host-side network I/O utilization.\n"
                "\tNetcard Speed: NIC rated rate (KB/s).\n"
                "\tPeak Used Speed: Maximum network rate (KB/s).\n"
                "\tRecommend Speed: Recommended network rate in virtualization scenarios (KB/s).\n",
            ),
            (
                'os_runtime_statistic',
                "Host-side syscall and pthreadcall.\n"
                "\tCount: Number of calls to an API.\n"
                "\tAvg, Max, Min\n: Average, maximum, and minimum duration of "
                "the calls to an API (μs).\n",
            ),
            (
                'cpu_usage',
                "System CPU usage on the host.\n"
                "\tUser: Percentage of time taken to execute user-mode processes.\n"
                "\tSys: Percentage of time taken to execute kernel-mode processes.\n"
                "\tIoWait: Percentage of I/O wait time.\n"
                "\tIrq: Percentage of hardware interrupt time.\n"
                "\tSoft: Percentage of software interrupt time.\n"
                "\tIdle: Percentage of idle time.\n",
            ),
            ('process_cpu_usage', "Include host process cpu usage data.\n"),
            ('sys_mem', "Memory usage of processes on the host.\n"),
            ('process_mem', "System memory usage on the host.\n"),
            (
                'api_statistic',
                "Time spent by AscendCL API, is used to collect statistics on the time spent "
                "by API execution at the CANN layer.\n"
                "\tLevel: Level of an API, including AscendCL, Runtime, Node, Model, and"
                " Communication.\n",
            ),
            (
                'op_summary',
                "AI Core, AI CPU, AI Vector and COMMUNICATION communication operator data,"
                "is used to collect statistics on operator details and time consumptions.\n"
                "\tOp Name: Operator name.\n"
                "\tOP Type: Operator type. \n"
                "\tTask Type: Task type. \n"
                "\tTask Start Time: Task start time (μs).\n"
                "\tTask Duration: Task duration, including the scheduling time and the start time to "
                "the latest end time of the first core. The unit is μs.\n"
                "\tTask Wait Time: Interval between tasks (μs)."
                "(= this task's start_time - last task's start_time - last task's duration_time)\n"
                "\tBlock Num: Number of running task blocks, which corresponds to the number of cores "
                "during task running.\n"
                "\tMix Block Num: Number of running task blocks in Mix scenarios, which corresponds to "
                "the number of cores during task running.\n"
                "\tContext ID: Context ID.\n"
                "\taiv_time: Average task execution duration on AIV."
                "The value is calculated based on total_cycles "
                "and mix block num.\n"
                "\taicore_time: Average task execution duration on AI Core."
                "The value is calculated based on "
                "total_cycles and block num. The unit is μs. The data is inaccurate in "
                "the manual frequency modulation, dynamic frequency modulation "
                "(the power consumption exceeds the default value), and "
                "Atlas 300V/Atlas 300I Pro scenarios. You are not advised referring to it.\n"
                "\ttotal_cycles: Number of cycles taken to execute all task instructions.\n"
                "\tRegister value: Value of the custom register whose data is to be collected.\n",
            ),
            (
                'op_statistic',
                "AI Core and AI CPU operator calling times and time consumption.\n"
                "The parameters of the msprof command line tool are used as examples. "
                "The parameters of other collection modes are the same.Analyze the total calling time "
                "and total number of calls of each type of operators, check whether there are "
                "any operators with long total execution time, and analyze whether there is "
                "any optimization space for these operators.\n",
            ),
            (
                'step_trace',
                "Iteration trace data.\n"
                "\tIteration End: End time of each iteration. The unit is μs.\n"
                "\tIteration Time(us): Iteration time. (Iteration End of the current iteration - Iteration "
                "End of the previous iteration). The Iteration End data of the previous iteration is "
                "unavailable when the duration of the first iteration is calculated. "
                "Therefore, Duration of the first iteration = Iteration End time of the current "
                "iteration – FP start time of the current iteration. The unit is μs.\n"
                "\tFP to BP Time(us): FP/BP elapsed time (= BP End – FP Start). The unit is μs.\n"
                "\tIteration Refresh(us): Iteration refresh hangover time (= Iteration End – BP End). "
                "The unit is μs.\n"
                "\tData Aug Bound(us): Data augmentation hangover time "
                "(= Current FP Start – Previous Iteration End). The elapsed time of iteration 0 "
                "is N/A because the previous Iteration End is absent. The unit is μs.\n"
                "\tModel ID: Graph ID in the model of a round of iteration.\n"
                "\tReduce Start: Start time of collective communication.\n"
                "\tReduce Duration(us): Total time spent by collective communication. "
                "The collective communication duration is divided into two segments according to "
                "the default segmentation policy. Reduce Start indicates the start time, and Reduce "
                "Duration indicates the duration (μs) from the start to the end. Note that the Reduce "
                "columns are not available in a single-device environment.\n",
            ),
            (
                'communication_statistic',
                "Communication operator statistics, through which you can learn the time"
                " consumption of this operator type and the time consumption ratio of each"
                " Communication operator in collective communication to determine whether an"
                " operator can be optimized.\n",
            ),
            (
                'aicpu',
                "AI CPU summary.This file collects AI CPU data reported by data preprocessing. "
                "Other files involving AI CPU data collect full AI CPU data.\n",
            ),
            ('fusion_op', "Operator fusion summary.\n"),
            (
                'task_time',
                "Task Scheduler summary.\n"
                "\tWaiting: Total wait time of a task (μs).\n"
                "\tRunning: Total run time of a task (μs). An abnormally large value indicates that "
                "the operator implementation needs to be improved.\n"
                "\tPending: Total pending time of a task (μs).\n",
            ),
            (
                'l2_cache',
                "L2 cache data.\n"
                "\tHit Rate: L2 hit rate in requests received by DHA from AI Core\n"
                "\tVictim Rate: Victim rate in requests received by DHA from AI Core.\n",
            ),
            (
                'ai_core_utilization',
                "AI Core utilization.\n"
                "\ticache_miss_rate: I-Cache miss rate. The smaller the value, "
                "the higher the performance.\n"
                "\tmemory_bound: AI Core memory bound, calculated as: "
                "mte2_ratio/max(mac_ratio, vec_ratio). If the value is less than 1, "
                "no memory bound exists. If the value is greater than 1, a memory bound exists. "
                "The greater the value, the severer the bound.\n",
            ),
            (
                'ai_vector_core_utilization',
                "Percentage of instructions on each AI Vector Core.\n"
                "\ticache_miss_rate: I-Cache miss rate. The smaller the value, "
                "the higher the performance.\n"
                "\tmemory_bound: AI Core memory bound, calculated as: "
                "mte2_ratio/max(mac_ratio, vec_ratio). If the value is less than 1, "
                "no memory bound exists. If the value is greater than 1,"
                "a memory bound exists. "
                "The greater the value, the severer the bound.\n",
            ),
            (
                'memory_record',
                "Records the memory applied for by the GE component at the CANN leveland the memory usage time.\n",
            ),
            (
                'operator_memory',
                "Records the memory required and occupied time when"
                "CANN level operators are executed on the NPU. The memory is applied by the GE.\n",
            ),
            ('ddr', "DDR memory read/write speed.\n"),
            ('hbm', "HBM memory read/write speed.\n"),
            (
                'npu_mem',
                "NPU memory usage data.\n"
                "\tevent: Event type.device app\n"
                "\tddr(KB): DDR memory usage. This field is not supported by the "
                "current product and is displayed as 0.\n"
                "\thbm(KB): HBM memory usage. This field is not supported by the "
                "current product and is displayed as 0.\n"
                "\tmemory(KB): Sum of DDR and HBM memory usages.\n",
            ),
            ('hccs', "HCCS bandwidth.\n\tMode: Tx and Rx: receive bandwidth and transmit bandwidth (MB/s).\n"),
            (
                'roce',
                "RoCE bandwidth.\n"
                "\tRx Bandwidth efficiency: Receive bandwidth.\n"
                "\trxPacket/s: Packet receive rate.\n"
                "\trxError rate: Received packet error rate.\n"
                "\trxDropped rate: Received packet loss rate.\n"
                "\tTx Bandwidth efficiency: Transmit bandwidth.\n"
                "\ttxPacket/s: Packet transmit rate.\n"
                "\ttxError rate: Transmitted packet error rate.\n"
                "\ttxDropped rate: Transmitted packet loss rate.\n"
                "\tfuncId: Node ID.\n",
            ),
            ('pcie', "PCIe bandwidth.\n"),
            (
                'nic',
                "NIC summary.\n"
                "\tRx Bandwidth efficiency: Receive bandwidth.\n"
                "\trxPacket/s: Packet receive rate.\n"
                "\trxError rate: Received packet error rate.\n"
                "\trxDropped rate: Received packet loss rate.\n"
                "\tTx Bandwidth efficiency: Transmit bandwidth.\n"
                "\ttxPacket/s: Packet transmit rate.\n"
                "\ttxError rate: Transmitted packet error rate.\n"
                "\ttxDropped rate: Transmitted packet loss rate.\n"
                "\tfuncId: Node ID.\n",
            ),
            (
                'dvpp',
                "DVPP data.\n"
                "\tDvpp Id: DVPP ID.The ID range is related to the number of devices on the AI Server. "
                "Each device is mapped to five IDs, indexed starting at 0.\n",
            ),
            (
                'llc_read_write',
                "L3 cache read/write speed.\n"
                "\tHit Rate: L3 cache hit rate.\n"
                "\tThroughput: L3 cache throughput (MB/s).\n",
            ),
            ('llc_aicpu', "L3 cache used by AI CPU.\n\tUsed Capacity of LLC: Used L3 cache (MB).\n"),
            ('llc_ctrlcpu', "L3 cache used by Ctrl CPU.\n\tUsed Capacity of LLC: Used L3 cache (MB).\n"),
            ('llc_bandwidth', "L3 cache bandwidth."),
            (
                'ai_cpu_top_function',
                "AI CPU top functions.\n"
                "\tFunction: Top functions of AI CPU.\n"
                "\tModule: Module where the function is located.\n"
                "\tCycles: Cycles taken to execute a function in the sampling period.\n"
                "\tCycles(%): Percentage of cycles taken to execute a function "
                "in the sampling period.\n",
            ),
            ('ai_cpu_pmu_events', "AI CPU PMU events.\n"),
            (
                'ctrl_cpu_pmu_events',
                "Ctrl CPU top functions.\n"
                "\tFunction: Top functions of Ctrl CPU.\n"
                "\tModule: Module where the function is located.\n"
                "\tCycles: Cycles taken to execute a function in the sampling period.\n"
                "\tCycles(%): Percentage of cycles taken to execute a function "
                "in the sampling period.\n",
            ),
            ('ctrl_cpu_pmu_events', "Ctrl CPU PMU events.\n"),
            (
                'ts_cpu_top_function',
                "TS CPU top functions.\n"
                "\tFunction: Top functions of TS CPU.\n"
                "\tCycles: Cycles taken to execute a function in the sampling period.\n"
                "\tCycles(%): Percentage of cycles taken to execute a function "
                "in the sampling period.\n",
            ),
            ('ts_cpu_pmu_events', "TS CPU PMU events.\n"),
            (
                'msprof_tx',
                "msproftx summary data. The collected host msproftx summary data is combined by "
                "thread for associated performance display.\n"
                "\tmessage: Character string description carried in the msproftx profiling process.\n",
            ),
        ],
    }
