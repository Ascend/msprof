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

from decimal import Decimal


class NumberConstant:
    """
    Constant for number
    """

    NULL_NUMBER = 0
    EXCEPTION = -1
    SUCCESS = 0
    ERROR = 1
    WARN = 2
    SKIP = 3
    KILOBYTE = 1024.0
    FLOAT_ZERO_BOUND = 1e-17

    MS_TIME_RATE = 1000000
    NS_TIME_RATE = 1000000000.0
    DEFAULT_LENGTH = 50
    DEFAULT_NUMBER = 9223372036854775807  # max value for 16 bytes Integer
    DEFAULT_START_TIME = 0
    DEFAULT_END_TIME = 9223372036854775807  # max value for 16 bytes Integer
    FILE_AUTHORITY = 0o640
    DIR_AUTHORITY = 0o750
    U_MASK = 0o027
    NANO_SECOND = 1000000000.0
    MICRO_SECOND = 1000000.0
    MILLI_SECOND = 1000.0
    HEX_NUMBER = 16
    DATA_NUM = 8196
    DEFAULT_TABLE_FIELD_NUM = 0
    RATIO_NAME_LEN = 5
    EXTRA_RATIO_NAME_LEN = 11
    PERCENTAGE = 100
    DECIMAL_ACCURACY = 6
    ROUND_THREE_DECIMAL = 3
    ROUND_TWO_DECIMAL = 2
    LLC_CAPACITY = 64.0
    CPU_FREQ = 680000

    # AICORE metrics index in metric summary tables
    METRICS_DEVICE_INDEX = -4
    METRICS_TASK_INDEX = -3
    METRICS_STREAM_INDEX = -2
    METRICS_ITER_INDEX = -1

    DEFAULT_ITER_ID = 1
    DEFAULT_ITER_COUNT = 1
    DEFAULT_MODEL_ID = -1
    STATIC_SHAPE_ITER_ID = 0

    # time units transfer
    NS_TO_US = 1000.0
    NS_TO_MS = 1000000.0
    TEN_NS_TO_US = 100.0
    NS_TO_S = 0.001**3
    US_TO_S = 0.001**2
    MS_TO_S = 0.001
    S_TO_MS = 1000.0
    MS_TO_US = 1000.0
    US_TO_MS = 1000.0
    MS_TO_NS = 1000000.0
    FREQ_TO_MHz = 1000000.0
    LLC_BYTE = 64.0
    FLT_EPSILON = 1.0e-9
    USTONS = 1000
    DEFAULT_STREAM_ID = 65535
    DEFAULT_TASK_ID = 65535
    PROF_PATH_MAX_LEN = 1024
    INT64_MAX = 2**63 - 1
    UINT64_MAX = 18446744073709551615

    # memory units transfer
    BYTES_TO_KB = 1024.0

    # time conversion ns to us / ms to s
    CONVERSION_TIME = 1000.0

    # string max length, no more than 8 MB
    MAX_STR_LENGTH = 8 * 1024 * 1024

    # llc capacity num translate to MB
    LLC_CAPACITY_CONVERT_MB = 64.0 / (1024 * 1024)
    USAGE_PLACES = Decimal(10) ** -6
    SEC_TO_US = 10**6

    INVALID_ITER_ID = -1
    ZERO_ITER_ID = 0
    ZERO_ITER_END = (0,)

    # training trace index
    FORWARD_PROPAGATION = 3
    STEP_END = 5
    DATA_AUG_BOUND = 9

    COLUMN_COUNT = 9

    # pytorch msproftx event_type
    MARKER = 0
    PUSH_AND_POP = 1
    START_AND_END = 2
    MARKER_EX = 3

    # task time
    TASK_TIME_PID = 0

    # the default batch id of chip v1
    DEFAULT_BATCH_ID = 0
    DEFAULT_FFTS_SUBTASK_ID = 0
    DEFAULT_GE_CONTEXT_ID = 4294967295

    # invalid id
    INVALID_TASK_TIME = -1
    INVALID_STREAM_ID = -1
    INVALID_TASK_ID = -1
    INVALID_OP_EXE_TIME = -1
    INVALID_MODEL_ID = 4294967295

    # core id edge
    MAX_CORE_ID_OF_AIC = 24

    # HCCL info
    RDMA_TRANSIT_OP_NUM = 5
    MAIN_STREAM_THREAD_ID = 0
    WAIT_TIME_THRESHOLD = 0.2
    ANALYSIS_STEP_NUM = 1
    RDMA_BANDWIDTH_V2_1_0 = 12.5
    HCCS_BANDWIDTH_V2_1_0 = 18
    PCIE_BANDWIDTH_V2_1_0 = 20
    RDMA_BANDWIDTH_V4_1_0 = 25
    HCCS_BANDWIDTH_V4_1_0 = 28
    HCCS_MESSAGE_SIZE_THRESHOLD = 32
    PCIE_MESSAGE_SIZE_THRESHOLD = 32
    SIO_MESSAGE_SIZE_THRESHOLD = 32
    RDMA_MESSAGE_SIZE_THRESHOLD = 0.5
    LARGE_MESSAGE_RATE = 0.8
    BANDWIDTH_THRESHOLD = 0.8
    DOMINATED_BOTTLENECK_THRESHOLD = 0.25

    RANK_NUM_PER_SERVER = 8
    RANK_NUM_PER_OS = 4
    MAX_RANK_NUMS = 4096
    COMMUNICATION_B_to_MB = 1000**2
    COMMUNICATION_B_to_GB = 0.001**3
    COMMUNICATION_MB_to_GB = 1000

    # magic number: 5A5A
    MAGIC_NUM = 23130

    HOST_ID = 64

    STATIC_GRAPH_INDEX = 0

    RDMA_NO_BARRIER_TASK_NUM = 3
    RDMA_WITH_BARRIER_TASK_NUM = 5

    # high-precision data threshold
    CSV_MAX_PRECISION = 15

    # voltage, mV
    DEFAULT_VOLTAGE = 900

    @property
    def conversion_time(self: any) -> float:
        """
        time conversion ns to us
        :return: time conversion
        """
        return self.CONVERSION_TIME

    @property
    def max_str_length(self: any) -> int:
        """
        string max length, no more than 8 MB
        :return: string max length
        """
        return self.MAX_STR_LENGTH

    @staticmethod
    def is_zero(number: any) -> bool:
        """
        :param number
        """
        if isinstance(number, int):
            return number == 0
        elif isinstance(number, float):
            return abs(number) < NumberConstant.FLOAT_ZERO_BOUND
        else:
            return False
