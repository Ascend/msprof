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
#ifndef MSPROFILER_PARSER_STRUCT_H_
#define MSPROFILER_PARSER_STRUCT_H_

#include <stdint.h>

#include <vector>

#include "analysis/csrc/infrastructure/utils/prof_struct.h"

enum class CompactInfoFormat : uint8_t
{
    COMPACT_INFO_TYPE = 0,
    RUNTIME_TRACK_TYPE,
    DPU_TRACK_TYPE,
    CAPTURE_STREAM_INFO_TYP,
    NODE_BASIC_INFO_TYPE,
    ATTR_INFO_TYPE,
    HCCL_OP_INFO_TYPE,
    MEMCPY_INFO_TYPE,
};

enum class AdditionalInfoFormat : uint8_t
{
    ADDITIONAL_INFO_TYPE = 0,
    CONTEXT_ID_INFO_TYPE,
    FUSION_OP_INFO_TYPE,
    GRAPH_ID_TYPE,
    TENSOR_INFO_TYPE,
    HCCL_INFO_TYPE,
    MEMORY_APPLICATION_TYPE,
    MULTI_THREAD_TYPE,
    TASK_MEMORY_INFO_TYPE,
};

enum class VariableInfoFormat : uint8_t
{
    VARIABLE_INFO_TYPE = 0,
    RUNTIME_OP_INFO_TYPE,
};

enum class RuntimeTrackFormat : uint8_t
{
    V1 = 0,
    V2,
};

enum class CaptureStreamFormat : uint8_t
{
    V1 = 0,
    V2,
};

struct ParserApi
{
    uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    uint16_t level;
    uint32_t type;
    uint32_t threadId;
    uint32_t reserve;
    uint64_t beginTime;
    uint64_t endTime;
    uint64_t itemId;
};

struct ParserEvent
{
    uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    uint16_t level;
    uint32_t type;
    uint32_t threadId;
    uint32_t requestId;  // 0xFFFF means single event
    uint64_t timeStamp;
    uint64_t reserve = MSPROF_EVENT_FLAG;
    uint64_t itemId;
};

struct ParserKernelInfo
{
    uint16_t numBlocks;
    uint16_t argsSize;
    uint8_t ratio;
    uint8_t schedMode;
};

struct ParserSimtKernelInfo
{
    MsprofDim3 gridDim;
    MsprofDim3 blockDim;
    uint16_t argsSize;
    uint8_t schedMode;
};

struct ParserModelInfo
{
    uint32_t modelId;
};

struct ParserEventInfo
{
    uint64_t key;
};

struct ParserNotifyInfo
{
    uint64_t key;
};

struct ParserRuntimeTrack
{  // for MsprofReportCompactInfo buffer data
    uint16_t deviceId;
    uint32_t streamId;
    uint32_t taskId;
    uint64_t taskType;
    uint64_t kernelName;
    struct ParserKernelInfo kernelInfo;
    struct ParserSimtKernelInfo simtKernelInfo;
    struct ParserModelInfo modelInfo;
    struct ParserEventInfo eventInfo;
    struct ParserNotifyInfo notifyInfo;
};

struct ParserDpuTrack
{                       // for MsprofReportCompactInfo buffer data
    uint16_t deviceId;  // high 4 bits, devType: dpu: 1, low 12 bits device id
    uint32_t streamId;
    uint32_t taskId;
    uint32_t taskType;  // task type enum
    uint32_t res;
    uint64_t startTime;  // start time
};

struct ParserCaptureStreamInfo
{
    uint16_t deviceId;
    uint16_t captureStatus;  //  0：创建 1：销毁
    uint32_t streamId;
    uint32_t originalStreamId;
    uint32_t modelId;
};

struct ParserHcclOPInfo
{  // for MsprofReportCompactInfo buffer data
    uint8_t relay;
    uint8_t retry;
    uint8_t dataType;
    uint64_t algType;  // 通信算子使用的算法,hash的key,其值是以"-"分隔的字符串
    uint64_t count;
    uint64_t groupName;
};

