# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

import logging
import struct

from common_func.ms_constant.ge_enum_constant import GeDataFormat
from common_func.ms_constant.ge_enum_constant import GeDataType
from common_func.ms_constant.ge_enum_constant import GeTaskType
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.struct_info.struct_decoder import StructDecoder

HEAD_FMT = "=II"
NAME_FMT = "s"
SHAPE_FMT = "Q"
NUM_FMT = "=I"
L1_OP_DESC_FMT = "=IIIIIBI"
L2_INPUT_DESC_FMT = "=IIIQQQI"
L2_OUTPUT_DESC_FMT = "=IIIIIIQQQI"


class MagicHeadBean(StructDecoder):
    """
    dbg magic head bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._magic_num = -1
        self._fmt_size = struct.calcsize(HEAD_FMT)

    @property
    def magic_num(self: any) -> int:
        """
        head data type
        """
        return self._magic_num

    @property
    def fmt_size(self: any) -> int:
        """
        type head's fmt size
        """
        return self._fmt_size

    def decode(self: any, data: bytes, offset: int):
        """
        decode the head data
        """
        self.construct_bean(struct.unpack(HEAD_FMT, data[offset : offset + self._fmt_size]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the head data
        """
        field = args[0]
        self._magic_num = field[1]


class TypeHeadBean(StructDecoder):
    """
    dbg type head bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._datatype = -1
        self._length = 0
        self._fmt_size = struct.calcsize(HEAD_FMT)

    @property
    def datatype(self: any) -> int:
        """
        head data type
        """
        return self._datatype

    @property
    def length(self: any) -> int:
        """
        head data length
        """
        return self._length

    @property
    def fmt_size(self: any) -> int:
        """
        type head's fmt size
        """
        return self._fmt_size

    def decode(self: any, data: bytes, offset: int):
        """
        decode the head data
        """
        self.construct_bean(struct.unpack(HEAD_FMT, data[offset : offset + self._fmt_size]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the head data
        """
        field = args[0]
        self._datatype = field[0]
        self._length = field[1]


class NameBean(StructDecoder):
    """
    dbg name bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._name = ""

    @property
    def name(self: any) -> str:
        """
        name data in dbg
        """
        return self._name

    def decode(self: any, data: bytes, offset: int, type_head: TypeHeadBean):
        """
        decode the name data by TyptHeadBean's length
        """
        self.construct_bean(
            struct.unpack(
                StructFmt.BYTE_ORDER_CHAR + str(type_head.length) + NAME_FMT, data[offset : offset + type_head.length]
            )
        )
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the name data
        """
        field = args[0]
        self._name = field[0].decode()


class ShapeBean(StructDecoder):
    """
    dbg shape bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._shape = ""
        self._count = 0

    @property
    def shape(self: any) -> str:
        """
        shape data in input/output
        """
        return self._shape

    def decode(self: any, data: bytes, offset: int, type_head: TypeHeadBean):
        """
        decode the shape bin data by TyptHeadBean's length
        """
        self._count = type_head.length // struct.calcsize(SHAPE_FMT)
        self.construct_bean(
            struct.unpack(
                StructFmt.BYTE_ORDER_CHAR + str(self._count) + SHAPE_FMT,
                data[offset : offset + self._count * struct.calcsize(SHAPE_FMT)],
            )
        )
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the shape data
        """
        if self._count < 1:
            self._shape = ""
            return

        field = args[0]
        for index in range(self._count - 1):
            self._shape += str(field[index]) + ","
        self._shape += str(field[self._count - 1])


class NumBean(StructDecoder):
    """
    dbg num bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._num = -1
        self._fmt_size = struct.calcsize(NUM_FMT)

    @property
    def num(self: any) -> int:
        """
        num data
        """
        return self._num

    @property
    def fmt_size(self: any) -> int:
        """
        num fmt size
        """
        return self._fmt_size

    def decode(self: any, data: bytes, offset: int):
        """
        decode the num bin data
        """
        self.construct_bean(struct.unpack(NUM_FMT, data[offset : offset + self._fmt_size]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the num data
        """
        field = args[0]
        self._num = field[0]


