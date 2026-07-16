/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#ifndef ANALYSIS_APPLICATION_DB_CONSTANT_H
#define ANALYSIS_APPLICATION_DB_CONSTANT_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

#include "analysis/csrc/infrastructure/utils/prof_common.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Application
{

// mstx数据 host侧无device连线的默认connection_id值
constexpr uint32_t DEFAULT_CONNECTION_ID_MSTX = 4000000000;
// 以DEFAULT_CONNECTION_ID_MSTX + 1 作为mstx连线connection_id起点
constexpr uint32_t START_CONNECTION_ID_MSTX = DEFAULT_CONNECTION_ID_MSTX + 1;

// db name
const std::string DB_NAME_MSPROF_DB = "msprof";

// processor name & table name
const std::string PROCESSOR_NAME_STEP_TRACE = "STEP_TRACE";
const std::string PROCESSOR_NAME_STRING_IDS = "STRING_IDS";
const std::string TABLE_NAME_STRING_IDS = "STRING_IDS";

const std::string PROCESSOR_NAME_SESSION_TIME_INFO = "SESSION_TIME_INFO";
const std::string TABLE_NAME_SESSION_TIME_INFO = "SESSION_TIME_INFO";

const std::string PROCESSOR_NAME_NPU_INFO = "NPU_INFO";
const std::string TABLE_NAME_NPU_INFO = "NPU_INFO";
const std::string TABLE_NAME_RANK_DEVICE_MAP = "RANK_DEVICE_MAP";

const std::string PROCESSOR_NAME_HOST_INFO = "HOST_INFO";
const std::string TABLE_NAME_HOST_INFO = "HOST_INFO";

const std::string PROCESSOR_NAME_TASK = "TASK";
const std::string TABLE_NAME_TASK = "TASK";

const std::string PROCESSOR_NAME_COMPUTE_TASK_INFO = "COMPUTE_TASK_INFO";
const std::string TABLE_NAME_COMPUTE_TASK_INFO = "COMPUTE_TASK_INFO";
const std::string TABLE_NAME_COMMUNICATION_SCHEDULE_TASK_INFO = "COMMUNICATION_SCHEDULE_TASK_INFO";

const std::string PROCESSOR_NAME_METRIC_SUMMARY = "METRIC_SUMMARY";
const std::string TABLE_NAME_METRIC_SUMMARY = "METRIC_SUMMARY";

const std::string PROCESSOR_NAME_COMMUNICATION = "COMMUNICATION";
const std::string PROCESSOR_NAME_COMM_STATISTIC = "COMMUNICATION_STATISTIC";
const std::string TABLE_NAME_COMMUNICATION_TASK_INFO = "COMMUNICATION_TASK_INFO";
const std::string TABLE_NAME_COMMUNICATION_OP = "COMMUNICATION_OP";

const std::string PROCESSOR_NAME_OP_STATISTIC = "OP_STATISTIC";

const std::string PROCESSOR_NAME_API = "API";
const std::string TABLE_NAME_CANN_API = "CANN_API";

const std::string PROCESSOR_NAME_NPU_MEM = "NPU_MEM";
const std::string TABLE_NAME_NPU_MEM = "NPU_MEM";

const std::string PROCESSOR_NAME_NPU_MODULE_MEM = "NPU_MODULE_MEM";
const std::string TABLE_NAME_NPU_MODULE_MEM = "NPU_MODULE_MEM";

const std::string PROCESSOR_NAME_NPU_OP_MEM = "NPU_OP_MEM";
const std::string TABLE_NAME_NPU_OP_MEM = "NPU_OP_MEM";

const std::string PROCESSOR_NAME_ENUM = "ENUM";
const std::string TABLE_NAME_ENUM_API_TYPE = "ENUM_API_TYPE";
const std::string TABLE_NAME_ENUM_MODULE = "ENUM_MODULE";
const std::string TABLE_NAME_ENUM_HCCL_DATA_TYPE = "ENUM_HCCL_DATA_TYPE";
const std::string TABLE_NAME_ENUM_HCCL_LINK_TYPE = "ENUM_HCCL_LINK_TYPE";
const std::string TABLE_NAME_ENUM_HCCL_TRANSPORT_TYPE = "ENUM_HCCL_TRANSPORT_TYPE";
const std::string TABLE_NAME_ENUM_HCCL_RDMA_TYPE = "ENUM_HCCL_RDMA_TYPE";
const std::string TABLE_NAME_ENUM_OVERLAP_ANALYSIS_TYPE = "ENUM_OVERLAP_ANALYSIS_TYPE";

const std::string PROCESSOR_NAME_NIC = "NIC";
const std::string TABLE_NAME_NIC = "NIC";

const std::string PROCESSOR_NAME_ROCE = "ROCE";
const std::string TABLE_NAME_ROCE = "ROCE";

const std::string PROCESSOR_NAME_HBM = "HBM";
const std::string TABLE_NAME_HBM = "HBM";

const std::string PROCESSOR_NAME_DDR = "DDR";
const std::string TABLE_NAME_DDR = "DDR";

const std::string PROCESSOR_NAME_LLC = "LLC";
const std::string TABLE_NAME_LLC = "LLC";

const std::string PROCESSOR_NAME_PMU = "PMU";
const std::string PROCESSOR_NAME_UNIFIED_PMU = "UNIFIED_PMU";
const std::string PROCESSOR_NAME_TASK_PMU_INFO = "TASK_PMU_INFO";
const std::string PROCESSOR_NAME_SAMPLE_PMU_TIMELINE = "SAMPLE_PMU_TIMELINE";
const std::string PROCESSOR_NAME_SAMPLE_PMU_SUMMARY = "SAMPLE_PMU_SUMMARY";
const std::string TABLE_NAME_TASK_PMU_INFO = "TASK_PMU_INFO";
const std::string TABLE_NAME_SAMPLE_PMU_TIMELINE = "SAMPLE_PMU_TIMELINE";
const std::string TABLE_NAME_SAMPLE_PMU_SUMMARY = "SAMPLE_PMU_SUMMARY";

const std::string PROCESSOR_NAME_PCIE = "PCIE";
const std::string TABLE_NAME_PCIE = "PCIE";

const std::string PROCESSOR_NAME_HCCS = "HCCS";
const std::string TABLE_NAME_HCCS = "HCCS";

const std::string PROCESSOR_NAME_NETDEV_STATS = "NETDEV_STATS";
const std::string TABLE_NAME_NETDEV_STATS = "NETDEV_STATS";

const std::string PROCESSOR_NAME_ACC_PMU = "ACC_PMU";
const std::string TABLE_NAME_ACC_PMU = "ACC_PMU";

const std::string PROCESSOR_NAME_SOC = "SOC";
const std::string TABLE_NAME_SOC = "SOC_BANDWIDTH_LEVEL";

const std::string PROCESSOR_NAME_META_DATA = "META_DATA";
const std::string TABLE_NAME_META_DATA = "META_DATA";

const std::string PROCESSOR_NAME_AICORE_FREQ = "AICORE_FREQ";
const std::string TABLE_NAME_AICORE_FREQ = "AICORE_FREQ";

const std::string PROCESSOR_NAME_LOW_POWER = "LOW_POWER";
const std::string TABLE_NAME_LOW_POWER = "LOW_POWER";

const std::string PROCESSOR_NAME_MSTX = "MSTX";
const std::string TABLE_NAME_MSTX = "MSTX_EVENTS";
const std::string TABLE_NAME_MSTX_EVENT_TYPE = "ENUM_MSTX_EVENT_TYPE";

const std::string PROCESSOR_NAME_MEMCPY_INFO = "MEMCPY_INFO";
const std::string TABLE_NAME_MEMCPY_INFO = "MEMCPY_INFO";
const std::string TABLE_NAME_ENUM_MEMCPY_OPERATION = "ENUM_MEMCPY_OPERATION";

const std::string PROCESSOR_NAME_CHIP_TRAINS = "CHIP_TRAINS";
const std::string PROCESSOR_NAME_CHIP_TRAINS_V6 = "CHIP_TRAINS_V6";
const std::string TABLE_NAME_PA_LINK_INFO = "PA_LINK_INFO";
const std::string TABLE_NAME_PCIE_INFO = "ENUM_PCIE_INFO";
const std::string TABLE_NAME_PCIE_INFO_V6 = "ENUM_PCIE_INFO_V6";
const std::string PROCESSOR_NAME_KFC_TASK = "KFC_TASK";
const std::string PROCESSOR_NAME_KFC_COMM = "KFC_COMM";
const std::string PROCESSOR_NAME_DEVICE_TX = "DEVICE_MSTX";

const std::string PROCESSOR_NAME_SIO = "SIO";
const std::string TABLE_NAME_SIO = "SIO";

const std::string PROCESSOR_NAME_CPU_USAGE = "CPU_USAGE";
const std::string TABLE_NAME_CPU_USAGE = "CPU_USAGE";

const std::string PROCESSOR_NAME_MEM_USAGE = "MEM_USAGE";
const std::string TABLE_NAME_HOST_MEM_USAGE = "HOST_MEM_USAGE";

const std::string PROCESSOR_NAME_DISK_USAGE = "DISK_USAGE";
const std::string TABLE_NAME_HOST_DISK_USAGE = "HOST_DISK_USAGE";

const std::string PROCESSOR_NAME_NETWORK_USAGE = "NETWORK_USAGE";
const std::string TABLE_NAME_HOST_NETWORK_USAGE = "HOST_NETWORK_USAGE";

const std::string PROCESSOR_NAME_OSRT_API = "OSRT_API";
const std::string TABLE_NAME_OSRT_API = "OSRT_API";

const std::string PROCESSOR_NAME_HASH = "HASH_INIT";
const std::string PROCESSOR_NAME_OVERLAP_ANALYSIS = "OVERLAP_ANALYSIS";
const std::string TABLE_NAME_OVERLAP_ANALYSIS = "OVERLAP_ANALYSIS";
const std::string PROCESSOR_NAME_QOS = "QOS";
const std::string TABLE_NAME_QOS = "QOS";
const std::string PROCESSOR_MC2_COMM_INFO = "MC2_COMM_INFO";
const std::string PROCESSOR_NAME_NIC_TIMELINE = "NIC_TIMELINE";
const std::string PROCESSOR_NAME_ROCE_TIMELINE = "ROCE_TIMELINE";
const std::string AICPU_KERNEL = "AicpuKernel";
const std::string AIV_KERNEL = "AivKernel";
const std::string PROCESSOR_OP_SUMMARY = "OP_SUMMARY";
const std::string PROCESSOR_PMU = "PMU";
const std::string PROCESSOR_NAME_FUSION_OP = "FUSION_OP";
const std::string PROCESSOR_NAME_FUSION_TASK = "FUSION_TASK";
const std::string PROCESSOR_NAME_MODEL_NAME = "MODEL_NAME";
const std::string PROCESSOR_NAME_CCU_MISSION = "CCU_MISSION";
const std::string TABLE_NAME_CCU = "CCU";

const std::string PROCESSOR_TASK_TIME_SUMMARY = "TASK_TIME_SUMMARY";

const std::string PROCESSOR_HOST_TASK = "HOST_TASK";

const std::string PROCESSOR_NAME_BIU_PERF = "BIU_PERF";
const std::string PROCESSOR_NAME_UB = "UB";
const std::string TABLE_NAME_UB = "UB";

const std::string PROCESSOR_NAME_BLOCK_DETAIL = "BLOCK_DETAIL";
const std::string PROCESSOR_NAME_PAGE_FAULT = "PAGE_FAULT";

const std::string PROCESSOR_NAME_DPU = "DPU";
const std::string TABLE_NAME_DPU_TASK = "DPU_TASK";
// mstx event type
const std::unordered_map<std::string, uint16_t> MSTX_EVENT_TYPE_TABLE = {
    {"marker", 0}, {"push/pop", 1}, {"start/end", 2}, {"marker_ex", 3}};

// api level
const std::unordered_map<std::string, uint16_t> API_LEVEL_TABLE = {{"acl", MSPROF_REPORT_ACL_LEVEL},
                                                                   {"model", MSPROF_REPORT_MODEL_LEVEL},
                                                                   {"node", MSPROF_REPORT_NODE_LEVEL},
                                                                   {"communication", MSPROF_REPORT_HCCL_NODE_LEVEL},
                                                                   {"runtime", MSPROF_REPORT_RUNTIME_LEVEL}};

// npu module name
const std::unordered_map<std::string, uint16_t> MODULE_NAME_TABLE = {
    {"SLOG", 0},        {"IDEDD", 1},     {"SCC", 2},
    {"HCCL", 3},        {"FMK", 4},       {"CCU", 5},
    {"DVPP", 6},        {"RUNTIME", 7},   {"CCE", 8},
    {"HDC", 9},         {"DRV", 10},      {"NET", 11},
    {"DEVMM", 22},      {"KERNEL", 23},   {"LIBMEDIA", 24},
    {"CCECPU", 25},     {"ROS", 27},      {"HCCP", 28},
    {"ROCE", 29},       {"TEFUSION", 30}, {"PROFILING", 31},
    {"DP", 32},         {"APP", 33},      {"TS", 34},
    {"TSDUMP", 35},     {"AICPU", 36},    {"LP", 37},
    {"TDT", 38},        {"FE", 39},       {"MD", 40},
    {"MB", 41},         {"ME", 42},       {"IMU", 43},
    {"IMP", 44},        {"GE", 45},       {"CAMERA", 47},
    {"ASCENDCL", 48},   {"TEEOS", 49},    {"ISP", 50},
    {"SIS", 51},        {"HSM", 52},      {"DSS", 53},
    {"PROCMGR", 54},    {"BBOX", 55},     {"AIVECTOR", 56},
    {"TBE", 57},        {"FV", 58},       {"TUNE", 60},
    {"HSS", 61},        {"FFTS", 62},     {"OP", 63},
    {"UDF", 64},        {"HICAID", 65},   {"TSYNC", 66},
    {"AUDIO", 67},      {"TPRT", 68},     {"ASCENDCKERNEL", 69},
    {"ASYS", 70},       {"ATRACE", 71},   {"RTC", 72},
    {"SYSMONITOR", 73}, {"AMP", 74},      {"ADETECT", 75},
    {"MBUFF", 76},      {"CUSTOM", 77}};

// hccl info DataType
const std::unordered_map<std::string, uint16_t> HCCL_DATA_TYPE_TABLE = {
    {"INT8", 0},     {"INT16", 1},    {"INT32", 2},      {"FP16", 3},          {"FP32", 4},
    {"INT64", 5},    {"UINT64", 6},   {"UINT8", 7},      {"UINT16", 8},        {"UINT32", 9},
    {"FP64", 10},    {"BFP16", 11},   {"INT128", 12},    {"HIF8", 14},         {"FP8E4M3", 15},
    {"FP8E5M2", 16}, {"FP8E8M0", 17}, {"RESERVED", 255}, {NA, UINT16_MAX - 1}, {"INVALID_TYPE", UINT16_MAX},
};

// hccl info LinkType
const std::unordered_map<std::string, uint16_t> HCCL_LINK_TYPE_TABLE = {
    {"ON_CHIP", 0}, {"HCCS", 1},       {"PCIE", 2},          {"ROCE", 3},
    {"SIO", 4},     {"HCCS_SW", 5},    {"STANDARD_ROCE", 6}, {"UB", 7},
    {"UBoE", 8},    {"RESERVED", 255}, {NA, UINT16_MAX - 1}, {"INVALID_TYPE", UINT16_MAX},
};

// hccl info TransPortType
const std::unordered_map<std::string, uint16_t> HCCL_TRANSPORT_TYPE_TABLE = {
    {"SDMA", 0}, {"RDMA", 1},       {"LOCAL", 2},         {"UB", 3},
    {"ROCE", 4}, {"RESERVED", 255}, {NA, UINT16_MAX - 1}, {"INVALID_TYPE", UINT16_MAX},
};

// hccl info RdmaType
const std::unordered_map<std::string, uint16_t> HCCL_RDMA_TYPE_TABLE = {
    {"RDMA_SEND_NOTIFY", 0}, {"RDMA_SEND_PAYLOAD", 1},     {"RESERVED", 255},
    {NA, UINT16_MAX - 1},    {"INVALID_TYPE", UINT16_MAX},
};

// TS为stars时芯片的sqetype
const std::map<std::string, std::string> STARS_SQE_TYPE_TABLE{
    {"0", "AI_CORE"},          {"1", "AI_CPU"},         {"2", "AIV_SQE"},           {"3", "PLACE_HOLDER_SQE"},
    {"4", "EVENT_RECORD_SQE"}, {"5", "EVENT_WAIT_SQE"}, {"6", "NOTIFY_RECORD_SQE"}, {"7", "NOTIFY_WAIT_SQE"},
    {"8", "WRITE_VALUE_SQE"},  {"9", "VQ6_SQE"},        {"10", "TOF_SQE"},          {"11", "SDMA_SQE"},
    {"12", "VPC_SQE"},         {"13", "JPEGE_SQE"},     {"14", "JPEGD_SQE"},        {"15", "DSA_SQE"},
    {"16", "ROCCE_SQE"},       {"17", "PCIE_DMA_SQE"},  {"18", "HOST_CPU_SQE"},     {"19", "CDQM_SQE"},
    {"20", "C_CORE_SQE"}};

// TS为hwts时芯片的sqetype
const std::map<std::string, std::string> HW_SQE_TYPE_TABLE{
    {"0", "AI_CORE"},          {"1", "AI_CPU"},         {"2", "AIV_SQE"},           {"3", "PLACE_HOLDER_SQE"},
    {"4", "EVENT_RECORD_SQE"}, {"5", "EVENT_WAIT_SQE"}, {"6", "NOTIFY_RECORD_SQE"}, {"7", "NOTIFY_WAIT_SQE"},
    {"8", "WRITE_VALUE_SQE"},  {"9", "SDMA_SQE"},       {"10", "MAX_SQE"}};

// 拷贝类型
const std::unordered_map<std::string, uint16_t> MEMCPY_OPERATION_TABLE{
    {"host to host", 0},      {"host to device", 1},    {"device to host", 2},
    {"device to device", 3},  {"managed memory", 4},    {"addr device to device", 5},
    {"host to device ex", 6}, {"device to host ex", 7}, {"other", UINT16_MAX}};

// overlap analysis 类型
const std::unordered_map<std::string, uint16_t> OVERLAP_TYPE_TABLE{
    {"COMPUTE", 0}, {"COMMUNICATION", 1}, {"COMM_NOT_OVERLAP_COMP", 2}, {"FREE", 3}, {"RESERVE", UINT16_MAX}};

const std::unordered_map<std::string, std::unordered_map<std::string, uint16_t>>

    ENUM_TABLE = {{TABLE_NAME_ENUM_API_TYPE, API_LEVEL_TABLE},
                  {TABLE_NAME_ENUM_MODULE, MODULE_NAME_TABLE},
                  {TABLE_NAME_ENUM_HCCL_DATA_TYPE, HCCL_DATA_TYPE_TABLE},
                  {TABLE_NAME_ENUM_HCCL_LINK_TYPE, HCCL_LINK_TYPE_TABLE},
                  {TABLE_NAME_ENUM_HCCL_TRANSPORT_TYPE, HCCL_TRANSPORT_TYPE_TABLE},
                  {TABLE_NAME_ENUM_HCCL_RDMA_TYPE, HCCL_RDMA_TYPE_TABLE},
                  {TABLE_NAME_MSTX_EVENT_TYPE, MSTX_EVENT_TYPE_TABLE},
                  {TABLE_NAME_ENUM_MEMCPY_OPERATION, MEMCPY_OPERATION_TABLE},
                  {TABLE_NAME_ENUM_OVERLAP_ANALYSIS_TYPE, OVERLAP_TYPE_TABLE}};

const std::string SCHEMA_VERSION_MAJOR = "1";
const std::string SCHEMA_VERSION_MINOR = "2";
const std::string SCHEMA_VERSION_MICRO = "0";
const std::string SCHEMA_VERSION = Utils::Join(".", SCHEMA_VERSION_MAJOR, SCHEMA_VERSION_MINOR, SCHEMA_VERSION_MICRO);
// 版本号相关 SCHEMA_VERSION
// SCHEMA_VERSION_MAJOR，对于数据库的整体格式，只有在整体db格式发生重大重写或重构时，进行更改
// SCHEMA_VERSION_MINOR，表示存在非兼容性修改，需要做适配。例如，当表名或字段类型发生变动时。
// SCHEMA_VERSION_MICRO，表示版本变动，每次变动代表支持兼容性新特性，对原有功能不存在修改，不存在影响和中断原有脚本和数据读取
const std::unordered_map<std::string, std::string> META_DATA = {{NAME_STR(SCHEMA_VERSION), SCHEMA_VERSION},
                                                                {NAME_STR(SCHEMA_VERSION_MAJOR), SCHEMA_VERSION_MAJOR},
                                                                {NAME_STR(SCHEMA_VERSION_MINOR), SCHEMA_VERSION_MINOR},
                                                                {NAME_STR(SCHEMA_VERSION_MICRO), SCHEMA_VERSION_MICRO}};

const std::unordered_map<uint16_t, std::string> CHIP_TABLE = {
    {0, "Ascend310"}, {1, "Ascend910A"}, {4, "Ascend310P"}, {5, "Ascend910B"}, {7, "Ascend310B"}, {15, "Ascend950"},
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_DB_CONSTANT_H