struct ParserMemcpyInfo
{                              // for MsprofReportCompactInfo buffer data
    uint64_t dataSize;         // 数据量大小， 字节
    uint64_t maxSize;          // 单个task拷贝的最大数据量
    uint16_t memcpyDirection;  // memcpy的方向
};

struct ParserNodeBasicInfo
{
    uint64_t opName;
    uint32_t taskType;
    uint64_t opType;
    uint32_t blockNum;
    uint32_t opFlag;
    uint8_t opState;
};

struct ParserAttrInfo
{
    uint64_t opName;
    uint32_t attrType;
    uint64_t hashId;
};

const uint16_t PARSER_COMPACT_INFO_DATA_LENGTH = 104;
struct ParserCompactInfo
{
    uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    uint16_t level;
    uint32_t type;
    uint32_t threadId;
    uint32_t dataLen;
    uint64_t timeStamp;
    union
    {
        uint8_t info[PARSER_COMPACT_INFO_DATA_LENGTH];
        ParserRuntimeTrack runtimeTrack;
        ParserDpuTrack dpuTrack;
        ParserCaptureStreamInfo captureStreamInfo;
        ParserNodeBasicInfo nodeBasicInfo;
        ParserAttrInfo nodeAttrInfo;
        ParserHcclOPInfo hcclopInfo;
        ParserMemcpyInfo memcpyInfo;
    } data;
};

struct ParserAicpuNodeAdditionalData
{
    uint16_t streamId;
    uint16_t taskId;
    uint32_t rev;
    uint64_t runStartTime;
    uint64_t runStartTick;
    uint64_t computeStartTime;
    uint64_t memcpyStartTime;
    uint64_t memcpyEndTime;
    uint64_t runEndTime;
    uint64_t runEndTick;
    uint32_t threadId;
    uint32_t deviceId;
    uint64_t submitTick;
    uint64_t scheduleTick;
    uint64_t tickBeforeRun;
    uint64_t tickAfterRun;
    uint32_t kernelType;
    uint32_t dispatchTime;
    uint32_t totalTime;
    uint16_t fftsThreadId;
    uint8_t version;
};

struct ParserAicpuDpAdditionalData
{
    char action[MSPROF_DP_DATA_ACTION_LEN];
    char source[MSPROF_DP_DATA_SOURCE_LEN];
    uint64_t index;
    uint64_t size;
};

struct ParserAicpuModelAdditionalData
{
    uint64_t indexId;
    uint32_t modelId;
    uint16_t tagId;
    uint16_t rsv1;
    uint64_t eventId;
};

struct ParserAicpuMiAdditionalData
{
    uint32_t nodeTag;  // MsprofMindsporeNodeTag:1
    uint32_t reserve;
    uint64_t queueSize;
    uint64_t runStartTime;
    uint64_t runEndTime;
};

struct ParserAicpuKfcProfCommTurn
{
    uint64_t serverStartTime;     // 进入KFC流程
    uint64_t waitMsgStartTime;    // 开始等待客户端消息
    uint64_t kfcAlgExeStartTime;  // 开始通信算法执行
    uint64_t sendTaskStartTime;   // 开始下发task
    uint64_t sendSqeFinishTime;   // task下发完成
    uint64_t rtsqExeEndTime;      // sq执行结束时间
    uint64_t serverEndTime;       // KFC流程结束时间
    uint64_t dataLen;             // 本轮通信数据长度
    uint32_t deviceId;
    uint16_t streamId;
    uint16_t taskId;
    uint8_t version;
    uint8_t commTurn;  // 总通信轮次
    uint8_t currentTurn;
    uint8_t reserve[5];
};

struct ParserAicpuKfcProfComputeTurn
{
    uint64_t waitComputeStartTime;  // 开始等待计算
    uint64_t computeStartTime;      // 开始计算
    uint64_t computeExeEndTime;     // 计算执行结束
    uint64_t dataLen;               // 本轮计算数据长度
    uint32_t deviceId;
    uint16_t streamId;
    uint16_t taskId;
    uint8_t version;
    uint8_t computeTurn;  // 总计算轮次
    uint8_t currentTurn;
    uint8_t reserve[5];
};

