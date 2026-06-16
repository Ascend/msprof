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
#ifndef MSPROFILER_PROF_COMMON_H_
#define MSPROFILER_PROF_COMMON_H_

#include <stdint.h>

#include <vector>

#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus

#define MSPROF_DATA_HEAD_MAGIC_NUM 0x5a5a
#define MSPROF_EVENT_FLAG 0xFFFFFFFFFFFFFFFFULL

    enum MsprofDataTag
    {
        MSPROF_ACL_DATA_TAG = 0,             // acl data tag, range: 0~19
        MSPROF_GE_DATA_TAG_MODEL_LOAD = 20,  // ge data tag, range: 20~39
        MSPROF_GE_DATA_TAG_FUSION = 21,
        MSPROF_GE_DATA_TAG_INFER = 22,
        MSPROF_GE_DATA_TAG_TASK = 23,
        MSPROF_GE_DATA_TAG_TENSOR = 24,
        MSPROF_GE_DATA_TAG_STEP = 25,
        MSPROF_GE_DATA_TAG_ID_MAP = 26,
        MSPROF_GE_DATA_TAG_HOST_SCH = 27,
        MSPROF_RUNTIME_DATA_TAG_API = 40,  // runtime data tag, range: 40~59
        MSPROF_RUNTIME_DATA_TAG_TRACK = 41,
        MSPROF_AICPU_DATA_TAG = 60,  // aicpu data tag, range: 60~79
        MSPROF_AICPU_MODEL_TAG = 61,
        MSPROF_HCCL_DATA_TAG = 80,       // hccl data tag, range: 80~99
        MSPROF_DP_DATA_TAG = 100,        // dp data tag, range: 100~119
        MSPROF_MSPROFTX_DATA_TAG = 120,  // msproftx data tag, range: 120~139
        MSPROF_DATA_TAG_MAX = 65536,     // data tag value type is uint16_t
    };

#define PATH_LEN_MAX 1023
#define PARAM_LEN_MAX 4095
    struct MsprofCommandHandleParams
    {
        uint32_t pathLen;
        uint32_t storageLimit;  // MB
        uint32_t profDataLen;
        char path[PATH_LEN_MAX + 1];
        char profData[PARAM_LEN_MAX + 1];
    };

/**
 * @brief profiling command info
 */
#define MSPROF_MAX_DEV_NUM 64
    struct MsprofCommandHandle
    {
        uint64_t profSwitch;
        uint64_t profSwitchHi;
        uint32_t devNums;
        uint32_t devIdList[MSPROF_MAX_DEV_NUM];
        uint32_t modelId;
        uint32_t type;
        uint32_t cacheFlag;
        struct MsprofCommandHandleParams params;
    };

#define MSPROF_GE_TENSOR_DATA_SHAPE_LEN 8
#define MSPROF_GE_TENSOR_DATA_NUM 5
#define MSPROF_GE_FUSION_OP_NUM 8
#define MSPROF_CTX_ID_MAX_NUM 55
#pragma pack(1)
    struct MsprofNodeBasicInfo
    {
        uint64_t opName;
        uint32_t taskType;
        uint64_t opType;
        uint32_t blockNum;
        uint32_t opFlag;
        uint8_t opState;
    };

    enum AttrType
    {
        OP_ATTR = 0,
    };

    struct MsprofAttrInfo
    {
        uint64_t opName;
        uint32_t attrType;
        uint64_t hashId;
    };

    struct MsrofTensorData
    {
        uint32_t tensorType;
        uint32_t format;
        uint32_t dataType;
        uint32_t shape[MSPROF_GE_TENSOR_DATA_SHAPE_LEN];
    };

    struct MsprofTensorInfo
    {
        uint64_t opName;
        uint32_t tensorNum;
        MsrofTensorData tensorData[MSPROF_GE_TENSOR_DATA_NUM];
    };

    struct ProfFusionOpInfo
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

    struct MsprofContextIdInfo
    {
        uint64_t opName;
        uint32_t ctxIdNum;
        uint32_t ctxIds[MSPROF_CTX_ID_MAX_NUM];
    };

    struct MsprofGraphIdInfo
    {
        uint64_t modelName;
        uint32_t graphId;
        uint32_t modelId;
    };

    struct MsprofMemoryInfo
    {
        uint64_t addr;
        int64_t size;
        uint64_t nodeId;
        uint64_t totalAllocateMemory;
        uint64_t totalReserveMemory;
        uint32_t deviceId;
        uint32_t deviceType;
    };

    struct MsprofStaticOpMem
    {
        int64_t size;                  // op memory size
        uint64_t opName;               // op name hash id
        uint64_t lifeStart;            // serial number of op memory used
        uint64_t lifeEnd;              // serial number of op memory used
        uint64_t totalAllocateMemory;  // static graph total allocate memory
        uint64_t dynOpName;            // 0: invalid， other： dynamic op name of root
        uint32_t graphId;              // multiple model
    };

