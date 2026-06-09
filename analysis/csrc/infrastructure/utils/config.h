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

#ifndef ANALYSIS_DVVP_COMMON_CONFIG_CONFIG_H
#define ANALYSIS_DVVP_COMMON_CONFIG_CONFIG_H

#include <cstdint>
#include <map>
#include <string>

namespace analysis
{
namespace dvvp
{
namespace common
{
namespace config
{
// /////////////////////common//////////////////////////////////
const std::string DEVICE_APP_DIR = "/usr/local/profiler/";
const std::string PROF_SCRIPT_FILE_PATH = "/usr/bin/msprof_data_collection.sh";
const std::string PROF_MSPROF_PY_PATH = "profiler/profiler_tool/analysis/msprof/msprof.py";
const std::string PROF_MSPROF_PY_NAME = "msprof.py";
const std::string PROF_MSPROF_SO_NAME = "libmsprofiler.so";
const std::string PROF_MSPROF_BIN_NAME = "msprof";

const std::string CONTAINER_NO_SUPPORT_MESSAGE = "MESSAGE_CONTAINER_NO_SUPPORT";

const std::string ASCEND_WORK_PATH_ENV = "ASCEND_WORK_PATH";
const std::string ASCEND_MSPROF_WORK_PATH = "/profiling_data";

const char* const HOST_TAG_KEY = "Host";
const char* const DEVICE_TAG_KEY = "Device";
const char* const CLOCK_REALTIME_KEY = "clock_realtime";
const char* const CLOCK_MONOTONIC_RAW_KEY = "clock_monotonic_raw";
const char* const CLOCK_CNTVCT_KEY = "cntvct";
const char* const CLOCK_CNTVCT_KEY_DIFF = "cntvct_diff";

const std::string HOST_APP_DIR = "~/profiler-app";

const int MAX_PATH_LENGTH = 1024;

const int PROFILING_PACKET_MAX_LEN = (3 * 1024 * 1024);  // 3 * 1024 *1024 means 3mb

const int MSVP_BATCH_MAX_LEN = 2621440;  // 2621440 : 2.5 * 1024 *1024 means 2.5MB

const int RECEIVE_CHUNK_SIZE = 1024;  // chunk size:1024

const int HASH_DATA_MAX_LEN = 1024;  // hash data max len:1024

const int MSVP_COLLECT_CLIENT_INPUT_MAX_LEN = 128;  // 128 is max string size

const int MSVP_PROFILER_THREADNAME_MAXNUM = 16;

const int MSVP_DECODE_MESSAGE_MAX_LEN = (128 * 1024 * 1024);  // 128 * 1024 *1024 means 128mb

const int MSVP_MESSAGE_TYPE_NAME_MAX_LEN = 1024;  // 1024 means 1KB

const int MSVP_CFG_MAX_SIZE = 64 * 1024 * 1024;  // 64 * 1024 *1024 means 64mb

const long long MSVP_LARGE_FILE_MAX_LEN = 512 * 1024 * 1024;  // 512 * 1024 * 1024 means 512mb
const long long MSVP_SMALL_FILE_MAX_LEN = 2 * 1024 * 1024;    // 2 * 1024 * 1024 means 2mb

const int MSVP_CLN_SENDER_QUEUE_CAPCITY = 1024;
const int MSVP_CLN_SENDER_POOL_THREAD_NUM = 2;

const int STORAGE_LIMIT_DOWN_THD = 200;  // 200MB

const int MAX_CUSTOM_METRICS_LEN = 70;
const int HEX_MODE = 16;
const int DEC_MODE = 10;
const std::string CUSTOM_METRICS_VALID_HEADER = "Custom:";
const std::string NULL_CHUNK = "null";

const int MAX_ASCEND_INSTALL_INFO_FILE_SIZE = 1024;  // 1024 Byte

const int THOUSAND = 1000;  // 1000 : 1k

const int MS_TO_NS = 1000000;  // 1ms = 1000000ns
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
const int MSVP_MMPROCESS = NULL;
const char* const MSVP_ENV_DELIMITER = ":";
const std::string LIBRARY_PATH = "Path";
#else
const char* const MSVP_ENV_DELIMITER = ";";
const int MSVP_MMPROCESS = -1;
const std::string LIBRARY_PATH = "LD_LIBRARY_PATH";
#endif

const char* const MSVP_UNDERLINE = "_";

const int HOST_PID_DEFAULT = -1;

const std::string SYSTEM_ROOT = "systemroot";

const char* const MSVP_CHANNEL_POOL_NAME_PREFIX = "MSVP_ChanPool_";
const char* const MSVP_SENDER_POOL_NAME_PREFIX = "MSVP_SndPool_";

const char* const msvpChannelThreadName = "MSVP_ChanPoll";
const char* const MSVP_CTRL_RECEIVER_THREAD_NAME = "MSVP_CtrlRecv";
const char* const MSVP_UPLOADER_THREAD_NAME = "MSVP_Upld";
const char* const MSVP_DEVICE_TRANSPORT_THREAD_NAME = "MSVP_DevTrans";
const char* const MSVP_PROF_TASK_THREAD_NAME = "MSVP_ProfTask";
const char* const MSVP_DEVICE_THREAD_NAME_PREFIX = "MSVP_Dev_";
const char* const MSVP_COLLECT_SERVER_THREAD_NAME = "MSVP_CollectSrv";
const char* const MSVP_COLLECT_PERF_SCRIPT_THREAD_NAME = "MSVP_PerfScript";
const char* const MSVP_COLLECT_PROF_TIMER_THREAD_NAME = "MSVP_ProfTimer";
const char* const MSVP_CLOUD_TASK_THREAD_NAME = "MSVP_Task";
const char* const MSVP_HEART_BEAT_THREAD_NAME = "MSVP_HeartBeat";
const char* const MSVP_UPLOADER_DUMPER_THREAD_NAME = "MSVP_UploaderDumper";
const char* const MSVP_HDC_DUMPER_THREAD_NAME = "MSVP_HdcDumper";
const char* const MSVP_RPC_DUMPER_THREAD_NAME = "MSVP_RpcDumper";
const char* const MSVP_DYN_PROF_SERVER_THREAD_NAME = "MSVP_DynProfServer";
const char* const MSVP_DYN_PROF_CLIENT_THREAD_NAME = "MSVP_DynProfClient";
const char* const MSVP_HASH_DATA_UPLOAD_THREAD_NAME = "MSVP_hashDataUpload";
const char* const MSVP_TYPE_INFO_UPLOAD_THREAD_NAME = "MSVP_typeInfoUpload";
const char* const MSVP_MSTX_DATA_HANDLE_THREAD_NAME = "MSVP_MstxDataHandle";

const char* const MSVP_PROF_ACLAPI_MODE = "aclapi";
const char* const MSVP_PROF_SUBSCRIBE_MODE = "subscribe";
const char* const MSVP_PROF_SYSTEM_MODE = "system";

// cloud prof config
const std::string SAMPLE_KEY = "sample";
const std::string FILE_UNDERLINE = "_";
const int MAX_BEAT_TIME = 30;
const int MAX_DEVICE_NUMS = 8;

const size_t THREAD_QUEUE_SIZE_DEFAULT = 64;

// prof peripheral job config
const uint32_t DEFAULT_INTERVAL = 100;
const uint32_t DEFAULT_MASTER_ID = 0xFFFFFFFF;
const uint32_t PERIPHERAL_EVENT_READ = 0;
const uint32_t PERIPHERAL_EVENT_WRITE = 1;
const uint32_t PERIPHERAL_EVENT_NPU_APP_MEM = 0;
const uint32_t PERIPHERAL_EVENT_NPU_MEM = 1;
const int32_t PERIPHERAL_INTERVAL_MS_SMIN = 10;
const int32_t PERIPHERAL_INTERVAL_MS_MIN = 20;
const int32_t PERIPHERAL_INTERVAL_MS_MAX = 1000;

// prof job config
const char* const PROF_DEVICE_SYS_CPU_USAGE_FILE = "SystemCpuUsage.data";
const char* const PROF_DEVICE_SYS_MEM_USAGE_FILE = "Memory.data";
const char* const PROF_HOST_PID_CPU_USAGE_FILE = "host_cpu.data";
const char* const PROF_HOST_PID_MEM_USAGE_FILE = "host_mem.data";
const char* const PROF_HOST_SYS_CPU_USAGE_FILE = "host_sys_cpu.data";
const char* const PROF_HOST_SYS_MEM_USAGE_FILE = "host_sys_mem.data";
const char* const PROF_HOST_SYS_NETWORK_USAGE_FILE = "host_network.data";
const char* const PROF_NETDEV_STATS_FILE = "netdev_stats.data";
const char* const MSVP_PROF_DATA_DIR = "/data";
const char* const MSVP_PROF_PERF_DATA_FILE = "ai_ctrl_cpu.data.";
const char* const MSVP_PROF_PERF_RET_FILE_SUFFIX = ".txt";
const std::string PROF_AICORE_SAMPLE = "aicore sample based";
const std::string PROF_AIV_SAMPLE = "ai vector core sample based";
const std::string PROF_AICORE_TASK = "aicore task based";
const std::string PROF_AIV_TASK = "ai vector core task based";

// prof task config
const unsigned long PROCESS_WAIT_TIME = 500000;  // should not modify
const std::string PROF_TASK_STREAMTASK_QUEUE_NAME = "ProfTaskStreamBuffer";

// uploader config
const std::string UPLOADER_QUEUE_NAME = "UploaderQueue";
const size_t UPLOADER_QUEUE_CAPACITY = 512;

// prof channel config
const std::string SPEED_PERFCOUNT_MODULE_NAME = std::string("ChannelReaderSpeed");
const std::string SPEEDALL_PERFCOUNT_MODULE_NAME = std::string("ChannelReaderSpeedAll");
const size_t CHANNELPOLL_THREAD_QUEUE_SIZE = 8192;

// prof mgr config
const std::string PROF_FEATURE_TASK = "task_trace";
const int PROF_MGR_TRACE_ID_DEFAULT_LEN = 27;

// receive data config
const size_t MAX_LOOP_TIMES = 1400;                // the max send package nums of once Dump()
const int SLEEP_INTERVAL_US = 1000;                // the interval of Run()
const size_t RING_BUFF_CAPACITY = 16384;           // 16384:16K. Note:capacity value must be 2^n
const size_t VARIABLE_RING_BUFF_CAPACITY = 65536;  // 65536:64K. Note:capacity value must be 2^n
const size_t GE_RING_BUFF_CAPACITY = 262144;       // 262144:256K. Note:capacity value must be 2^n
const size_t MSPROF_RING_BUFF_CAPACITY = 16384;    // 16384:16K. Note:capacity value must be 2^n

// sender config
const int SEND_BUFFER_LEN = 64 * 1024;  // 64 * 1024 menas 64k
const size_t SENDERPOOL_THREAD_QUEUE_SIZE = 8192;

// transport config config
const std::string IDE_PERFCOUNT_MODULE_NAME = std::string("IdeTransport");
const std::string HDC_PERFCOUNT_MODULE_NAME = std::string("HdcTransport");
const std::string FILE_PERFCOUNT_MODULE_NAME = std::string("FileTransport");
const uint64_t TRANSPORT_PRI_FREQ = 128;

// prof switch
const std::string MSVP_PROF_ON = "on";
const std::string MSVP_PROF_OFF = "off";
const std::string MSVP_PROF_L0 = "l0";
const std::string MSVP_PROF_L1 = "l1";
const char* const MSVP_PROF_EMPTY_STRING = "";

// ai core metrics type
const std::string ARITHMETIC_UTILIZATION = "ArithmeticUtilization";
const std::string PIPE_UTILIZATION = "PipeUtilization";
const std::string MEMORY_BANDWIDTH = "Memory";
const std::string L0B_AND_WIDTH = "MemoryL0";
const std::string RESOURCE_CONFLICT_RATIO = "ResourceConflictRatio";
const std::string PROFILER_SAMPLE_CONFIG_ENV = "PROFILER_SAMPLECONFIG";
const std::string MEMORY_UB = "MemoryUB";
const std::string L2_CACHE = "L2Cache";
const std::string PIPE_UTILIZATION_EXCT = "PipeUtilizationExct";
const std::string PIPE_EXECUTION_UTILIZATION = "PipelineExecuteUtilization";
const std::string MEMORY_ACCESS = "MemoryAccess";
const std::string PROFILING_RESULT_PATH_ENV = "PROFILING_RESULT_PATH";
const std::string PROFILING_AICPU_MODE_ENV = "AICPU_PROFILING_MODE";
const std::string RANK_TABLE_FILE_ENV = "RANK_TABLE_FILE";
const std::string RANK_ID_ENV = "RANK_ID";
const std::string RANK_ENV = "RANK";
const std::string PROFILING_MODE_ENV = "PROFILING_MODE";
const std::string DYNAMIC_PROFILING_VALUE = "dynamic";
const std::string DELAY_DURATION_PROFILING_VALUE = "delay_or_duration";

// llc  profiling events type
const std::string LLC_PROFILING_CAPACITY = "capacity";
const std::string LLC_PROFILING_BANDWIDTH = "bandwidth";
const std::string LLC_PROFILING_READ = "read";
const std::string LLC_PROFILING_WRITE = "write";

// host sys events type
const std::string HOST_SYS_CPU = "cpu";
const std::string HOST_SYS_MEM = "mem";
const std::string HOST_SYS_DISK = "disk";
const std::string HOST_SYS_NETWORK = "network";
const std::string HOST_SYS_OSRT = "osrt";  // os_runtime: system call && pthread

const std::string OUTPUT_RECORD = "profiling_output_record";

// keypoint op's name/type
const char* const KEYPOINT_OP_NAME = "keypoint_op";
const char* const KEYPOINT_OP_TYPE = "na";

// hash data file tag
const std::string HASH_TAG = "hash_dic";
const char* const HASH_DIC_DELIMITER = ":";

// need paired ageing file
const char* const HWTS_DATA = "hwts.data";
const char* const AICORE_DATA = "aicore.data";
const char* const STORAGE_LIMIT_UNIT = "MB";

// analysis script param
const char* const DEFAULT_INTERATION_ID = "-1";
const char* const DEFAULT_MODEL_ID = "-1";
const char* const PROFILING_SUMMARY_FORMAT = "csv";
const char* const DEFAULT_PROFILING_EXPORT_TYPE = "text";

// used for init param
constexpr int DEFAULT_PROFILING_INTERVAL_5MS = 5;
constexpr int DEFAULT_PROFILING_INTERVAL_10MS = 10;
constexpr int DEFAULT_PROFILING_INTERVAL_20MS = 20;
constexpr int DEFAULT_PROFILING_INTERVAL_50MS = 50;
constexpr int DEFAULT_PROFILING_INTERVAL_100MS = 100;
constexpr int DEFAULT_PROFILING_INSTR_PROFILING_FREQ = 1000;  // 1000 cycle

enum FileChunkDataModule
{
    PROFILING_DEFAULT_DATA_MODULE = 0,
    PROFILING_IS_FROM_MSPROF,
    PROFILING_IS_CTRL_DATA,
    PROFILING_IS_FROM_DEVICE,
    PROFILING_IS_FROM_MSPROF_DEVICE,
    PROFILING_IS_FROM_MSPROF_HOST,
    PROFILING_IS_FROM_INNER
};

// device-sys
constexpr int SYS_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int SYS_SAMPLING_FREQ_MAX_NUM = 10;
constexpr int PID_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int PID_SAMPLING_FREQ_MAX_NUM = 10;
constexpr int CPU_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int CPU_SAMPLING_FREQ_MAX_NUM = 50;
constexpr int INTERCONNECTION_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int INTERCONNECTION_SAMPLING_FREQ_MAX_NUM = 50;
constexpr int IO_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int IO_SAMPLING_FREQ_MAX_NUM = 100;
constexpr int DVPP_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int DVPP_SAMPLING_FREQ_MAX_NUM = 100;
constexpr int HARDWARE_MEM_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int HARDWARE_MEM_SAMPLING_FREQ_MAX_NUM = 1000;
constexpr int AIC_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int AIC_SAMPLING_FREQ_MAX_NUM = 100;
constexpr int AIV_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int AIV_SAMPLING_FREQ_MAX_NUM = 100;
constexpr int L2_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int L2_SAMPLING_FREQ_MAX_NUM = 100;

constexpr uint32_t TLV_VALUE_MAX_LEN = 1200;
constexpr uint32_t TLV_VALUE_CHUNK_MAX_LEN = 1032;
constexpr uint32_t TLV_VALUE_FILENAME_MAX_LEN = 64;
constexpr uint32_t TLV_VALUE_EXTRAINFO_MAX_LEN = 64;
constexpr uint32_t TLV_VALUE_ID_MAX_LEN = 16;
constexpr uint32_t TLV_HEAD = 0x5A5A5A5AU;

struct ProfTlv
{
    uint32_t head;
    uint32_t version;
    uint32_t type;
    uint32_t len;
    uint8_t value[TLV_VALUE_MAX_LEN];
};

struct ProfTlvValue
{
    bool isLastChunk;
    int32_t chunkModule;
    size_t chunkSize;
    int64_t offset;
    char chunk[TLV_VALUE_CHUNK_MAX_LEN];
    char fileName[TLV_VALUE_FILENAME_MAX_LEN];
    char extraInfo[TLV_VALUE_EXTRAINFO_MAX_LEN];
    char id[TLV_VALUE_ID_MAX_LEN];
};
}  // namespace config
}  // namespace common
}  // namespace dvvp
}  // namespace analysis

#endif