struct ParserAicpuFlipTask
{
    uint16_t streamId;
    uint16_t taskId;  // 值无特殊要求
    uint32_t flipNum;
    uint32_t reserve[2];
};

struct ParserAicpuHcclMainStreamTask
{
    uint16_t aicpuStreamId;
    uint16_t aicpuTaskId;
    uint16_t streamId;
    uint16_t taskId;
    uint16_t type;  // 0是头 1是尾
    uint16_t reserve[3];
};

struct ParserHcclInfo
{
    uint64_t itemId;
    uint64_t cclTag;
    uint64_t groupName;
    uint32_t localRank;
    uint32_t remoteRank;
    uint32_t rankSize;
    uint32_t workFlowMode;
    uint32_t planeID;
    uint32_t ctxID;
    uint64_t notifyID;
    uint32_t stage;
    uint32_t role;  // role {0: dst, 1:src}
    double durationEstimated;
    uint64_t srcAddr;
    uint64_t dstAddr;
    uint64_t dataSize;       // bytes
    uint32_t opType;         // {0: sum, 1: mul, 2: max, 3: min}
    uint32_t dataType;       // data type {0: INT8, 1: INT16, 2: INT32, 3: FP16, 4:FP32, 5:INT64, 6:UINT64}
    uint32_t linkType;       // link type {0: 'OnChip', 1: 'HCCS', 2: 'PCIe', 3: 'RoCE', 4: 'SIO'}
    uint32_t transportType;  // transport type {0: SDMA, 1: RDMA, 2:LOCAL}
    uint32_t rdmaType;       // RDMA type {0: RDMASendNotify, 1:RDMASendPayload}
    uint32_t reserve2;
};

struct ParserMultiThread
{
    uint32_t threadNum;
    uint32_t threadId[MSPROF_MULTI_THREAD_MAX_NUM];
};

struct ParserTensorData
{
    uint32_t tensorType;
    uint32_t format;
    uint32_t dataType;
    uint32_t shape[MSPROF_GE_TENSOR_DATA_SHAPE_LEN];
};

struct ParserTensorInfo
{
    uint64_t opName;
    uint32_t tensorNum;
    ParserTensorData tensorData[MSPROF_GE_TENSOR_DATA_NUM];
};

struct ParserConcatTensorInfo
{
    uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    uint16_t level = 0;
    uint32_t type = 0;
    uint32_t threadId = 0;
    uint32_t dataLen = 0;
    uint64_t timeStamp = 0;
    uint64_t opName = 0;
    uint32_t tensorNum = 0;
    std::vector<ParserTensorData> tensorData{MSPROF_GE_TENSOR_DATA_NUM};
};

struct ParserProfFusionOpInfo
{
    uint64_t opName;
    uint32_t fusionOpNum;
    uint64_t inputMemsize;
    uint64_t outputMemsize;
    uint64_t weightMemSize;
    uint64_t workspaceMemSize;
    uint64_t totalMemSize;
    uint64_t fusionOpId[MSPROF_GE_FUSION_OP_NUM];
};

struct ParserContextIdInfo
{
    uint64_t opName;
    uint32_t ctxIdNum;
    uint32_t ctxIds[MSPROF_CTX_ID_MAX_NUM];
};

struct ParserGraphIdInfo
{
    uint64_t modelName;
    uint32_t graphId;
    uint32_t modelId;
};

struct ParserMemoryInfo
{
    uint64_t addr;
    int64_t size;
    uint64_t nodeId;
    uint64_t totalAllocateMemory;
    uint64_t totalReserveMemory;
    uint32_t deviceId;
    uint32_t deviceType;
};