/**
 * @name  MsprofStampInfo
 * @brief struct of data reported by msproftx
 */
#define UIF_VALUE_LEN 2
#define MAX_MESSAGE_LEN 156
    struct MsprofStampInfo
    {
        uint16_t magicNumber;
        uint16_t dataTag;
        uint32_t processId;
        uint32_t threadId;
        uint32_t category;  // marker category
        uint32_t eventType;
        int32_t payloadType;
        union PayloadValue
        {
            uint64_t ullValue;
            int64_t llValue;
            double dValue;
            uint32_t uiValue[UIF_VALUE_LEN];
            int32_t iValue[UIF_VALUE_LEN];
            float fValue[UIF_VALUE_LEN];
        } payload;  // payload info for marker
        uint64_t startTime;
        uint64_t endTime;
        uint64_t markId;
        uint64_t domain;
        int32_t messageType;
        char message[MAX_MESSAGE_LEN];
    };

#define MSPROF_TX_VALUE_MAX_LEN 224  // 224 + 8 = 232: additional data len
    struct MsprofTxInfo
    {
        uint16_t infoType;  // 0: Mark; 1: MarkEx
        uint16_t res0;
        uint32_t res1;
        union
        {
            struct MsprofStampInfo stampInfo;
            uint8_t data[MSPROF_TX_VALUE_MAX_LEN];
        } value;
    };

#pragma pack()

/**
 * @brief struct of data reported by HCCL
 */
#pragma pack(4)
    struct MsprofHcclInfo
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

    const uint16_t MSPROF_MULTI_THREAD_MAX_NUM = 25;
    struct MsprofMultiThread
    {
        uint32_t threadNum;
        uint32_t threadId[MSPROF_MULTI_THREAD_MAX_NUM];
    };
