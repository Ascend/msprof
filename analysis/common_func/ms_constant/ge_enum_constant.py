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


@unique
class GeDataType(Enum):
    """
    DataType enum for ge graph operator data
    """

    FLOAT = 0
    FLOAT16 = 1
    INT8 = 2
    INT32 = 3
    UINT8 = 4
    INT16 = 6
    UINT16 = 7
    UINT32 = 8
    INT64 = 9
    UINT64 = 10
    DOUBLE = 11
    BOOL = 12
    STRING = 13
    DUAL_SUB_INT8 = 14
    DUAL_SUB_UINT8 = 15
    COMPLEX64 = 16
    COMPLEX128 = 17
    QINT8 = 18
    QINT16 = 19
    QINT32 = 20
    QUINT8 = 21
    QUINT16 = 22
    RESOURCE = 23
    STRING_REF = 24
    DUAL = 25
    DT_VARIANT = 26
    DT_BF16 = 27
    DT_UNDEFINED = 28
    DT_INT4 = 29
    DT_UINT1 = 30
    DT_INT2 = 31
    DT_UINT2 = 32
    DT_COMPLEX32 = 33
    DT_HIFLOAT8 = 34
    DT_FLOAT8_E5M2 = 35
    DT_FLOAT8_E4M3FN = 36
    DT_FLOAT8_E8M0 = 37
    DT_FLOAT6_E3M2 = 38
    DT_FLOAT6_E2M3 = 39
    DT_FLOAT4_E2M1 = 40
    DT_FLOAT4_E1M2 = 41
    DT_HIFLOAT4 = 42
    DT_MAX = 43
    NUMBER_TYPE_BEGIN_ = 229
    BOOL_ = 230
    INT_ = 231
    INT8_ = 232
    INT16_ = 233
    INT32_ = 234
    INT64_ = 235
    UINT_ = 236
    UINT8_ = 237
    UINT16_ = 238
    UINT32_ = 239
    UINT64_ = 240
    FLOAT_ = 241
    FLOAT16_ = 242
    FLOAT32_ = 243
    FLOAT64_ = 244
    COMPLEX_ = 245
    NUMBER_TYPE_END_ = 246
    UNDEFINED = 0xFFFFFFFF  # 4294967295(-1) 默认值

    @classmethod
    def member_map(cls: any) -> dict:
        """
        enum map for DataFormat value and data format member
        :return:
        """
        return cls._value2member_map_


@unique
class GeDataFormat(Enum):
    """
    DataFormat enum for ge graph operator data
    """

    NCHW = 0
    NHWC = 1
    ND = 2
    NC1HWC0 = 3
    FRACTAL_Z = 4
    NC1C0HWPAD = 5
    NHWC1C0 = 6
    FSR_NCHW = 7
    FRACTAL_DECONV = 8
    C1HWNC0 = 9
    FRACTAL_DECONV_TRANSPOSE = 10
    FRACTAL_DECONV_SP_STRIDE_TRANS = 11
    NC1HWC0_C04 = 12
    FRACTAL_Z_C04 = 13
    CHWN = 14
    FRACTAL_DECONV_SP_STRIDE8_TRANS = 15
    HWCN = 16
    NC1KHKWHWC0 = 17
    BN_WEIGHT = 18
    FILTER_HWCK = 19
    HASHTABLE_LOOKUP_LOOKUPS = 20
    HASHTABLE_LOOKUP_KEYS = 21
    HASHTABLE_LOOKUP_VALUE = 22
    HASHTABLE_LOOKUP_OUTPUT = 23
    HASHTABLE_LOOKUP_HITS = 24
    C1HWNCoC0 = 25
    MD = 26
    NDHWC = 27
    FRACTAL_ZZ = 28
    FRACTAL_NZ = 29
    NCDHW = 30
    DHWCN = 31
    NDC1HWC0 = 32
    FRACTAL_Z_3D = 33
    CN = 34
    NC = 35
    DHWNC = 36
    FRACTAL_Z_3D_TRANSPOSE = 37
    FRACTAL_ZN_LSTM = 38
    FRACTAL_Z_G = 39
    RESERVED = 40
    ALL = 41
    NULL = 42
    ND_RNN_BIAS = 43
    FRACTAL_ZN_RNN = 44
    NYUV = 45
    NYUV_A = 46
    NCL = 47
    FRACTAL_Z_WINO = 48
    C1HWC0 = 49
    FRACTAL_NZ_C0_16 = 50
    FRACTAL_NZ_C0_32 = 51
    FRACTAL_NZ_C0_2 = 52
    FRACTAL_NZ_C0_4 = 53
    FRACTAL_NZ_C0_8 = 54
    END = 55
    MAX = 0xFF  # 255
    UNKNOWN_ = 200
    DEFAULT_ = 201
    NC1KHKWHWC0_ = 202
    ND_ = 203
    NCHW_ = 204
    NHWC_ = 205
    HWCN_ = 206
    NC1HWC0_ = 207
    FRAC_Z_ = 208
    C1HWNCOC0_ = 209
    FRAC_NZ_ = 210
    NC1HWC0_C04_ = 211
    FRACTAL_Z_C04_ = 212
    NDHWC_ = 213
    FRACTAL_ZN_LSTM_ = 214
    FRACTAL_ZN_RNN_ = 215
    ND_RNN_BIAS_ = 216
    NDC1HWC0_ = 217
    NCDHW_ = 218
    FRACTAL_Z_3D_ = 219
    DHWNC_ = 220
    DHWCN_ = 221
    UNDEFINED = 0xFFFFFFFF  # 4294967295(-1) 默认值

    @classmethod
    def member_map(cls: any) -> dict:
        """
        enum map for DataFormat value and data format member
        :return:
        """
        return cls._value2member_map_


@unique
class GeTaskType(Enum):
    """
    TaskType enum for ge graph operator data
    """

    AI_CORE = 0
    AI_CPU = 1
    AI_VECTOR_CORE = 2
    WRITE_BACK = 3
    MIX_AIC = 4
    MIX_AIV = 5
    FFTS_PLUS = 6
    DSA_SQE = 7
    DVPP = 8
    HCCL = 9
    FUSION = 10
    INVALID = 11

    @classmethod
    def member_map(cls: any) -> dict:
        """
        enum map for TaskType member
        :return:
        """
        return cls._value2member_map_
