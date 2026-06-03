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

from msparser.data_struct_size_constant import StructFmt


class StructDecoder:
    """
    class for decode binary data
    """

    STRUCT_FMT_DICT = {
        "TimeLineData": StructFmt.TIME_LINE_FMT,
        "AiCoreTaskInfo": StructFmt.EVENT_COUNT_FMT,
        "StepTrace": StructFmt.STEP_TRACE_FMT,
        "StepTraceChipV6": StructFmt.STEP_TRACE_V6_FMT,
        "TsMemcpy": StructFmt.TS_MEMCPY_FMT,
        "AiCpuData": StructFmt.AI_CPU_FMT,
        "AcsqTask": StructFmt.ACSQ_TASK_FMT,
        "AccPmuDecoder": StructFmt.ACC_PMU_FMT,
        "InterSoc": StructFmt.SOC_FMT,
        "SioDecoderImpl": StructFmt.SIO_FMT,
        "SioDecoderV6": StructFmt.V6_SIO_FMT,
        "StarsDecoder": StructFmt.STARS_FMT,
        "LpeDecoder": StructFmt.LPE_FMT,
        "LpsDecoder": StructFmt.LPS_FMT,
        "FftsPmuBean": StructFmt.FFTS_PMU_FMT,
        "FftsBlockPmuBean": StructFmt.FFTS_BLOCK_PMU_FMT,
        "FftsLogDecoder": StructFmt.FFTS_LOG_FMT,
        "FftsPlusLogDecoder": StructFmt.FFTS_PLUS_LOG_FMT,
        "AicoreSample": StructFmt.AICORE_SAMPLE_FMT,
        "AivPmuBean": StructFmt.AIV_FMT,
        "TscpuDecoder": StructFmt.TSCPU_FMT,
        "MdcTscpuDecoder": StructFmt.MDC_TSCPU_FMT,
        "HwtsLogBean": StructFmt.HWTS_LOG_FMT,
        "AicPmuBean": StructFmt.AIC_PMU_FMT,
        "MsprofTxDecoder": StructFmt.MSPROFTX_FMT,
        "GeStepBean": StructFmt.GE_STEP_FMT,
        "GeSessionInfoBean": StructFmt.GE_SESSION_FMT,
        "GeTensorBean": StructFmt.GE_TENSOR_FMT,
        "GeHostBean": StructFmt.GE_HOST_FMT,
        "GeLogicStreamInfoBean": StructFmt.GE_LOGIC_STREAM_INFO_FMT,
        "CaptureStreamInfoBean": StructFmt.CAPTURE_STREAM_INFO_FMT,
        "ModelWithQBean": StructFmt.HELPER_MODEL_WITH_Q_FMT,
        "StepTraceReader": StructFmt.STEP_TRACE_FMT,
        "StarsChipTransBean": StructFmt.CHIP_TRANS_FMT,
        "StarsChipTransV6Bean": StructFmt.CHIP_TRANS_V6_FMT,
        "LowPowerBean": StructFmt.LOWPOWER_FMT,
        "FlowBean": StructFmt.FLOW_FMT,
        "CyclesBean": StructFmt.CYCLES_FMT,
        "TaskTypeBean": StructFmt.TS_TASK_TYPE_FMT,
        "ApiDataBean": StructFmt.API_FMT,
        "EventDataBean": StructFmt.EVENT_FMT,
        "TaskTrackBean": StructFmt.TASK_TRACK_FMT,
        "TaskTrackChip6Bean": StructFmt.TASK_TRACK_FMT,
        "MemcpyInfoBean": StructFmt.MEMCPY_INFO_FMT,
        "HcclInfoBean": StructFmt.HCCL_INFO_FMT,
        "MultiThreadBean": StructFmt.MULTI_THREAD_FMT,
        "GraphAddInfoBean": StructFmt.GRAPH_ADD_INFO_FMT,
        "NodeBasicInfoBean": StructFmt.NODE_BASIC_INFO_FMT,
        "NodeAttrInfoBean": StructFmt.NODE_ATTR_INFO_FMT,
        "TensorAddInfoBean": StructFmt.TENSOR_ADD_INFO_FMT,
        "FusionAddInfoBean": StructFmt.FUSION_ADD_INFO_FMT,
        "CtxIdBean": StructFmt.CTX_ID_FMT,
        "MemoryApplicationBean": StructFmt.MEMORY_APPLICATION_FMT,
        "MemoryOpBean": StructFmt.MEMORY_OP_FMT,
        "TaskFlipBean": StructFmt.DEVICE_TASK_FLIP,
        "BlockNumBean": StructFmt.TS_BLOCK_NUM_FMT,
        "NpuModuleMemDataBean": StructFmt.NPU_MODULE_MEM_FMT,
        "HcclOpInfoBean": StructFmt.HCCL_OP_INFO_FMT,
        "StaticOpMemBean": StructFmt.STATIC_OP_MEM_FMT,
        "Mc2CommInfoBean": StructFmt.MC2_COMM_INFO_FMT,
        "AcsqTaskV6": StructFmt.ACSQ_TASK_V6_FMT,
        "BlockLogBean": StructFmt.BLOCK_LOG_FMT,
        "PmuBeanV6": StructFmt.PMU_BLOCK_FMT,
        "StarsQosBean": StructFmt.STARS_QOS_FMT,
        "AicoreSampleV6": StructFmt.AICORE_SAMPLE_V6_FMT,
        "BiuPerfInstructionBean": StructFmt.BIU_PERF_FMT,
        "RuntimeOpInfoBean": StructFmt.RUNTIME_OP_INFO_FMT,
        "RuntimeOpInfo256Bean": StructFmt.RUNTIME_OP_INFO_256_FMT,
        "V5ExeomBean": StructFmt.V5_MODEL_EXEOM_FMT,
        "V5StarsBean": StructFmt.V5_STARS_PROFILE_FMT,
        "DPUHcclTrackBean": StructFmt.DPU_HCCL_TRACK_FMT,
        "DPUTaskTrackBean": StructFmt.DPU_TASK_TRACK_FMT,
        "FusionTaskBean": StructFmt.FUSION_TASK_FMT,
    }

    @classmethod
    def decode(cls: any, binary_data: bytes, additional_fmt: str = "") -> any:
        """
        decode binary dato to class
        :param binary_data:
        :param additional_fmt:
        :return:
        """
        fmt = StructFmt.BYTE_ORDER_CHAR + cls.get_fmt() + additional_fmt
        return cls(struct.unpack_from(fmt, binary_data))

    @classmethod
    def get_fmt(cls: any) -> str:
        """
        get fmt
        """
        return cls.STRUCT_FMT_DICT.get(str(cls.__name__))