#pragma pack()

    /* Msprof report level */
    const uint16_t MSPROF_REPORT_PYTORCH_LEVEL = 30000;
    const uint16_t MSPROF_REPORT_PTA_LEVEL = 25000;
    const uint16_t MSPROF_REPORT_TX_LEVEL = 20500;
    const uint16_t MSPROF_REPORT_ACL_LEVEL = 20000;
    const uint16_t MSPROF_REPORT_MODEL_LEVEL = 15000;
    const uint16_t MSPROF_REPORT_NODE_LEVEL = 10000;
    const uint16_t MSPROF_REPORT_HCCL_NODE_LEVEL = 5500;
    const uint16_t MSPROF_REPORT_RUNTIME_LEVEL = 5000;

    /* Msprof report type of pytorch(30000) level(proftx), offset: 0 */
    const uint32_t MSPROF_REPORT_PYTORCH_PROFTX_TYPE = 0;
    const uint32_t MSPROF_REPORT_PYTORCH_CATEGORY_DIC_TYPE = 1;
    const uint32_t MSPROF_REPORT_PYTORCH_CALLSTACK_TYPE = 2;
    const uint32_t MSPROF_REPORT_PYTORCH_CANN_OP_TYPE = 3;
    const uint32_t MSPROF_REPORT_PYTORCH_TORCH_OP_TYPE = 4;
    const uint32_t MSPROF_REPORT_PYTORCH_PIPELINE_TYPE = 5;

    /* Msprof report type of tx(20500) level, offset: 0x000000 */
    const uint32_t MSPROF_REPORT_TX_BASE_TYPE = 0x000000U;

    /* Msprof report type of acl(20000) level(acl), offset: 0x020000 */
    const uint32_t MSPROF_REPORT_ACL_OP_BASE_TYPE = 0x010000U;
    const uint32_t MSPROF_REPORT_ACL_MODEL_BASE_TYPE = 0x020000U;
    const uint32_t MSPROF_REPORT_ACL_RUNTIME_BASE_TYPE = 0x030000U;
    const uint32_t MSPROF_REPORT_ACL_OTHERS_BASE_TYPE = 0x040000U;

    /* Msprof report type of acl(20000) level(host api hccl), offset: 0x070000 */
    const uint32_t MSPROF_REPORT_ACL_NN_BASE_TYPE = 0x050000U;
    const uint32_t MSPROF_REPORT_ACL_ASCENDC_TYPE = 0x060000U;
    const uint32_t MSPROF_REPORT_ACL_HOST_HCCL_BASE_TYPE = 0x070000U;
    const uint32_t MSPROF_REPORT_ACL_DVPP_BASE_TYPE = 0x090000U;
    const uint32_t MSPROF_REPORT_ACL_GRAPH_BASE_TYPE = 0x0A0000U;

    /* Msprof report type of model(15000) level, offset: 0x000000 */
    const uint32_t MSPROF_REPORT_MODEL_GRAPH_ID_MAP_TYPE = 0;       /* type info: graph_id_map */
    const uint32_t MSPROF_REPORT_MODEL_EXECUTE_TYPE = 1;            /* type info: execute */
    const uint32_t MSPROF_REPORT_MODEL_LOAD_TYPE = 2;               /* type info: load */
    const uint32_t MSPROF_REPORT_MODEL_INPUT_COPY_TYPE = 3;         /* type info: IntputCopy */
    const uint32_t MSPROF_REPORT_MODEL_OUTPUT_COPY_TYPE = 4;        /* type info: OutputCopy */
    const uint32_t MSPROF_REPORT_MODEL_LOGIC_STREAM_TYPE = 7;       /* type info: logic_stream_info */
    const uint32_t MSPROF_REPORT_MODEL_EXEOM_TYPE = 8;              /* type info: exeom */
    const uint32_t MSPROF_REPORT_MODEL_UDF_BASE_TYPE = 0x010000U;   /* type info: udf_info */
    const uint32_t MSPROF_REPORT_MODEL_AICPU_BASE_TYPE = 0x020000U; /* type info: aicpu */

    /* Msprof report type of node(10000) level, offset: 0x000000 */
    const uint32_t MSPROF_REPORT_NODE_BASIC_INFO_TYPE = 0;      /* type info: node_basic_info */
    const uint32_t MSPROF_REPORT_NODE_TENSOR_INFO_TYPE = 1;     /* type info: tensor_info */
    const uint32_t MSPROF_REPORT_NODE_FUSION_OP_INFO_TYPE = 2;  /* type info: fusion_op_info */
    const uint32_t MSPROF_REPORT_NODE_CONTEXT_ID_INFO_TYPE = 4; /* type info: context_id_info */
    const uint32_t MSPROF_REPORT_NODE_LAUNCH_TYPE = 5;          /* type info: launch */
    const uint32_t MSPROF_REPORT_NODE_TASK_MEMORY_TYPE = 6;     /* type info: task_memory_info */
    const uint32_t MSPROF_REPORT_NODE_HOST_OP_EXEC_TYPE = 8;    /* type info: op exec */
    const uint32_t MSPROF_REPORT_NODE_ATTR_INFO_TYPE = 9;       /* type info: node_attr_info */
    const uint32_t MSPROF_REPORT_NODE_HCCL_OP_INFO_TYPE = 10;   /* type info: hccl op info */
    const uint32_t MSPROF_REPORT_NODE_STATIC_OP_MEM_TYPE = 11;  /* type info: static_op_mem */
    /* Msprof report type of node(10000) level(ge api), offset: 0x010000 */
    const uint32_t MSPROF_REPORT_NODE_GE_API_BASE_TYPE = 0x010000U;
    const uint32_t MSPROF_REPORT_NODE_HCCL_BASE_TYPE = 0x020000U;     /* type info: hccl api */
    const uint32_t MSPROF_REPORT_NODE_DVPP_API_BASE_TYPE = 0x030000U; /* type info: dvpp api */

    /* Msprof report type of hccl(5500) level(op api), offset: 0x010000 */
    const uint32_t MSPROF_REPORT_HCCL_NODE_BASE_TYPE = 0x010000U;
    const uint32_t MSPROF_REPORT_HCCL_MASTER_TYPE = 0x010001U;
    const uint32_t MSPROF_REPORT_HCCL_SLAVE_TYPE = 0x010002U;

    struct MsprofApi
    {  // for MsprofReportApi
        uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        uint16_t level;
        uint32_t type;
        uint32_t threadId;
        uint32_t reserve;
        uint64_t beginTime;
        uint64_t endTime;
        uint64_t itemId;
    };

    struct MsprofEvent
    {  // for MsprofReportEvent
        uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        uint16_t level;
        uint32_t type;
        uint32_t threadId;
        uint32_t requestId;  // 0xFFFF means single event
        uint64_t timeStamp;
        uint64_t reserve = MSPROF_EVENT_FLAG;
        uint64_t itemId;
    };

    struct MsporfKernelInfo
    {
        uint16_t numBlocks;
        uint16_t argsSize;
        uint8_t ratio : 3;
        uint8_t schedMode : 2;
        uint8_t rsv : 3;
        uint8_t reserved[11];
    };

    struct MsprofDim3
    {
        uint16_t x;
        uint16_t y;
        uint16_t z;
    };

    struct MsprofSimtKernelInfo
    {
        MsprofDim3 gridDim;
        MsprofDim3 blockDim;
        uint16_t argsSize;
        uint8_t schedMode : 2;
        uint8_t rsv : 6;
        uint8_t reserved;
    };

    struct MsprofRuntimeTrack
    {  // for MsprofReportCompactInfo buffer data
        uint16_t deviceId;
        uint16_t streamId;
        uint32_t taskId;
        uint64_t taskType;
        uint64_t kernelName;
        union
        {
            struct MsporfKernelInfo kernelInfo;
            struct MsprofSimtKernelInfo simtKernelInfo;
        } extInfo;
    };

    struct MsprofDpuTrack
    {                       // for MsprofReportCompactInfo buffer data
        uint16_t deviceId;  // high 4 bits, devType: dpu: 1, low 12 bits device id
        uint16_t streamId;
        uint32_t taskId;
        uint32_t taskType;  // task type enum
        uint32_t res;
        uint64_t startTime;  // start time
    };

    struct MsprofCaptureStreamInfo
    {                               // for MsprofReportCompactInfo buffer data
        uint16_t captureStatus;     // 标志是否销毁 0记为正常 1记为销毁
        uint16_t modelStreamId;     // capture stream id 销毁记录的stream id设置为 UINT16_MAX
        uint16_t originalStreamId;  // ori stream id 销毁记录的stream id设置为 UINT16_MAX
        uint16_t modelId;           // capture model id与GE无关
        uint16_t deviceId;
    };

    enum AlgType
    {
        HCCL_ALG_NONE = 0,
        HCCL_ALG_MESH,
        HCCL_ALG_RING,
        HCCL_ALG_NB,
        HCCL_ALG_HD,
        HCCL_ALG_NHR,
        HCCL_ALG_PIPELINE,
        HCCL_ALG_PAIRWISE,
        HCCL_ALG_STAR,
    };

