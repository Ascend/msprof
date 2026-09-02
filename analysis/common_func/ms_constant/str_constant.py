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

import re

from common_func.db_name_constant import DBNameConstant


class StrConstant:
    """
    Constant for String
    """

    # msvp data types
    GE_BASIC_DATA = "ge_basic"
    AIC = "aic"
    AIV = "aiv"
    HWTS = "hwts"
    DVPP_DATA = "dvpp"
    NIC_DATA = "nic"
    ROCE_DATA = "roce"
    LLC_AICPU = "llc_aicpu"
    LLC_CTRLCPU = "llc_ctrlcpu"
    LLC_PROF = "llc_profiling"
    LLC_CAPACITY_ITEM = "capacity"
    LLC_BAND_ITEM = "bandwidth"
    CTL_CPU_PMU = "ctrl_cpu_pmu_events"
    AI_CPU_PMU = "ai_cpu_pmu_events"
    TS_CPU_PMU = "ts_cpu_pmu_events"
    AI_CORE_OP_SUMMARY = "ai_core_op_summary"
    TOP_DOWN = "top_down"
    AICPU = "aicpu"
    CTRL_CPU = "ctrlcpu"

    # msvp info params
    PARAM_RESULT_DIR = "project"
    PARAM_DATA_TYPE = "data_type"
    PARAM_DEVICE_ID = "device_id"
    PARAM_MODEL_ID = "model_id"
    PARAM_ITER_ID = "iter_id"
    PARAM_EXPORT_TYPE = "export_type"
    PARAM_EXPORT_FORMAT = "export_format"
    PARAM_EXPORT_DUMP_FOLDER = "export_dump_folder"

    # constant within sample config
    SAMPLE_CONFIG_PROJECT_PATH = "result_dir"
    TIME_PATTERN = r'\d+\.\d+'
    SAMPLE_CONFIG = "sample_config"

    # info.json key
    CPU_NUMS = "cpuNums"
    SYS_CLOCK_FREQ = "sysClockFreq"
    MEMORY_TOTAL = "memoryTotal"
    NET_CARD_NUMS = "netCardNums"
    NET_CARD = "netCard"
    LLC_ORIGIN_TABLE = "LLCOriginalData"
    LLC_EVENTS_TABLE = "LLCEvents"
    LLC_METRICS_TABLE = "LLCMetrics"
    COLLECT_DATE_BEGIN = "collectionDateBegin"
    COLLECT_DATE_END = "collectionDateEnd"
    COLLECT_TIME_BEGIN = "collectionTimeBegin"
    COLLECT_TIME_END = "collectionTimeEnd"
    COLLECT_RAW_TIME_BEGIN = "clockMonotonicRaw"
    COLLECT_RAW_TIME_END = "clockMonotonicRaw"
    MONOTONIC_TIME = "clock_monotonic_raw"
    DEVICE_SYSCNT = "cntvct"
    ITER_ID = "iter_id"

    # sample.json key
    HOST_CPU_SAMPLING_INTV = "host_cpu_profiling_sampling_interval"
    HOST_MEM_SAMPLING_INTV = "host_mem_profiling_sampling_interval"

    # msvp export type
    EXPORT_JSON = "json"
    EXPORT_CSV = "csv"
    FILE_SUFFIX_JSON = ".json"
    FILE_SUFFIX_CSV = ".csv"
    DATA_TYPE = "data_type"
    CORE_DATA_TYPE = "core_data_type"
    CONFIG_HANDLER = "handler"
    CONFIG_HEADERS = "headers"
    CONFIG_DB = "db"
    CONFIG_TABLE = "table"
    CONFIG_COLUMNS = "columns"
    CONFIG_UNUSED_COLS = "unused_cols"
    CONFIG_FMT = "fmt"
    INFO_JSON = "info.json"

    AICORE_PROFILING_MODE = "ai_core_profiling_mode"
    AIV_PROFILING_MODE = "aiv_profiling_mode"
    AI_CORE_PMU_EVENTS = "ai_core_profiling_events"
    AI_VECTOR_CORE_PMU_EVENTS = "aiv_profiling_events"
    AI_CORE_PROFILING_METRICS = "ai_core_metrics"
    AI_VECTOR_CORE_PROFILING_METRICS = "aiv_metrics"
    AIV_PROFILING_METRICS = "aiv_metrics"
    LLC_PROFILING_READ_EVENT = "read"
    LLC_PROFILING_WRITE_EVENT = "write"
    HOST_DISK_FREQ = "host_disk_freq"
    DATA_PATH = "data"
    TIMELINE_PATH = "timeline"
    HOST_PATH = "host"
    DEVICE_PATH = "device"
    AIC_TASK_BASED_MODE = "task-based"
    AIC_SAMPLE_BASED_MODE = "sample-based"

    # timeline flow cat
    ASYNC_ACL_NPU = "async_acl_npu"
    ASYNC_NPU = "async_npu"

    HOST_TO_DEVICE = "HostToDevice"
    ACL_RECORD_EVENT = "AscendCL@aclrtRecordEvent"
    ACL_WAIT_EVENT = "AscendCL@aclrtStreamWaitEvent"

    MSTX = "MsTx"

    # pmu task type
    CONTEXT_PMU_TYPE = 'context_task'
    BLOCK_PMU_TYPE = 'block_task'

    RATIO_NAME = "ratio"
    RATIO_EXTRA_NAME = "ratio_extra"
    MAC_RATIO = "mac_ratio"
    AIC_MAC_RATIO = "aic_mac_ratio"
    VEC_RATIO = "vec_ratio"
    AIV_VEC_RATIO = "aiv_vec_ratio"
    MTE1_RATIO = "mte1_ratio"
    MTE2_RATIO = "mte2_ratio"
    MTE3_RATIO = "mte3_ratio"
    SCALAR_RATIO = "scalar_ratio"
    AIC_SCALAR_RATIO = "aic_scalar_ratio"
    AIV_SCALAR_RATIO = "aiv_scalar_ratio"
    AIC_MTE1_RATIO = "aic_mte1_ratio"
    AIC_MTE2_RATIO = "aic_mte2_ratio"
    AIC_MTE3_RATIO = "aic_mte3_ratio"
    AIV_MTE1_RATIO = "aiv_mte1_ratio"
    AIV_MTE2_RATIO = "aiv_mte2_ratio"
    AIV_MTE3_RATIO = "aiv_mte3_ratio"
    TASK_DURATION = 'task_duration'

    BANDWIDTH = "bw"
    ACCURACY = "%.6f"
    AYNC_MEMCPY = "MemcpyAsync"

    TRACE_HEADER_PID = "pid"
    TRACE_HEADER_TID = "tid"
    TRACE_HEADER_NAME = "name"
    TRACE_HEADER_ARGS = "args"

    API_EVENT_HEADER_LEVEL = "level"

    TASK_START_TIME = 'Task Start Time(us)'

    EXPORT_MODE = "export_mode"

    # prefix for api and event
    LEVEL_MAP = {"acl": "AscendCL", "runtime": "Runtime", "model": "Model", "node": "Node"}

    # parallel mode
    STAND_ALONE = "stand_alone"
    DATA_PARALLEL = "data_parallel"
    MODEL_PARALLEL = "model_parallel"
    PIPELINE_PARALLEL = "pipeline_parallel"
    PARALLEL_TABLE_NAME_MAPPING = {
        DATA_PARALLEL: DBNameConstant.TABLE_CLUSTER_DATA_PARALLEL,
        MODEL_PARALLEL: DBNameConstant.TABLE_CLUSTER_MODEL_PARALLEL,
        PIPELINE_PARALLEL: DBNameConstant.TABLE_CLUSTER_PIPELINE_PARALLEL,
    }

    TASK_TYPE_MAPPING = {
        "0": 'kernel AI core task',
        "1": 'kernel AI cpu task',
        "2": 'event record task',
        "3": 'stream wait event task',
        "4": 'fusion issue task',
        "5": 'memory copy task',
        "6": 'maintenance task',
        "7": 'create stream task',
        "8": 'kernel data dump task',
        "9": 'event notify task',
        "10": 'pctrace enable task',
        "11": 'create L2 addr task',
        "12": 'model maintaince task',
        "13": 'model execute task',
        "14": 'notify wait task',
        "15": 'notify record task',
        "16": 'HCCL rdma cpy task',
        "17": 'L2 SDMA memory copy task',
        "18": 'stream switch task',
        "19": 'stream active task',
        "20": 'label set task',
        "21": 'label switch task',
        "22": 'label goto task',
        "23": 'profiler trace task',
        "24": 'event reset task',
        "25": "HCCL rdma db cpy task",
        "26": "profiler trace task",
        "50": "stars common task",
        "51": "ffts task",
        "52": "ffts plus task",
        "64": 'profiling enable task',
        "65": 'profiling disable task',
        "66": "AI vector task",
        "67": "add model end graph task",
        "68": "AICPU schedule task",
        "69": "active stream task",
        "70": "load data dump info task",
        "71": "stream switch n task",
        "72": "Host func Callback task",
        "73": "start online profiling task",
        "74": "stop online profiling task",
        "75": "stream label switch task",
        "77": "stream label goto task",
        "78": "overflow debug register task",
        "79": "overflow debug unregister task",
        "80": "L1 fusion dump set task",
        "81": "add model exit graph task",
        "82": "mdc profiling task",
        "83": "device ring buff set task",
        "84": "stream overflow debug register task",
        "85": "stream overflow debug unregister task",
        "86": "timeout set task",
        "87": "get device message task",
        "101": "model-task update task",
        "116": "task update task",
        "": "",
    }

    DVPP_ENGINE_TYPE = {"0": "VDEC", "1": "JPEGD", "2": "PNGD", "3": "JPEGE", "4": "VPC", "5": "VENC", "6": "SCD"}

    OPERATOR_PLUS = '+'
    OPERATOR_MINUS = '-'
    OPERATOR_MULTIPLY = '*'
    OPERATOR_DIVISOR = '/'

    API_FUNC_NAME_FILTER = [
        re.compile("accept"),
        re.compile("accept4"),
        re.compile("acct"),
        re.compile("alarm"),
        re.compile("arch_prctl"),
        re.compile("bind"),
        re.compile("bpf"),
        re.compile("brk"),
        re.compile("chroot"),
        re.compile("clock_nanosleep"),
        re.compile("connect"),
        re.compile("copy_file_range"),
        re.compile("creat"),
        re.compile("creat64"),
        re.compile("dup"),
        re.compile("dup2"),
        re.compile("dup3"),
        re.compile("epoll_ctl"),
        re.compile("epoll_pwait"),
        re.compile("epoll_wait"),
        re.compile("fallocate"),
        re.compile("fallocate64"),
        re.compile("fcntl"),
        re.compile("fdatasync"),
        re.compile("flock"),
        re.compile("fork"),
        re.compile("fsync"),
        re.compile("ftruncate"),
        re.compile("futex"),
        re.compile("ioctl"),
        re.compile("ioperm"),
        re.compile("iopl"),
        re.compile("kill"),
        re.compile("killpg"),
        re.compile("listen"),
        re.compile("membarrier"),
        re.compile("mlock"),
        re.compile("mlock2"),
        re.compile("mlockall"),
        re.compile("mmap"),
        re.compile("mmap64"),
        re.compile("mount"),
        re.compile("move_pages"),
        re.compile("mprotect"),
        re.compile("mq_notify"),
        re.compile("mq_open"),
        re.compile("mq_receive"),
        re.compile("mq_send"),
        re.compile("mq_timedreceive"),
        re.compile("mq_timedsend"),
        re.compile("mremap"),
        re.compile("msgctl"),
        re.compile("msgget"),
        re.compile("msgrcv"),
        re.compile("msgsnd"),
        re.compile("msync"),
        re.compile("munmap"),
        re.compile("nanosleep"),
        re.compile("nfsservctl"),
        re.compile("open"),
        re.compile("open64"),
        re.compile("openat"),
        re.compile("openat64"),
        re.compile("pause"),
        re.compile("pipe"),
        re.compile("pipe2"),
        re.compile("pivot_root"),
        re.compile("poll"),
        re.compile("ppoll"),
        re.compile("prctl"),
        re.compile("pread"),
        re.compile("pread64"),
        re.compile("preadv"),
        re.compile("preadv2"),
        re.compile("preadv64"),
        re.compile("process_vm_readv"),
        re.compile("process_vm_writev"),
        re.compile("pselect6"),
        re.compile("ptrace"),
        re.compile("pwrite"),
        re.compile("pwrite64"),
        re.compile("pwritev"),
        re.compile("pwritev2"),
        re.compile("pwritev64"),
        re.compile("read"),
        re.compile("readv"),
        re.compile("reboot"),
        re.compile("recv"),
        re.compile("recvfrom"),
        re.compile("recvmmsg"),
        re.compile("recvmsg"),
        re.compile("rt_sigaction"),
        re.compile("rt_sigqueueinfo"),
        re.compile("rt_sigsuspend"),
        re.compile("rt_sigtimedwait"),
        re.compile("sched_yield"),
        re.compile("seccomp"),
        re.compile("select"),
        re.compile("semctl"),
        re.compile("semget"),
        re.compile("semop"),
        re.compile("semtimedop"),
        re.compile("send"),
        re.compile("sendfile"),
        re.compile("sendfile64"),
        re.compile("sendmmsg"),
        re.compile("sendmsg"),
        re.compile("sendto"),
        re.compile("shmat"),
        re.compile("shmctl"),
        re.compile("shmdt"),
        re.compile("shmget"),
        re.compile("shutdown"),
        re.compile("sigaction"),
        re.compile("sigsuspend"),
        re.compile("sigtimedwait"),
        re.compile("socket"),
        re.compile("socketpair"),
        re.compile("splice"),
        re.compile("swapoff"),
        re.compile("swapon"),
        re.compile("sync"),
        re.compile("sync_file_range"),
        re.compile("syncfs"),
        re.compile("tee"),
        re.compile("tgkill"),
        re.compile("tgsigqueueinfo"),
        re.compile("tkill"),
        re.compile("truncate"),
        re.compile("umount2"),
        re.compile("unshare"),
        re.compile("uselib"),
        re.compile("vfork"),
        re.compile("vhangup"),
        re.compile("vmsplice"),
        re.compile("wait"),
        re.compile("wait3"),
        re.compile("wait4"),
        re.compile("waitid"),
        re.compile("waitpid"),
        re.compile("write"),
        re.compile("writev"),
        re.compile("_sysctl"),
    ]

    # hccl str constant
    TRANSIT_TYPE = ["RDMA", "HCCS", "PCIE", "SDMA", "SIO", "UB"]
    TOTAL = "Total HCCL Operators"
    ON_CHIP = "ON_CHIP"
    HCCS = "HCCS"
    PCIE = "PCIE"
    SIO = "SIO"
    HCCS_SW = "HCCS_SW"
    STANDARD_ROCE = "STANDARD_ROCE"
    RDMA = "RDMA"
    SDMA = "SDMA"
    UB = "UB"
    UBOE = "UBoE"
    UB_TYPES = (UB, UBOE)
    LOCAL = "LOCAL"
    NOTIFY_WAIT = "Notify_Wait"
    RDMA_PAYLOAD_PREPARE = "RDMA_PAYLOAD_PREPARE"
    RDMA_SEND_PAYLOAD = "RDMA_SEND_PAYLOAD"
    RDMA_PAYLOAD_ACK = "RDMA_PAYLOAD_ACK"
    REDUCE_TBE = "Reduce TBE"
    RDMA_SEND = "RDMASend"
    SDMA_TRANSIT_ITEMS = ["Memcpy", "Reduce_Inline"]
    UB_TRANSIT_ITEMS = ["Ub_Inline_Write", "Ub_Write_Or_Read"]
    COMMUNICATION_TIME_INFO = "Communication Time Info"
    COMMUNICATION_BANDWIDTH_INFO = "Communication Bandwidth Info"
    SLOW_RANK_SUGGESTION = "Slow Rank Suggestion"
    SLOW_LINK_SUGGESTION = "Slow Link Suggestion"
    MATRIX_SUGGESTION = "Matrix Suggestion"
    SUGGESTION = "Suggestion"
    SUGGESTION_HEADER = "Suggestion: "
    OP_NAME = "op_name"
    LINK_INFO = "link_info"
    TIME_RATIO = "Time Ratio"
    TRANSPORT_TYPE_INFO = "Transport Type Info"
    AVERAGE_INFO = "Average Info"
    SLOWEST_LINK_INFO = "Slowest Link Info"

    # Bound constant
    CUBE_UTILIZATION = "Cube Utilization"
    VECTOR_UTILIZATION = "Vector Utilization"
    SCALAR_UTILIZATION = "Scalar Utilization"
    MTE_UTILIZATION = "MTE Utilization"

    # prof level
    PROF_LEVEL_0 = "l0"
    PROF_LEVEL_0_HISI = "level0"

    STATUS = "status"
    INFO = "info"
    MSG = "msg"
    DATA = "data"
    AICPU_KERNEL = "AicpuKernel"
    AIV_KERNEL = "AivKernel"
    NORMAL = "normal"

    @property
    def accuracy(self: any) -> str:
        """
        format of accuracy
        :return: format
        """
        return self.ACCURACY

    @property
    def bandwidth(self: any) -> str:
        """
        sign of bandwidth
        :return: sign of bandwidth
        """
        return self.BANDWIDTH