struct ParserStaticOpMem
{
    int64_t size;                  // op memory size
    uint64_t opName;               // op name hash id
    uint64_t lifeStart;            // serial number of op memory used
    uint64_t lifeEnd;              // serial number of op memory used
    uint64_t totalAllocateMemory;  // static graph total allocate memory
    uint64_t dynOpName;            // 0: invalid， other： dynamic op name of root
    uint32_t graphId;              // multiple model
};

struct ParserAicpuHCCLOPInfo
{
    uint8_t relay;       // 借轨通信
    uint8_t retry;       // 重传标识
    uint8_t dataType;    // 跟HcclDataType类型保存一致
    uint64_t algType;    // 通信算子使用的算法,hash的key,其值是以"-"分隔的字符串
    uint64_t count;      // 发送数据个数
    uint64_t groupName;  // group hash id
    uint32_t rankSize;
    uint16_t streamId;
    uint32_t taskId;
};

struct ParserAicpuHcclTaskInfo
{
    uint64_t itemId;
    uint64_t cclTag;
    uint64_t groupName;
    uint32_t localRank;
    uint32_t remoteRank;
    uint32_t rankSize;
    uint32_t stage;
    uint64_t notifyID;
    uint64_t timeStamp;
    double durationEstimated;
    uint64_t srcAddr;
    uint64_t dstAddr;
    uint64_t dataSize;  // bytes
    uint32_t taskId;
    uint32_t reserve;
    uint16_t streamId;
    uint16_t planeID;
    uint8_t opType;         // {0: sum, 1: mul, 2: max, 3: min}
    uint8_t dataType;       // data type {0: INT8, 1: INT16, 2: INT32, 3: FP16, 4:FP32, 5:INT64, 6:UINT64}
    uint8_t linkType;       // link type {0: 'OnChip', 1: 'HCCS', 2: 'PCIe', 3: 'RoCE'}
    uint8_t transportType;  // transport type {0: SDMA, 1: RDMA, 2:LOCAL}
    uint8_t rdmaType;       // RDMA type {0: RDMASendNotify, 1:RDMASendPayload}
    uint8_t role;           // role {0: dst, 1:src}
    uint8_t workFlowMode;
    uint8_t reserves[9];
};

struct ParserKfcInfos
{
    ParserAicpuHcclTaskInfo infos[KFC_INFOS_NUM];
};

struct ParserAdditionalInfo
{  // for MsprofReportAdditionalInfo buffer data
    uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    uint16_t level;
    uint32_t type;
    uint32_t threadId;
    uint32_t dataLen;
    uint64_t timeStamp;
    union
    {
        uint8_t data[MSPROF_ADDITIONAL_INFO_DATA_LENGTH];
        ParserAicpuNodeAdditionalData aicpuNode;
        ParserAicpuDpAdditionalData aicpuDp;
        ParserAicpuModelAdditionalData aicpuModel;
        ParserAicpuMiAdditionalData aicpuMi;
        ParserAicpuKfcProfCommTurn commTurn;
        ParserAicpuKfcProfComputeTurn computeTurn;
        ParserAicpuHCCLOPInfo opInfo;
        ParserAicpuFlipTask flipTask;
        ParserAicpuHcclMainStreamTask mainStreamTask;
        ParserKfcInfos kfcInfos;
        ParserHcclInfo hcclInfo;
        ParserMultiThread multiThread;
        ParserNodeBasicInfo nodeBasicInfo;
        ParserAttrInfo attrInfo;
        ParserTensorInfo tensorInfo;
        ParserProfFusionOpInfo fusionOpInfo;
        ParserContextIdInfo contextIdInfo;
        ParserGraphIdInfo graphIdInfo;
        ParserMemoryInfo memoryInfo;
    };
};

struct ParserVariableInfo
{  // for MsprofVariableInfo: 24 字节公共头 + dataLen
    uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    uint16_t level = 0;
    uint32_t type = 0;
    uint32_t threadId = 0;
    uint32_t dataLen = 0;
    uint64_t timeStamp = 0;
    std::vector<uint8_t> data;
};

#endif  // MSPROFILER_PARSER_STRUCT_H_
