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
import struct

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.utils import Utils
from msparser.add_info.add_info_bean import AddInfoBean
from msparser.compact_info.hccl_op_info_bean import HcclOpInfoBean
from msparser.data_struct_size_constant import StructFmt
from msmodel.sqe_type_map import SqeType
from profiling_bean.stars.stars_common import StarsCommon


class AicpuNodeBean:
    """
    aicpu data info bean
    """

    def __init__(self: any, *args) -> None:
        data = args[0]
        self._stream_id = StarsCommon.set_stream_id(data[6], data[7], SqeType.StarsSqeType.AI_CPU)
        self._task_id = StarsCommon.set_task_id(data[6], data[7], SqeType.StarsSqeType.AI_CPU, need_merge=False)
        self._ai_cpu_task_start = data[10]
        # payload 为 us，落盘与 C++ 对齐为 ns
        self._compute_time = (data[12] - data[11]) * NumberConstant.USTONS
        self._mem_copy_time = (data[13] - data[12]) * NumberConstant.USTONS
        self._ai_cpu_task_end = data[15]
        self._submit_tick = data[18]
        self._after_run_tick = data[21]
        self._dispatch_time = data[23] * NumberConstant.USTONS

    @property
    def stream_id(self: any) -> any:
        """
        stream id for ai cpu
        :return: stream id for ai cpu
        """
        return self._stream_id

    @property
    def task_id(self: any) -> any:
        """
        task id for ai cpu
        :return: task id for ai cpu
        """
        return self._task_id

    @property
    def compute_time(self: any) -> float:
        """
        ai cpu compute time
        :return: ai cpu compute time
        """
        return self._compute_time

    @property
    def memory_copy_time(self: any) -> float:
        """
        ai cpu memory copy time
        :return: ai cpu memory copy time
        """
        return self._mem_copy_time

    @property
    def ai_cpu_task_start_time(self: any) -> any:
        """
        ai cpu task start time
        :return: ai cpu task start time
        """
        if self._ai_cpu_task_start != 0:
            return InfoConfReader().time_from_syscnt(self._ai_cpu_task_start)
        return 0

    @property
    def ai_cpu_task_start_syscnt(self: any) -> any:
        """
        ai cpu task start time
        :return: ai cpu task start time
        """
        return self._ai_cpu_task_start

    @property
    def ai_cpu_task_end_time(self: any) -> any:
        """
        ai cpu task end time
        :return: ai cpu task end time
        """
        if self._ai_cpu_task_end != 0:
            return InfoConfReader().time_from_syscnt(self._ai_cpu_task_end)
        return 0

    @property
    def ai_cpu_task_end_syscnt(self: any) -> any:
        """
        ai cpu task end time
        :return: ai cpu task end time
        """
        return self._ai_cpu_task_end

    @property
    def ai_cpu_task_time(self: any) -> float:
        """
        ai cpu task time
        :return: ai cpu task time
        """
        return self.ai_cpu_task_end_time - self.ai_cpu_task_start_time

    @property
    def dispatch_time(self: any) -> float:
        """
        ai cpu dispatch time
        :return: ai cpu dispatch time
        """
        return self._dispatch_time

    @property
    def total_time(self: any) -> float:
        """
        ai cpu total time
        :return: ai cpu total time
        """
        return InfoConfReader().duration_from_syscnt(
            self._after_run_tick - self._submit_tick,
            time_fmt=NumberConstant.NANO_SECOND,
        )


class AicpuDPBean:
    def __init__(self: any, *args) -> None:
        data = args[0]
        self._action = data[6].partition(b"\x00")[0].decode("utf-8", "ignore")
        self._source = data[7].partition(b"\x00")[0].decode("utf-8", "ignore")
        self._buffer_size = data[9]

    @property
    def action(self: any) -> str:
        return self._action

    @property
    def source(self: any) -> str:
        return self._source

    @property
    def buffer_size(self: any) -> int:
        return self._buffer_size


class AicpuModelBean:
    def __init__(self: any, *args) -> None:
        data = args[0]
        self._index_id = data[6]
        self._model_id = data[7]
        self._tag_id = data[8]
        self._event_id = data[10]

    @property
    def index_id(self: any) -> int:
        return self._index_id

    @property
    def model_id(self: any) -> int:
        return self._model_id

    @property
    def tag_id(self: any) -> int:
        return self._tag_id

    @property
    def event_id(self: any) -> int:
        return self._event_id