class OpAnalysisType:
    START_TIME = 'Start Timestamp(us)'
    ELAPSE_TIME = "Elapse Time(ms)"
    TRANSIT_TIME = "Transit Time(ms)"
    WAIT_TIME = "Wait Time(ms)"
    SYNCHRONIZATION_TIME = "Synchronization Time(ms)"
    IDLE_TIME = 'Idle Time(ms)'
    WAIT_TIME_RATIO = "Wait Time Ratio"
    SYNCHRONIZATION_TIME_RATIO = "Synchronization Time Ratio"


class OpBandWidthType:
    TRANSIT_SIZE_MB = "Transit Size(MB)"
    TRANSIT_TIME_MS = "Transit Time(ms)"
    BANDWIDTH_GB_S = "Bandwidth(GB/s)"
    BANDWIDTH_UTILIZATION = "Bandwidth(Utilization)"
    LARGE_PACKET_RATIO = "Large Packet Ratio"
    SIZE_DISTRIBUTION = "Size Distribution"


class CommunicationMatrixInfo:
    SRC_RANK = "Src Rank"
    DST_RANK = "Dst Rank"
    TRANSPORT_TYPE = "Transport Type"
    TRANSIT_SIZE_MB = "Transit Size(MB)"
    TRANSIT_TIME_MS = "Transit Time(ms)"
    BANDWIDTH_GB_S = "Bandwidth(GB/s)"
    BANDWIDTH_UTILIZATION = "Bandwidth(Utilization)"
    LARGE_PACKET_RATIO = "Large Packet Ratio"
