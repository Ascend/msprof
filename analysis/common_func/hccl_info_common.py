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
from enum import Enum
from enum import unique


class RoleType(Enum):
    DST = 0
    SRC = 1
    RESERVED = 255
    INVALID_TYPE = 4294967295


class OpType(Enum):
    SUM = 0
    MUL = 1
    MAX = 2
    MIN = 3
    RESERVED = 255
    INVALID_TYPE = 4294967295


@unique
class DataType(Enum):
    """
    Data type enum
    """

    INT8 = 0
    INT16 = 1
    INT32 = 2
    FP16 = 3
    FP32 = 4
    INT64 = 5
    UINT64 = 6
    UINT8 = 7
    UINT16 = 8
    UINT32 = 9
    FP64 = 10
    BFP16 = 11
    INT128 = 12
    HIF8 = 14
    FP8E4M3 = 15
    FP8E5M2 = 16
    FP8E8M0 = 17
    RESERVED = 255
    INVALID_TYPE = 4294967295

    @classmethod
    def member_map(cls: any) -> dict:
        """
        enum map for DataFormat value and data format member
        :return:
        """
        return cls._value2member_map_


class LinkType(Enum):
    ON_CHIP = 0
    HCCS = 1
    PCIE = 2
    ROCE = 3
    SIO = 4
    HCCS_SW = 5
    STANDARD_ROCE = 6
    UB = 7
    UBoE = 8
    RESERVED = 255
    INVALID_TYPE = 4294967295


class TransPortType(Enum):
    SDMA = 0
    RDMA = 1
    LOCAL = 2
    UB = 3
    ROCE = 4
    RESERVED = 255
    INVALID_TYPE = 4294967295


class RdmaType(Enum):
    RDMA_SEND_NOTIFY = 0
    RDMA_SEND_PAYLOAD = 1
    RESERVED = 255
    INVALID_TYPE = 4294967295


class AlgType(Enum):
    NONE = 0
    MESH = 1
    RING = 2
    NB = 3
    HD = 4
    NHR = 5
    PIPELINE = 6
    PAIRWISE = 7
    STAR = 8
    INVALID = 4294967295


class DeviceHcclSource(Enum):
    HCCL = 0
    MC2 = 1
    INVALID = 65535


@unique
class ReduceOpType(Enum):
    """
    DataType enum for reduce op
    """

    SUM = 0
    PROD = 1
    MAX = 2
    MIN = 3
    RESERVED = 255

    @classmethod
    def member_map(cls: any) -> dict:
        """
        enum map for DataFormat value and data format member
        :return:
        """
        return cls._value2member_map_


def trans_enum_name(enum_class, value):
    try:
        return enum_class(int(value)).name
    except ValueError:
        return value
