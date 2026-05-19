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

#include "analysis/csrc/viewer/database/finals/msprof_db.h"

#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis
{
namespace Viewer
{
namespace Database
{

namespace
{
const TableColumns STRING_IDS = {{"id", SQL_INTEGER_TYPE, true}, {"value", SQL_TEXT_TYPE}};

const TableColumns SESSION_TIME_INFO = {
    {"startTimeNs", SQL_INTEGER_TYPE},
    {"endTimeNs", SQL_INTEGER_TYPE},
};

const TableColumns NPU_INFO = {{"id", SQL_INTEGER_TYPE}, {"name", SQL_TEXT_TYPE}};

const TableColumns RANK_DEVICE_MAP = {{"rankId", SQL_INTEGER_TYPE}, {"deviceId", SQL_INTEGER_TYPE}};

const TableColumns HOST_INFO = {{"hostUid", SQL_TEXT_TYPE}, {"hostName", SQL_TEXT_TYPE}};

const TableColumns TASK = {
    {"startNs", SQL_INTEGER_TYPE},      {"endNs", SQL_INTEGER_TYPE},        {"deviceId", SQL_INTEGER_TYPE},
    {"connectionId", SQL_INTEGER_TYPE}, {"globalTaskId", SQL_INTEGER_TYPE}, {"globalPid", SQL_INTEGER_TYPE},
    {"taskType", SQL_INTEGER_TYPE},     {"contextId", SQL_INTEGER_TYPE},    {"streamId", SQL_INTEGER_TYPE},
    {"taskId", SQL_INTEGER_TYPE},       {"modelId", SQL_INTEGER_TYPE}};

const TableColumns COMPUTE_TASK_INFO = {{"name", SQL_INTEGER_TYPE},
                                        {"globalTaskId", SQL_INTEGER_TYPE, true},
                                        {"blockNum", SQL_INTEGER_TYPE},
                                        {"mixBlockNum", SQL_INTEGER_TYPE},
                                        {"taskType", SQL_INTEGER_TYPE},
                                        {"opType", SQL_INTEGER_TYPE},
                                        {"inputFormats", SQL_INTEGER_TYPE},
                                        {"inputDataTypes", SQL_INTEGER_TYPE},
                                        {"inputShapes", SQL_INTEGER_TYPE},
                                        {"outputFormats", SQL_INTEGER_TYPE},
                                        {"outputDataTypes", SQL_INTEGER_TYPE},
                                        {"outputShapes", SQL_INTEGER_TYPE},
                                        {"attrInfo", SQL_INTEGER_TYPE},
                                        {"opState", SQL_INTEGER_TYPE},
                                        {"hf32Eligible", SQL_INTEGER_TYPE}};

const TableColumns COMMUNICATION_SCHEDULE_TASK_INFO = {{"name", SQL_INTEGER_TYPE},
                                                       {"globalTaskId", SQL_INTEGER_TYPE, true},
                                                       {"taskType", SQL_INTEGER_TYPE},
                                                       {"opType", SQL_INTEGER_TYPE}};

const TableColumns COMMUNICATION_TASK_INFO = {
    {"name", SQL_INTEGER_TYPE},          {"globalTaskId", SQL_INTEGER_TYPE}, {"taskType", SQL_INTEGER_TYPE},
    {"planeId", SQL_INTEGER_TYPE},       {"groupName", SQL_INTEGER_TYPE},    {"notifyId", SQL_INTEGER_TYPE},
    {"rdmaType", SQL_INTEGER_TYPE},      {"srcRank", SQL_INTEGER_TYPE},      {"dstRank", SQL_INTEGER_TYPE},
    {"transportType", SQL_INTEGER_TYPE}, {"size", SQL_INTEGER_TYPE},         {"dataType", SQL_INTEGER_TYPE},
    {"linkType", SQL_INTEGER_TYPE},      {"opId", SQL_INTEGER_TYPE},         {"isMaster", SQL_INTEGER_TYPE},
    {"bandwidth", SQL_NUMERIC_TYPE},
};

const TableColumns COMMUNICATION_OP = {
    {"opName", SQL_INTEGER_TYPE},       {"startNs", SQL_INTEGER_TYPE},   {"endNs", SQL_INTEGER_TYPE},
    {"connectionId", SQL_INTEGER_TYPE}, {"groupName", SQL_INTEGER_TYPE}, {"opId", SQL_INTEGER_TYPE, true},
    {"relay", SQL_INTEGER_TYPE},        {"retry", SQL_INTEGER_TYPE},     {"dataType", SQL_INTEGER_TYPE},
    {"algType", SQL_INTEGER_TYPE},      {"count", SQL_NUMERIC_TYPE},     {"opType", SQL_INTEGER_TYPE},
    {"deviceId", SQL_INTEGER_TYPE},     {"rankSize", SQL_INTEGER_TYPE}};

const TableColumns CANN_API = {{"startNs", SQL_INTEGER_TYPE},
                               {"endNs", SQL_INTEGER_TYPE},
                               {"type", SQL_INTEGER_TYPE},
                               {"globalTid", SQL_INTEGER_TYPE},
                               {"connectionId", SQL_INTEGER_TYPE, true},
                               {"name", SQL_INTEGER_TYPE}};

const TableColumns NPU_MEM = {{"type", SQL_INTEGER_TYPE},
                              {"ddr", SQL_NUMERIC_TYPE},
                              {"hbm", SQL_NUMERIC_TYPE},
                              {"timestampNs", SQL_INTEGER_TYPE},
                              {"deviceId", SQL_INTEGER_TYPE}};

const TableColumns NPU_MODULE_MEM = {{"moduleId", SQL_INTEGER_TYPE},
                                     {"timestampNs", SQL_INTEGER_TYPE},
                                     {"totalReserved", SQL_NUMERIC_TYPE},
                                     {"deviceId", SQL_INTEGER_TYPE}};

const TableColumns NPU_OP_MEM = {{"operatorName", SQL_INTEGER_TYPE},  {"addr", SQL_INTEGER_TYPE},
                                 {"type", SQL_INTEGER_TYPE},          {"size", SQL_INTEGER_TYPE},
                                 {"timestampNs", SQL_INTEGER_TYPE},   {"globalTid", SQL_INTEGER_TYPE},
                                 {"totalAllocate", SQL_NUMERIC_TYPE}, {"totalReserve", SQL_NUMERIC_TYPE},
                                 {"component", SQL_INTEGER_TYPE},     {"deviceId", SQL_INTEGER_TYPE}};

const TableColumns SYS_IO = {
    {"deviceId", SQL_INTEGER_TYPE},     {"timestampNs", SQL_INTEGER_TYPE}, {"bandwidth", SQL_INTEGER_TYPE},
    {"rxPacketRate", SQL_NUMERIC_TYPE}, {"rxByteRate", SQL_NUMERIC_TYPE},  {"rxPackets", SQL_INTEGER_TYPE},
    {"rxBytes", SQL_INTEGER_TYPE},      {"rxErrors", SQL_INTEGER_TYPE},    {"rxDropped", SQL_INTEGER_TYPE},
    {"txPacketRate", SQL_NUMERIC_TYPE}, {"txByteRate", SQL_NUMERIC_TYPE},  {"txPackets", SQL_INTEGER_TYPE},
    {"txBytes", SQL_INTEGER_TYPE},      {"txErrors", SQL_INTEGER_TYPE},    {"txDropped", SQL_INTEGER_TYPE},
    {"funcId", SQL_INTEGER_TYPE}};

const TableColumns HBM = {{"deviceId", SQL_INTEGER_TYPE},
                          {"timestampNs", SQL_INTEGER_TYPE},
                          {"bandwidth", SQL_NUMERIC_TYPE},
                          {"hbmId", SQL_INTEGER_TYPE},
                          {"type", SQL_INTEGER_TYPE}};

const TableColumns DDR = {{"deviceId", SQL_INTEGER_TYPE},
                          {"timestampNs", SQL_INTEGER_TYPE},
                          {"read", SQL_NUMERIC_TYPE},
                          {"write", SQL_NUMERIC_TYPE}};

const TableColumns LLC = {
    {"deviceId", SQL_INTEGER_TYPE}, {"llcId", SQL_INTEGER_TYPE},      {"timestampNs", SQL_INTEGER_TYPE},
    {"hitRate", SQL_REAL_TYPE},     {"throughput", SQL_INTEGER_TYPE}, {"mode", SQL_INTEGER_TYPE},
};

const TableColumns TASK_PMU_INFO = {
    {"globalTaskId", SQL_INTEGER_TYPE}, {"name", SQL_INTEGER_TYPE}, {"value", SQL_NUMERIC_TYPE}};

const TableColumns SAMPLE_PMU_TIMELINE = {
    {"deviceId", SQL_INTEGER_TYPE}, {"timestampNs", SQL_INTEGER_TYPE}, {"totalCycle", SQL_INTEGER_TYPE},
    {"usage", SQL_NUMERIC_TYPE},    {"freq", SQL_NUMERIC_TYPE},        {"coreId", SQL_INTEGER_TYPE},
    {"coreType", SQL_INTEGER_TYPE},
};

const TableColumns SAMPLE_PMU_SUMMARY = {
    {"deviceId", SQL_INTEGER_TYPE}, {"metric", SQL_INTEGER_TYPE},   {"value", SQL_NUMERIC_TYPE},
    {"coreId", SQL_INTEGER_TYPE},   {"coreType", SQL_INTEGER_TYPE},
};

const TableColumns PCIE = {{"deviceId", SQL_INTEGER_TYPE},
                           {"timestampNs", SQL_INTEGER_TYPE},
                           {"txPostMin", SQL_NUMERIC_TYPE},
                           {"txPostMax", SQL_NUMERIC_TYPE},
                           {"txPostAvg", SQL_NUMERIC_TYPE},
                           {"txNonpostMin", SQL_NUMERIC_TYPE},
                           {"txNonpostMax", SQL_NUMERIC_TYPE},
                           {"txNonpostAvg", SQL_NUMERIC_TYPE},
                           {"txCplMin", SQL_NUMERIC_TYPE},
                           {"txCplMax", SQL_NUMERIC_TYPE},
                           {"txCplAvg", SQL_NUMERIC_TYPE},
                           {"txNonpostLatencyMin", SQL_NUMERIC_TYPE},
                           {"txNonpostLatencyMax", SQL_NUMERIC_TYPE},
                           {"txNonpostLatencyAvg", SQL_NUMERIC_TYPE},
                           {"rxPostMin", SQL_NUMERIC_TYPE},
                           {"rxPostMax", SQL_NUMERIC_TYPE},
                           {"rxPostAvg", SQL_NUMERIC_TYPE},
                           {"rxNonpostMin", SQL_NUMERIC_TYPE},
                           {"rxNonpostMax", SQL_NUMERIC_TYPE},
                           {"rxNonpostAvg", SQL_NUMERIC_TYPE},
                           {"rxCplMin", SQL_NUMERIC_TYPE},
                           {"rxCplMax", SQL_NUMERIC_TYPE},
                           {"rxCplAvg", SQL_NUMERIC_TYPE}};

const TableColumns HCCS = {{"deviceId", SQL_INTEGER_TYPE},
                           {"timestampNs", SQL_INTEGER_TYPE},
                           {"txThroughput", SQL_NUMERIC_TYPE},
                           {"rxThroughput", SQL_NUMERIC_TYPE}};

const TableColumns NETDEV_STATS = {
    {"deviceId", SQL_INTEGER_TYPE},       {"timestampNs", SQL_INTEGER_TYPE},    {"macTxPfcPkt", SQL_INTEGER_TYPE},
    {"macRxPfcPkt", SQL_INTEGER_TYPE},    {"macTxByte", SQL_INTEGER_TYPE},      {"macTxBandwidth", SQL_NUMERIC_TYPE},
    {"macRxByte", SQL_INTEGER_TYPE},      {"macRxBandwidth", SQL_NUMERIC_TYPE}, {"macTxBadByte", SQL_INTEGER_TYPE},
    {"macRxBadByte", SQL_INTEGER_TYPE},   {"roceTxPkt", SQL_INTEGER_TYPE},      {"roceRxPkt", SQL_INTEGER_TYPE},
    {"roceTxErrPkt", SQL_INTEGER_TYPE},   {"roceRxErrPkt", SQL_INTEGER_TYPE},   {"roceTxCnpPkt", SQL_INTEGER_TYPE},
    {"roceRxCnpPkt", SQL_INTEGER_TYPE},   {"roceNewPktRty", SQL_INTEGER_TYPE},  {"nicTxByte", SQL_INTEGER_TYPE},
    {"nicTxBandwidth", SQL_NUMERIC_TYPE}, {"nicRxByte", SQL_INTEGER_TYPE},      {"nicRxBandwidth", SQL_NUMERIC_TYPE}};

const TableColumns ENUM_TABLE = {{"id", SQL_INTEGER_TYPE, true}, {"name", SQL_TEXT_TYPE}};

const TableColumns ACC_PMU = {{"accId", SQL_INTEGER_TYPE},         {"readBwLevel", SQL_INTEGER_TYPE},
                              {"writeBwLevel", SQL_INTEGER_TYPE},  {"readOstLevel", SQL_INTEGER_TYPE},
                              {"writeOstLevel", SQL_INTEGER_TYPE}, {"timestampNs", SQL_NUMERIC_TYPE},
                              {"deviceId", SQL_INTEGER_TYPE}};

const TableColumns SOC_BANDWIDTH_LEVEL = {{"l2BufferBwLevel", SQL_INTEGER_TYPE},
                                          {"mataBwLevel", SQL_INTEGER_TYPE},
                                          {"timestampNs", SQL_NUMERIC_TYPE},
                                          {"deviceId", SQL_INTEGER_TYPE}};

const TableColumns META_DATA = {{"name", SQL_TEXT_TYPE}, {"value", SQL_TEXT_TYPE}};

const TableColumns AICORE_FREQ = {
    {"deviceId", SQL_INTEGER_TYPE}, {"timestampNs", SQL_NUMERIC_TYPE}, {"freq", SQL_INTEGER_TYPE}};

const TableColumns MSTX = {{"startNs", SQL_INTEGER_TYPE},   {"endNs", SQL_INTEGER_TYPE},
                           {"eventType", SQL_INTEGER_TYPE}, {"rangeId", SQL_INTEGER_TYPE},
                           {"category", SQL_INTEGER_TYPE},  {"message", SQL_INTEGER_TYPE},
                           {"globalTid", SQL_INTEGER_TYPE}, {"endGlobalTid", SQL_INTEGER_TYPE},
                           {"domainId", SQL_INTEGER_TYPE},  {"connectionId", SQL_INTEGER_TYPE}};

const TableColumns MEMCPY_INFO = {
    {"globalTaskId", SQL_NUMERIC_TYPE}, {"size", SQL_NUMERIC_TYPE}, {"memcpyOperation", SQL_NUMERIC_TYPE}};

const TableColumns CPU_USAGE = {
    {"timestampNs", SQL_NUMERIC_TYPE}, {"cpuId", SQL_NUMERIC_TYPE}, {"usage", SQL_NUMERIC_TYPE}};

const TableColumns HOST_MEM_USAGE = {{"timestampNs", SQL_NUMERIC_TYPE}, {"usage", SQL_NUMERIC_TYPE}};

const TableColumns HOST_DISK_USAGE = {{"timestampNs", SQL_NUMERIC_TYPE},
                                      {"readRate", SQL_NUMERIC_TYPE},
                                      {"writeRate", SQL_NUMERIC_TYPE},
                                      {"usage", SQL_NUMERIC_TYPE}};

const TableColumns HOST_NETWORK_USAGE = {
    {"timestampNs", SQL_NUMERIC_TYPE}, {"usage", SQL_NUMERIC_TYPE}, {"speed", SQL_NUMERIC_TYPE}};

const TableColumns OSRT_API = {{"name", SQL_NUMERIC_TYPE},
                               {"globalTid", SQL_NUMERIC_TYPE},
                               {"startNs", SQL_NUMERIC_TYPE},
                               {"endNs", SQL_NUMERIC_TYPE}};

const TableColumns QOS = {{"deviceId", SQL_INTEGER_TYPE},
                          {"eventName", SQL_NUMERIC_TYPE},
                          {"bandwidth", SQL_NUMERIC_TYPE},
                          {"timestampNs", SQL_NUMERIC_TYPE}};
}  // namespace

MsprofDB::MsprofDB()
{
    dbName_ = "msprof.db";
    tableColNames_ = {{TABLE_NAME_STRING_IDS, STRING_IDS},
                      {TABLE_NAME_SESSION_TIME_INFO, SESSION_TIME_INFO},
                      {TABLE_NAME_NPU_INFO, NPU_INFO},
                      {TABLE_NAME_RANK_DEVICE_MAP, RANK_DEVICE_MAP},
                      {TABLE_NAME_HOST_INFO, HOST_INFO},
                      {TABLE_NAME_TASK, TASK},
                      {TABLE_NAME_COMPUTE_TASK_INFO, COMPUTE_TASK_INFO},
                      {TABLE_NAME_COMMUNICATION_SCHEDULE_TASK_INFO, COMMUNICATION_SCHEDULE_TASK_INFO},
                      {TABLE_NAME_COMMUNICATION_TASK_INFO, COMMUNICATION_TASK_INFO},
                      {TABLE_NAME_COMMUNICATION_OP, COMMUNICATION_OP},
                      {TABLE_NAME_CANN_API, CANN_API},
                      {TABLE_NAME_NPU_MEM, NPU_MEM},
                      {TABLE_NAME_NPU_MODULE_MEM, NPU_MODULE_MEM},
                      {TABLE_NAME_NPU_OP_MEM, NPU_OP_MEM},
                      {TABLE_NAME_NIC, SYS_IO},
                      {TABLE_NAME_ROCE, SYS_IO},
                      {TABLE_NAME_HBM, HBM},
                      {TABLE_NAME_DDR, DDR},
                      {TABLE_NAME_LLC, LLC},
                      {TABLE_NAME_TASK_PMU_INFO, TASK_PMU_INFO},
                      {TABLE_NAME_SAMPLE_PMU_TIMELINE, SAMPLE_PMU_TIMELINE},
                      {TABLE_NAME_SAMPLE_PMU_SUMMARY, SAMPLE_PMU_SUMMARY},
                      {TABLE_NAME_PCIE, PCIE},
                      {TABLE_NAME_HCCS, HCCS},
                      {TABLE_NAME_NETDEV_STATS, NETDEV_STATS},
                      {TABLE_NAME_ACC_PMU, ACC_PMU},
                      {TABLE_NAME_SOC, SOC_BANDWIDTH_LEVEL},
                      {TABLE_NAME_META_DATA, META_DATA},
                      {TABLE_NAME_AICORE_FREQ, AICORE_FREQ},
                      {TABLE_NAME_MSTX, MSTX},
                      {TABLE_NAME_MEMCPY_INFO, MEMCPY_INFO},
                      {TABLE_NAME_CPU_USAGE, CPU_USAGE},
                      {TABLE_NAME_HOST_MEM_USAGE, HOST_MEM_USAGE},
                      {TABLE_NAME_HOST_DISK_USAGE, HOST_DISK_USAGE},
                      {TABLE_NAME_HOST_NETWORK_USAGE, HOST_NETWORK_USAGE},
                      {TABLE_NAME_OSRT_API, OSRT_API},
                      {TABLE_NAME_QOS, QOS},
                      // ENUM
                      {TABLE_NAME_ENUM_API_TYPE, ENUM_TABLE},
                      {TABLE_NAME_ENUM_MODULE, ENUM_TABLE},
                      {TABLE_NAME_ENUM_HCCL_DATA_TYPE, ENUM_TABLE},
                      {TABLE_NAME_ENUM_HCCL_LINK_TYPE, ENUM_TABLE},
                      {TABLE_NAME_ENUM_HCCL_TRANSPORT_TYPE, ENUM_TABLE},
                      {TABLE_NAME_ENUM_HCCL_RDMA_TYPE, ENUM_TABLE},
                      {TABLE_NAME_MSTX_EVENT_TYPE, ENUM_TABLE},
                      {TABLE_NAME_ENUM_MEMCPY_OPERATION, ENUM_TABLE}};
}

}  // namespace Database
}  // namespace Viewer
}  // namespace Analysis
