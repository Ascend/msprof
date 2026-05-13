/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "number_mapping.h"

#include "analysis/csrc/infrastructure/dfx/log.h"

namespace Analysis {
namespace Domain {
namespace {
    const std::unordered_map<uint32_t, std::string> geDataTypeMap {
        {0,   "FLOAT"},
        {1,   "FLOAT16"},
        {2,   "INT8"},
        {3,   "INT32"},
        {4,   "UINT8"},
        {6,   "INT16"},
        {7,   "UINT16"},
        {8,   "UINT32"},
        {9,   "INT64"},
        {10,  "UINT64"},
        {11,  "DOUBLE"},
        {12,  "BOOL"},
        {13,  "STRING"},
        {14,  "DUAL_SUB_INT8"},
        {15,  "DUAL_SUB_UINT8"},
        {16,  "COMPLEX64"},
        {17,  "COMPLEX128"},
        {18,  "QINT8"},
        {19,  "QINT16"},
        {20,  "QINT32"},
        {21,  "QUINT8"},
        {22,  "QUINT16"},
        {23,  "RESOURCE"},
        {24,  "STRING_REF"},
        {25,  "DUAL"},
        {26,  "DT_VARIANT"},
        {27,  "DT_BF16"},
        {28,  "DT_UNDEFINED"},
        {29,  "DT_INT4"},
        {30,  "DT_UINT1"},
        {31,  "DT_INT2"},
        {32,  "DT_UINT2"},
        {33,  "DT_COMPLEX32"},
        {34,  "DT_HIFLOAT8"},
        {35,  "DT_FLOAT8_E5M2"},
        {36,  "DT_FLOAT8_E4M3FN"},
        {37,  "DT_FLOAT8_E8M0"},
        {38,  "DT_FLOAT6_E3M2"},
        {39,  "DT_FLOAT6_E2M3"},
        {40,  "DT_FLOAT4_E2M1"},
        {41,  "DT_FLOAT4_E1M2"},
        {42,  "DT_MAX"},
        {229, "NUMBER_TYPE_BEGIN_"},
        {230, "BOOL_"},
        {231, "INT_"},
        {232, "INT8_"},
        {233, "INT16_"},
        {234, "INT32_"},
        {235, "INT64_"},
        {236, "UINT_"},
        {237, "UINT8_"},
        {238, "UINT16_"},
        {239, "UINT32_"},
        {240, "UINT64_"},
        {241, "FLOAT_"},
        {242, "FLOAT16_"},
        {243, "FLOAT32_"},
        {244, "FLOAT64_"},
        {245, "COMPLEX_"},
        {246, "NUMBER_TYPE_END_"},
        {UINT32_MAX, "UNDEFINED"},
    };

