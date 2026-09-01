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
#ifndef MSPROFILER_PROF_STRUCT_H_
#define MSPROFILER_PROF_STRUCT_H_

#include <stdint.h>

#include <vector>

#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus

#define MSPROF_DATA_HEAD_MAGIC_NUM 0x5a5a
#define MSPROF_EVENT_FLAG 0xFFFFFFFFFFFFFFFFULL

#pragma pack(1)
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

    // =====================API_EVENT=====================
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
    // =====================API_EVENT=====================

    // =====================VARIABLE=====================
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
    // =====================VARIABLE=====================

    // =====================ADDITIONAL=====================
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

#pragma pack(1)
    struct MsprofAicpuHCCLOPInfo
    {
        uint8_t relay : 1;   // 借轨通信
        uint8_t retry : 1;   // 重传标识
        uint8_t dataType;    // 跟HcclDataType类型保存一致
        uint64_t algType;    // 通信算子使用的算法,hash的key,其值是以"-"分隔的字符串
        uint64_t count;      // 发送数据个数
        uint64_t groupName;  // group hash id
        uint32_t rankSize;
        uint16_t streamId;
        uint32_t taskId;
    };

    struct MsprofAicpuHcclTaskInfo
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
#pragma pack()
    const uint32_t KFC_INFOS_NUM = 2;
    struct MsprofKfcInfos
    {
        MsprofAicpuHcclTaskInfo infos[KFC_INFOS_NUM];
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

    // Aicore算子执行时间
    struct AicpuKfcProfComputeTurn
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

    // 翻转task的上报
    struct MsporfAicpuFlipTask
    {
        uint16_t streamId;
        uint16_t taskId;  // 值无特殊要求
        uint32_t flipNum;
        uint32_t reserve[2];
    };

    struct MsprofAicpuHcclMainStreamTask
    {
        uint16_t aicpuStreamId;
        uint16_t aicpuTaskId;
        uint16_t streamId;
        uint16_t taskId;
        uint16_t type;  // 0是头 1是尾
        uint16_t reserve[3];
    };

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

#define MSPROF_GE_TENSOR_DATA_SHAPE_LEN 8
    struct MsrofTensorData
    {
        uint32_t tensorType;
        uint32_t format;
        uint32_t dataType;
        uint32_t shape[MSPROF_GE_TENSOR_DATA_SHAPE_LEN];
    };

#define MSPROF_GE_TENSOR_DATA_NUM 5
    struct MsprofTensorInfo
    {
        uint64_t opName;
        uint32_t tensorNum;
        MsrofTensorData tensorData[MSPROF_GE_TENSOR_DATA_NUM];
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

#define MSPROF_GE_FUSION_OP_NUM 8
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

#define MSPROF_CTX_ID_MAX_NUM 55
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
#pragma pack()

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
            MsprofAicpuDpAdditionalData aicpuDp;
            MsprofAicpuModelAdditionalData aicpuModel;
            MsprofAicpuMiAdditionalData aicpuMi;
            AicpuKfcProfCommTurn commTurn;
            AicpuKfcProfComputeTurn computeTurn;
            MsprofAicpuHCCLOPInfo opInfo;
            MsporfAicpuFlipTask flipTask;
            MsprofAicpuHcclMainStreamTask mainStreamTask;
            MsprofKfcInfos kfcInfos;
            MsprofHcclInfo hcclInfo;
            MsprofMultiThread multiThread;
            MsprofNodeBasicInfo nodeBasicInfo;
            MsprofAttrInfo attrInfo;
            MsprofTensorInfo tensorInfo;
            ProfFusionOpInfo fusionOpInfo;
            MsprofContextIdInfo contextIdInfo;
            MsprofMemoryInfo memoryInfo;
        };
    };
    // =====================ADDITIONAL=====================

    // =====================COMPACT=====================
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

    struct MsprofModelInfo
    {
        uint32_t modelId;
        uint8_t rsv[12];
    };

    struct MsprofEventInfo
    {
        uint64_t key;
        uint32_t eventFlag;
        uint32_t flag;
    };

    struct MsprofNotifyInfo
    {
        uint64_t key;
        uint8_t rsv[8];
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
            uint8_t rsv[16];
            struct MsporfKernelInfo kernelInfo;
            struct MsprofSimtKernelInfo simtKernelInfo;
            struct MsprofModelInfo modelInfo;
            struct MsprofEventInfo eventInfo;
            struct MsprofNotifyInfo notifyInfo;
        } extInfo;
    };

    struct MsprofRuntimeTrackV2
    {  // for MsprofReportCompactInfo buffer data
        uint16_t deviceId;
        uint8_t rsv[2];
        uint32_t streamId;
        uint32_t taskId;
        uint32_t taskType;
        uint64_t kernelName;
        union
        {
            uint8_t rsv[16];
            struct MsporfKernelInfo kernelInfo;
            struct MsprofSimtKernelInfo simtKernelInfo;
            struct MsprofModelInfo modelInfo;
            struct MsprofEventInfo eventInfo;
            struct MsprofNotifyInfo notifyInfo;
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

    struct MsprofCaptureStreamInfoV2
    {
        uint16_t deviceId;
        uint8_t captureStatus;  //  0：创建 1：销毁
        uint8_t rsv;
        uint32_t streamId;
        uint32_t originalStreamId;
        uint32_t modelId;
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
            MsprofRuntimeTrackV2 runtimeTrackV2;
            MsprofDpuTrack dpuTrack;
            MsprofCaptureStreamInfo captureStreamInfo;
            MsprofCaptureStreamInfoV2 captureStreamInfoV2;
            MsprofNodeBasicInfo nodeBasicInfo;
            MsprofAttrInfo nodeAttrInfo;
            MsprofHcclOPInfo hcclopInfo;
            MsprofMemcpyInfo memcpyInfo;
        } data;
    };
    // =====================COMPACT=====================

#ifdef __cplusplus
}
#endif

#endif  // MSPROFILER_PROF_STRUCT_H_