#pragma pack(1)
    struct MsprofHcclOPInfo
    {  // for MsprofReportCompactInfo buffer data
        uint8_t relay : 1;
        uint8_t retry : 1;
        uint8_t dataType;
        uint64_t algType;  // 通信算子使用的算法,hash的key,其值是以"-"分隔的字符串
        uint64_t count;
        uint64_t groupName;
    };
#pragma pack()

    struct MsprofMemcpyInfo
    {                              // for MsprofReportCompactInfo buffer data
        uint64_t dataSize;         // 数据量大小， 字节
        uint64_t maxSize;          // 单个task拷贝的最大数据量
        uint16_t memcpyDirection;  // memcpy的方向
    };

    const uint16_t MSPROF_AICPU_DATA_RESERVE_BYTES = 9;
    struct MsprofAicpuNodeAdditionalData
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
        uint8_t reserve[MSPROF_AICPU_DATA_RESERVE_BYTES];
    };

    const uint16_t MSPROF_AICPU_MODEL_RESERVE_BYTES = 24;
    struct MsprofAicpuModelAdditionalData
    {
        uint64_t indexId;
        uint32_t modelId;
        uint16_t tagId;
        uint16_t rsv1;
        uint64_t eventId;
        uint8_t reserve[MSPROF_AICPU_MODEL_RESERVE_BYTES];
    };

    const uint16_t MSPROF_DP_DATA_RESERVE_BYTES = 16;
    const uint16_t MSPROF_DP_DATA_ACTION_LEN = 16;
    const uint16_t MSPROF_DP_DATA_SOURCE_LEN = 64;
    struct MsprofAicpuDpAdditionalData
    {
        char action[MSPROF_DP_DATA_ACTION_LEN];
        char source[MSPROF_DP_DATA_SOURCE_LEN];
        uint64_t index;
        uint64_t size;
        uint8_t reserve[MSPROF_DP_DATA_RESERVE_BYTES];
    };

    enum MsprofMindsporeNodeTag
    {
        GET_NEXT_DEQUEUE_WAIT = 1,
    };

    struct MsprofAicpuMiAdditionalData
    {
        uint32_t nodeTag;  // MsprofMindsporeNodeTag:1
        uint32_t reserve;
        uint64_t queueSize;
        uint64_t runStartTime;
        uint64_t runEndTime;
    };

    const uint16_t MSPROF_COMPACT_INFO_DATA_LENGTH = 40;
    struct MsprofCompactInfo
    {  // for MsprofReportCompactInfo buffer data
        uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        uint16_t level;
        uint32_t type;
        uint32_t threadId;
        uint32_t dataLen;
        uint64_t timeStamp;
        union
        {
            uint8_t info[MSPROF_COMPACT_INFO_DATA_LENGTH];
            MsprofRuntimeTrack runtimeTrack;
            MsprofDpuTrack dpuTrack;
            MsprofCaptureStreamInfo captureStreamInfo;
            MsprofNodeBasicInfo nodeBasicInfo;
            MsprofAttrInfo nodeAttrInfo;
            MsprofHcclOPInfo hcclopInfo;
            MsprofMemcpyInfo memcpyInfo;
        } data;
    };

    struct MsprofDpuHcclTrack
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
        uint32_t aicpu_task_id;
        uint16_t streamId;
        uint16_t planeID;
        uint16_t npuDevId;
        uint16_t dpuDevId;
        uint8_t opType;         // {0: sum, 1: mul, 2: max, 3: min}
        uint8_t dataType;       // data type {0: INT8, 1: INT16, 2: INT32, 3: FP16, 4:FP32, 5:INT64, 6:UINT64}
        uint8_t linkType;       // link type {0: 'OnChip', 1: 'HCCS', 2: 'PCIe', 3: 'RoCE'}
        uint8_t transportType;  // transport type {0: SDMA, 1: RDMA, 2:LOCAL}
        uint8_t rdmaType;       // RDMA type {0: RDMASendNotify, 1:RDMASendPayload}
        uint8_t role;           // role {0: dst, 1:src}
        uint8_t workFlowMode;
        uint8_t reserves[1];
    };

    const uint16_t MSPROF_ADDITIONAL_INFO_DATA_LENGTH = 232;
    struct MsprofAdditionalInfo
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
            MsprofAicpuNodeAdditionalData aicpuNode;
            MsprofAicpuModelAdditionalData aicpuModel;
            MsprofAicpuDpAdditionalData aicpuDp;
            MsprofAicpuMiAdditionalData aicpuMi;
            MsprofDpuHcclTrack dpuHcclTrack;
        };
    };

    struct MsprofVariableInfo
    {  // for MsprofVariableInfo buffer data
        uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        uint16_t level;
        uint32_t type;
        uint32_t threadId;
        uint32_t dataLen;
        uint64_t timeStamp;
        uint8_t data[0];
    };

    struct ConcatTensorInfo
    {
        uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        uint16_t level = 0;
        uint32_t type = 0;
        uint32_t threadId = 0;
        uint32_t dataLen = 0;
        uint64_t timeStamp = 0;
        uint64_t opName = 0;
        uint32_t tensorNum = 0;
        std::vector<MsrofTensorData> tensorData{MSPROF_GE_TENSOR_DATA_NUM};
    };

    // AICPU kfc算子执行时间
    struct AicpuKfcProfCommTurn
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

#ifdef __cplusplus
}
#endif

#endif  // MSPROFILER_PROF_COMMON_H_