class AicpuMiBean:
    GET_NEXT_DEQUEUE_WAIT = 1
    NODE_NAME = {
        GET_NEXT_DEQUEUE_WAIT: "GetNext_dequeue_wait",
    }

    def __init__(self: any, *args) -> None:
        data = args[0]
        self._node_tag = data[6]
        self._queue_size = data[8]
        self._start_time = data[9]  # us
        self._end_time = data[10]

    @property
    def node_name(self: any) -> str:
        return self.NODE_NAME.get(self._node_tag, "")

    @property
    def queue_size(self: any) -> int:
        return self._queue_size

    @property
    def start_time(self: any) -> int:
        return self._start_time

    @property
    def end_time(self: any) -> int:
        return self._end_time

    @property
    def duration(self: any) -> int:
        return self._end_time - self._start_time


class KfcHcclInfoBean:
    def __init__(self: any, *args) -> None:
        data = args[0]
        self._item_id = data[0]
        self._ccl_tag = data[1]
        self._group_name = data[2]
        self._local_rank = data[3]
        self._remote_rank = data[4]
        self._rank_size = data[5]
        self._stage = data[6]
        self._notify_id = data[7]
        self._timestamp = data[8]
        self._duration_estimated = data[9]
        self._src_addr = data[10]
        self._dst_addr = data[11]
        self._data_size = data[12]
        self._task_id = StarsCommon.set_task_id(data[15], data[13], SqeType.StarsSqeType.AI_CPU, need_merge=False)
        self._stream_id = StarsCommon.set_stream_id(data[15], data[13], SqeType.StarsSqeType.AI_CPU)
        self._plane_id = data[16]
        self._op_type = data[17]
        self._data_type = data[18]
        self._link_type = data[19]
        self._transport_type = data[20]
        self._rdma_type = data[21]
        self._role = data[22]
        self._work_flow_mode = data[23]
        self._batch_id = 0  # 由AicpuAddInfoParser预计算填充

    @property
    def item_id(self: any) -> str:
        return str(self._item_id)

    @property
    def ccl_tag(self: any) -> str:
        return str(self._ccl_tag)

    @property
    def group_name(self: any) -> str:
        return str(self._group_name)

    @property
    def local_rank(self: any) -> int:
        return Utils.get_valid_int_data(self._local_rank)

    @property
    def remote_rank(self: any) -> int:
        return Utils.get_valid_int_data(self._remote_rank)

    @property
    def rank_size(self: any) -> int:
        return Utils.get_valid_int_data(self._rank_size)

    @property
    def work_flow_mode(self: any) -> str:
        return str(self._work_flow_mode)

    @property
    def plane_id(self: any) -> int:
        return Utils.get_valid_int_data(self._plane_id)

    @property
    def notify_id(self: any) -> str:
        return str(self._notify_id)

    @property
    def stage(self: any) -> str:
        return str(self._stage)

    @property
    def role(self: any) -> str:
        return str(self._role)

    @property
    def duration_estimated(self: any) -> int:
        return Utils.get_valid_int_data(self._duration_estimated)

    @property
    def src_addr(self: any) -> str:
        return str(self._src_addr)

    @property
    def dst_addr(self: any) -> str:
        return str(self._dst_addr)

    @property
    def data_size(self: any) -> int:
        return Utils.get_valid_int_data(self._data_size)

    @property
    def op_type(self: any) -> str:
        return str(self._op_type)

    @property
    def data_type(self: any) -> str:
        return str(self._data_type)

    @property
    def link_type(self: any) -> str:
        return str(self._link_type)

    @property
    def transport_type(self: any) -> str:
        return str(self._transport_type)

    @property
    def rdma_type(self: any) -> str:
        return str(self._rdma_type)

    @property
    def task_id(self: any) -> int:
        return Utils.get_valid_int_data(self._task_id)

    @property
    def stream_id(self: any) -> int:
        return Utils.get_valid_int_data(self._stream_id)

    @property
    def timestamp(self: any) -> int:
        return Utils.get_valid_int_data(self._timestamp)

    @property
    def batch_id(self: any) -> int:
        return self._batch_id

    @batch_id.setter
    def batch_id(self: any, value: int) -> None:
        self._batch_id = value


class MergedKfcHcclInfoBean:
    def __init__(self: any, *args) -> None:
        data = args[0]
        self.first_hccl_info = KfcHcclInfoBean(data[6:39])  # 代码保证这里不会发生越界, 第一条数据
        self.second_hccl_info = KfcHcclInfoBean(data[39:])  # 代码保证这里不会发生越界，第二条数据