    const std::unordered_map<uint32_t, std::string> geFormatMap {
        {0,    "NCHW"},
        {1,    "NHWC"},
        {2,    "ND"},
        {3,    "NC1HWC0"},
        {4,    "FRACTAL_Z"},
        {5,    "NC1C0HWPAD"},
        {6,    "NHWC1C0"},
        {7,    "FSR_NCHW"},
        {8,    "FRACTAL_DECONV"},
        {9,    "C1HWNC0"},
        {10,   "FRACTAL_DECONV_TRANSPOSE"},
        {11,   "FRACTAL_DECONV_SP_STRIDE_TRANS"},
        {12,   "NC1HWC0_C04"},
        {13,   "FRACTAL_Z_C04"},
        {14,   "CHWN"},
        {15,   "FRACTAL_DECONV_SP_STRIDE8_TRANS"},
        {16,   "HWCN"},
        {17,   "NC1KHKWHWC0"},
        {18,   "BN_WEIGHT"},
        {19,   "FILTER_HWCK"},
        {20,   "HASHTABLE_LOOKUP_LOOKUPS"},
        {21,   "HASHTABLE_LOOKUP_KEYS"},
        {22,   "HASHTABLE_LOOKUP_VALUE"},
        {23,   "HASHTABLE_LOOKUP_OUTPUT"},
        {24,   "HASHTABLE_LOOKUP_HITS"},
        {25,   "C1HWNCoC0"},
        {26,   "MD"},
        {27,   "NDHWC"},
        {28,   "FRACTAL_ZZ"},
        {29,   "FRACTAL_NZ"},
        {30,   "NCDHW"},
        {31,   "DHWCN"},
        {32,   "NDC1HWC0"},
        {33,   "FRACTAL_Z_3D"},
        {34,   "CN"},
        {35,   "NC"},
        {36,   "DHWNC"},
        {37,   "FRACTAL_Z_3D_TRANSPOSE"},
        {38,   "FRACTAL_ZN_LSTM"},
        {39,   "FRACTAL_Z_G"},
        {40,   "RESERVED"},
        {41,   "ALL"},
        {42,   "NULL"},
        {43,   "ND_RNN_BIAS"},
        {44,   "FRACTAL_ZN_RNN"},
        {45,   "END"},
        {47,   "NCL"},
        {48,   "FRACTAL_Z_WINO"},
        {49,   "C1HWC0"},
        {50,   "FRACTAL_NZ_C0_16"},
        {51,   "FRACTAL_NZ_C0_32"},
        {52,   "FRACTAL_NZ_C0_2"},
        {53,   "FRACTAL_NZ_C0_4"},
        {54,   "FRACTAL_NZ_C0_8"},
        {55,   "END"},
        {0xff, "MAX"},
        {200,  "UNKNOWN_"},
        {201,  "DEFAULT_"},
        {202,  "NC1KHKWHWC0_"},
        {203,  "ND_"},
        {204,  "NCHW_"},
        {205,  "NHWC_"},
        {206,  "HWCN_"},
        {207,  "NC1HWC0_"},
        {208,  "FRAC_Z_"},
        {209,  "C1HWNCOC0_"},
        {210,  "FRAC_NZ_"},
        {211,  "NC1HWC0_C04_"},
        {212,  "FRACTAL_Z_C04_"},
        {213,  "NDHWC_"},
        {214,  "FRACTAL_ZN_LSTM_"},
        {215,  "FRACTAL_ZN_RNN_"},
        {216,  "ND_RNN_BIAS_"},
        {217,  "NDC1HWC0_"},
        {218,  "NCDHW_"},
        {219,  "FRACTAL_Z_3D_"},
        {220,  "DHWNC_"},
        {221,  "DHWCN_"},
        {UINT32_MAX, "UNDEFINED"},
    };
    const std::unordered_map<uint32_t, std::string> geTaskTypeMap {
        {0,  "AI_CORE"},
        {1,  "AI_CPU"},
        {2,  "AI_VECTOR_CORE"},
        {3,  "WRITE_BACK"},
        {4,  "MIX_AIC"},
        {5,  "MIX_AIV"},
        {6,  "FFTS_PLUS"},
        {7,  "DSA_SQE"},
        {8,  "DVPP"},
        {9,  "COMMUNICATION"},
        {10, "FUSION"},
        {11, "INVALID"},
        {256, "HCCL_AI_CPU"}
    };
    const std::unordered_map<uint32_t, std::string> hcclDataTypeMap {
        {0,          "INT8"},
        {1,          "INT16"},
        {2,          "INT32"},
        {3,          "FP16"},
        {4,          "FP32"},
        {5,          "INT64"},
        {6,          "UINT64"},
        {7,          "UINT8"},
        {8,          "UINT16"},
        {9,          "UINT32"},
        {10,         "FP64"},
        {11,         "BFP16"},
        {12,         "INT128"},
        {14,         "HIF8"},
        {15,         "FP8E4M3"},
        {16,         "FP8E5M2"},
        {17,         "FP8E8M0"},
        {255,        "RESERVED"},
        {4294967295, "INVALID_TYPE"},
    };
    const std::unordered_map<uint32_t, std::string> hcclLinkTypeMap {
        {0,          "ON_CHIP"},
        {1,          "HCCS"},
        {2,          "PCIE"},
        {3,          "ROCE"},
        {4,          "SIO"},
        {5,          "HCCS_SW"},
        {6,          "STANDARD_ROCE"},
        {7,          "UB"},
        {8,          "UBoE"},
        {255,        "RESERVED"},
        {4294967295, "INVALID_TYPE"}
    };
    const std::unordered_map<uint32_t, std::string> hcclTransportTypeMap {
        {0,          "SDMA"},
        {1,          "RDMA"},
        {2,          "LOCAL"},
        {3,          "UB"},
        {4,          "ROCE"},
        {255,        "RESERVED"},
        {4294967295, "INVALID_TYPE"},
    };
    const std::unordered_map<uint32_t, std::string> hcclRdmaTypeMap {
        {0,          "RDMA_SEND_NOTIFY"},
        {1,          "RDMA_SEND_PAYLOAD"},
        {255,        "RESERVED"},
        {4294967295, "INVALID_TYPE"}
    };
    const std::unordered_map<uint32_t, std::string> hcclOpTypeMap {
        {0,          "SUM"},
        {1,          "MUL"},
        {2,          "MAX"},
        {3,          "MIN"},
        {255,        "RESERVED"},
        {4294967295, "INVALID_TYPE"}
    };
    const std::unordered_map<uint32_t, std::string> levelMap {
        {30000, "pytorch"},
        {25000, "pta"},
        {20500, "msproftx"},
        {20000, "acl"},
        {15000, "model"},
        {10000, "node"},
        {5500,  "communication"},
        {5000,  "runtime"}
    };
    const std::unordered_map<uint32_t, std::string> aclApiTagMap {
        {1, "ACL_OP"},
        {2, "ACL_MODEL"},
        {3, "ACL_RTS"},
        {4, "ACL_OTHERS"},
        {5, "ACL_NN"},
        {6, "ACL_ASCENDC"},
        {7, "HOST_HCCL"},
        {9, "ACL_DVPP"},
        {10, "ACL_GRAPH"},
        {11, "ACL_ATB"},

    };
    std::vector<std::unordered_map<uint32_t, std::string>> allMaps {geDataTypeMap, geFormatMap, geTaskTypeMap,
                                                                    hcclDataTypeMap, hcclLinkTypeMap,
                                                                    hcclTransportTypeMap, hcclRdmaTypeMap,
                                                                    hcclOpTypeMap, levelMap,
                                                                    aclApiTagMap};

}

std::string NumberMapping::Get(MappingType type, uint32_t key)
{
    auto enumValue = static_cast<uint32_t>(type);
    if (allMaps.size() <= enumValue) {
        ERROR("wrong mapping type %, please check code", enumValue);
        return std::to_string(key);
    }
    if (allMaps.at(enumValue).find(key) == allMaps.at(enumValue).end()) {
        ERROR("Unknown key: %, type: %", key, enumValue);
        return std::to_string(key);
    }
    return allMaps.at(enumValue).at(key);
}
} // Domain
} // Analysis