class L1OpDescBean(StructDecoder):
    """
    dbg op description bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._task_id = -1
        self._stream_id = -1
        self._task_type = -1
        self._block_num = -1
        self._length = 0
        self._fmt_size = struct.calcsize(L1_OP_DESC_FMT)

    @property
    def task_id(self: any) -> int:
        """
        op task_id data
        :return: task_id
        """
        return self._task_id

    @property
    def stream_id(self: any) -> int:
        """
        op stream_id data
        :return: stream_id
        """
        return self._stream_id

    @property
    def task_type(self: any) -> str:
        """
        op task_type data
        :return: task_type
        """
        task_type_dict = GeTaskType.member_map()
        if self._task_type not in task_type_dict:
            logging.error("Unsupported task_type %d", self._task_type)
            return str(self._task_type)
        return task_type_dict.get(self._task_type).name

    @property
    def block_num(self: any) -> int:
        """
        op block_num data
        :return: block_num
        """
        return self._block_num

    @property
    def length(self: any) -> int:
        """
        op description data's other info's length
        """
        return self._length

    @property
    def fmt_size(self: any) -> int:
        """
        op description's fmt size
        """
        return self._fmt_size

    def decode(self: any, data: bytes, offset: int):
        """
        decode the op description bin data
        """
        self.construct_bean(struct.unpack(L1_OP_DESC_FMT, data[offset : offset + self._fmt_size]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh op description data
        """
        field = args[0]
        self._task_id = field[0]
        self._stream_id = field[1]
        self._task_type = field[3]
        self._block_num = field[4]
        self._length = field[6]


class L2InputDescBean(StructDecoder):
    """
    dbg op input info bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._data_type = -1
        self._format = -1
        self._length = 0
        self._fmt_size = struct.calcsize(L2_INPUT_DESC_FMT)

    @property
    def data_type(self: any) -> str:
        """
        intput data type
        """
        data_type_dict = GeDataType.member_map()
        if self._data_type not in data_type_dict:
            logging.error("Unsupported input data_type %d", self._data_type)
            return str(self._data_type)
        return data_type_dict.get(self._data_type).name

    @property
    def format(self: any) -> str:
        """
        intput data format
        """
        format_dict = GeDataFormat.member_map()
        if self._format not in format_dict:
            logging.error("Unsupported input tensor format %d", self._format)
            return str(self._format)
        return format_dict.get(self._format).name

    @property
    def length(self: any) -> int:
        """
        input other info's length
        """
        return self._length

    @property
    def fmt_size(self: any) -> int:
        """
        input's fmt size
        """
        return self._fmt_size

    def decode(self: any, data: bytes, offset: int):
        """
        decode the input bin data
        """
        self.construct_bean(struct.unpack(L2_INPUT_DESC_FMT, data[offset : offset + self._fmt_size]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the input data
        """
        field = args[0]
        self._data_type = field[0]
        self._format = field[1]
        self._length = field[6]


class L2OutputDescBean(L2InputDescBean):
    """
    dbg op output info bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        super().__init__()
        self._fmt_size = struct.calcsize(L2_OUTPUT_DESC_FMT)

    @property
    def data_type(self: any) -> str:
        """
        output data type
        """
        data_type_dict = GeDataType.member_map()
        if self._data_type not in data_type_dict:
            logging.error("Unsupported output data_type %d", self._data_type)
            return str(self._data_type)
        return data_type_dict.get(self._data_type).name

    @property
    def format(self: any) -> str:
        """
        output data format
        """
        format_dict = GeDataFormat.member_map()
        if self._format not in format_dict:
            logging.error("Unsupported output tensor format %d", self._format)
            return str(self._format)
        return format_dict.get(self._format).name

    def decode(self: any, data: bytes, offset: int):
        """
        decode the output bin data
        """
        self.construct_bean(struct.unpack(L2_OUTPUT_DESC_FMT, data[offset : offset + self._fmt_size]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the output data
        """
        field = args[0]
        self._data_type = field[0]
        self._format = field[1]
        self._length = field[9]