class KfcCommTurnBean:
    def __init__(self: any, *args) -> None:
        data = args[0]
        self._server_start_time = data[6]  # 进入KFC流程
        self._wait_msg_start_time = data[7]  # 开始等待客户端消息
        self._kfc_alg_exe_start_time = data[8]  # 开始通信算法执行
        self._send_task_start_time = data[9]  # 开始下发task
        self._send_sqe_finish_time = data[10]  # task下发完成
        self._rtsq_exe_end_time = data[11]  # sq执行结束时间
        self._server_end_time = data[12]  # KFC流程结束时间
        self._data_len = data[13]
        self._device_id = data[14]
        self._stream_id = StarsCommon.set_stream_id(data[15], data[16], SqeType.StarsSqeType.AI_CPU)
        self._task_id = StarsCommon.set_task_id(data[15], data[16], SqeType.StarsSqeType.AI_CPU)
        self._version = data[17]
        self._comm_turn = data[18]
        self._current_turn = data[19]

    @property
    def device_id(self: any) -> int:
        return self._device_id

    @property
    def stream_id(self: any) -> int:
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        return self._task_id

    @property
    def version(self: any) -> int:
        return self._version

    @property
    def current_turn(self: any) -> int:
        return self._current_turn

    @property
    def comm_turn(self: any) -> int:
        return self._comm_turn

    @property
    def server_start_time(self: any) -> int:
        return self._server_start_time

    @property
    def wait_msg_start_time(self: any) -> int:
        return self._wait_msg_start_time

    @property
    def kfc_alg_exe_start_time(self: any) -> int:
        return self._kfc_alg_exe_start_time

    @property
    def send_task_start_time(self: any) -> int:
        return self._send_task_start_time

    @property
    def send_sqe_finish_time(self: any) -> int:
        return self._send_sqe_finish_time

    @property
    def rtsq_exe_end_time(self: any) -> int:
        return self._rtsq_exe_end_time

    @property
    def server_end_time(self: any) -> int:
        return self._server_end_time


class KfcComputeTurnBean:
    def __init__(self: any, *args) -> None:
        data = args[0]
        self._wait_compute_start_time = data[6]
        self._compute_start_time = data[7]
        self._compute_exe_end_time = data[8]
        self._data_len = data[9]
        self._device_id = data[10]
        self._stream_id = StarsCommon.set_stream_id(data[11], data[12])
        self._task_id = StarsCommon.set_task_id(data[11], data[12])
        self._version = data[13]
        self._compute_turn = data[14]
        self._current_turn = data[15]

    @property
    def device_id(self: any) -> int:
        return self._device_id

    @property
    def stream_id(self: any) -> int:
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        return self._task_id

    @property
    def version(self: any) -> int:
        return self._version

    @property
    def current_turn(self: any) -> int:
        return self._current_turn

    @property
    def compute_turn(self: any) -> int:
        return self._compute_turn

    @property
    def wait_compute_start_time(self: any) -> int:
        return self._wait_compute_start_time

    @property
    def compute_start_time(self: any) -> int:
        return self._compute_start_time

    @property
    def compute_exe_end_time(self: any) -> int:
        return self._compute_exe_end_time


class DeviceHcclOpInfoBean(HcclOpInfoBean):
    def __init__(self: any, *args) -> None:
        data = args[0]
        self._relay = (data[6] >> self.RELAY_FLAG_BIT) & 0x1
        self._retry = (data[6] >> self.RETRY_FLAG_BIT) & 0x1
        self._data_type = data[7]
        self._alg_type = data[8]
        self._count = data[9]
        self._group_name = data[10]
        self._rank_size = data[11]
        self._stream_id = StarsCommon.set_stream_id(data[12], data[13], SqeType.StarsSqeType.AI_CPU)
        self._task_id = StarsCommon.set_task_id(data[12], data[13], SqeType.StarsSqeType.AI_CPU, need_merge=False)

    @property
    def stream_id(self: any) -> int:
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        return self._task_id


class AicpuFlipTaskBean:
    def __init__(self: any, *args) -> None:
        data = args[0]
        self._stream_id = StarsCommon.set_stream_id(data[6], data[7], SqeType.StarsSqeType.AI_CPU)
        self._task_id = StarsCommon.set_task_id(data[6], data[7], SqeType.StarsSqeType.AI_CPU)
        self._flip_num = data[8]

    @property
    def stream_id(self: any) -> int:
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        return self._task_id

    @property
    def flip_num(self: any) -> int:
        return self._flip_num


class AicpuMasterStreamHcclTaskBean:
    def __init__(self: any, *args) -> None:
        data = args[0]
        self._aicpu_stream_id = StarsCommon.set_stream_id(data[6], data[7], SqeType.StarsSqeType.AI_CPU)
        self._aicpu_task_id = StarsCommon.set_task_id(data[6], data[7], SqeType.StarsSqeType.AI_CPU)
        self._stream_id = StarsCommon.set_stream_id(data[8], data[9], SqeType.StarsSqeType.AI_CPU)
        self._task_id = StarsCommon.set_task_id(data[8], data[9], SqeType.StarsSqeType.AI_CPU)
        self._type = data[10]
        self._batch_id = 0  # 由AicpuAddInfoParser预计算填充
        self._aicpu_batch_id = 0  # 由AicpuAddInfoParser用device flip预计算填充
        self._timestamp = data[5]

    @property
    def aicpu_stream_id(self: any) -> int:
        return self._aicpu_stream_id

    @property
    def aicpu_task_id(self: any) -> int:
        return self._aicpu_task_id

    @property
    def stream_id(self: any) -> int:
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        return self._task_id

    @property
    def type(self: any) -> int:
        return self._type

    @property
    def batch_id(self: any) -> int:
        return self._batch_id

    @batch_id.setter
    def batch_id(self: any, value: int) -> None:
        self._batch_id = value

    @property
    def aicpu_batch_id(self: any) -> int:
        return self._aicpu_batch_id

    @aicpu_batch_id.setter
    def aicpu_batch_id(self: any, value: int) -> None:
        self._aicpu_batch_id = value

    @property
    def timestamp(self: any) -> int:
        return self._timestamp


class AicpuAddInfoBean(AddInfoBean):
    """
    aicpu data info bean
    """

    AICPU_NODE = 0
    AICPU_DP = 1
    AICPU_MODEL = 2  # helper: MODEL_WITH_Q
    AICPU_MI = 3  # MindSpore
    KFC_COMM_TURN = 4
    KFC_COMPUTE_TURN = 5
    KFC_HCCL_INFO = 13
    HCCL_OP_INFO = 10
    AICPU_FLIP_TASK = 11
    AICPU_MASTER_STREAM_HCCL_TASK = 12

    STRUCT_FMT = {
        AICPU_NODE: StructFmt.AI_CPU_NODE_ADD_FMT,
        AICPU_DP: StructFmt.AI_CPU_DP_ADD_FMT,
        AICPU_MODEL: StructFmt.AI_CPU_MODEL_ADD_FMT,
        AICPU_MI: StructFmt.AI_CPU_MI_ADD_FMT,
        KFC_COMM_TURN: StructFmt.KFC_COMM_TURN_FMT,
        KFC_COMPUTE_TURN: StructFmt.KFC_COMPUTE_TURN_FMT,
        KFC_HCCL_INFO: StructFmt.KFC_HCCL_INFO_FMT,
        HCCL_OP_INFO: StructFmt.DEVICE_HCCL_OP_INFO_FMT,
        AICPU_FLIP_TASK: StructFmt.AICPU_FLIP_TASK_FMT,
        AICPU_MASTER_STREAM_HCCL_TASK: StructFmt.AICPU_MASTER_STREAM_HCCL_TASK_FMT,
    }

    AICPU_BEAN = {
        AICPU_NODE: AicpuNodeBean,
        AICPU_DP: AicpuDPBean,
        AICPU_MODEL: AicpuModelBean,
        AICPU_MI: AicpuMiBean,
        KFC_COMM_TURN: KfcCommTurnBean,
        KFC_COMPUTE_TURN: KfcComputeTurnBean,
        KFC_HCCL_INFO: MergedKfcHcclInfoBean,
        HCCL_OP_INFO: DeviceHcclOpInfoBean,
        AICPU_FLIP_TASK: AicpuFlipTaskBean,
        AICPU_MASTER_STREAM_HCCL_TASK: AicpuMasterStreamHcclTaskBean,
    }

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._aicpu_data = None
        aicpu_bean = self.AICPU_BEAN.get(self._struct_type, None)
        if aicpu_bean:
            self._aicpu_data = aicpu_bean(data)

    @property
    def data(self: any) -> any:
        """
        ai cpu data
        :return: ai cpu data
        """
        return self._aicpu_data

    @classmethod
    def decode(cls: any, binary_data: bytes, additional_fmt: str = "") -> any:
        """
        decode binary data to class
        :param binary_data:
        :param additional_fmt:
        :return:
        """
        struct_type = struct.unpack("=I", binary_data[4:8])[0]
        fmt = StructFmt.BYTE_ORDER_CHAR + cls.STRUCT_FMT.get(struct_type, "") + additional_fmt
        return cls(struct.unpack_from(fmt, binary_data))
