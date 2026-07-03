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

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/database/db_assembler.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/application/credential/id_pool.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/memcpy_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/mc2_comm_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ccu_mission_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/dpu_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/unified_pmu_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/acc_pmu_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/low_power_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ddr_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hbm_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hccs_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/host_usage_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/llc_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_op_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_module_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/pcie_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/soc_bandwidth_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/sys_io_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/netdev_stats_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/qos_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/sio_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ub_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/overlap_analysis_data.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain::Environment;
using IdPool = Analysis::Application::Credential::IdPool;
using namespace Analysis::Infra;

namespace {
const int DEPTH = 0;
const std::string DATA_DIR = "./db_assembler";
const std::string PROF = File::PathJoin({DATA_DIR, "PROF"});
const std::string OUTPUT_PATH = File::PathJoin({PROF, "mindstudio_profiler_output"});
}

class DBAssemblerUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(DATA_DIR));
        EXPECT_TRUE(File::CreateDir(PROF));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF, DEVICE_PREFIX + "0"})));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(DATA_DIR, 0));
    }

    virtual void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(OUTPUT_PATH));
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1715760307197379"},
            {"endCollectionTimeEnd",     "1715760313397397"},
            {"startClockMonotonicRaw",   "9691377159398230"},
            {"hostMonotonic",            "9691377161797070"},
            {"platform_version",         "5"},
            {"CPU",                      {{{"Frequency",      "100.000000"}}}},
            {"DeviceInfo",               {{{"hwts_frequency", "50"}, {"aic_frequency", "1650"}}}},
            {"devCntvct",                "484576969200418"},
            {"hostCntvctDiff",           "0"},
            {"hostname",                 "localhost"},
            {"hostCntvctDiff",           "0"},
            {"pid",                      "1"},
            {"llc_profiling",            "read"},
            {"ai_core_profiling_mode",   "task-based"},
            {"qosEvents",                "0:OTHERS,1:AIC_DAT_QOS"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
        IdPool::GetInstance().Clear();
    }

    virtual void TearDown()
    {
        IdPool::GetInstance().Clear();
        if (File::Exist(OUTPUT_PATH)) {
            EXPECT_TRUE(File::RemoveDir(OUTPUT_PATH, DEPTH));
        }
        GlobalMockObject::verify();
    }
};

static std::string GetMsprofDbPath()
{
    std::vector<std::string> files = File::GetOriginData(OUTPUT_PATH, {DB_NAME_MSPROF_DB}, {".json", ".csv"});
    EXPECT_EQ(files.size(), 1);
    return files.size() ? files[0] : "";
}

template<typename ElemT>
static void StubReserveFailureForElem()
{
    MOCKER_CPP(&Reserve<ElemT>).stubs().will(returnValue(false));
}

template<typename ElemT>
static void ResetReserveFailureForElem()
{
    MOCKER_CPP(&Reserve<ElemT>).reset();
}

template<typename VectorT>
static void StubReserveFailureForVector()
{
    StubReserveFailureForElem<typename VectorT::value_type>();
}

template<typename VectorT>
static void ResetReserveFailureForVector()
{
    ResetReserveFailureForElem<typename VectorT::value_type>();
}

static std::vector<ApiData> GenerateApiData()
{
    std::vector<ApiData> res;
    ApiData data;
    data.apiName = "launch";
    data.connectionId = 2762; // connectionId 2762
    data.id = "0";
    data.timestamp = 1717575960208020750; // start 1717575960208020750
    data.itemId = "hcom_broadcast_";
    data.level = MSPROF_REPORT_NODE_LEVEL;
    data.threadId = 87144; // threadId 87144
    data.end = 1717575960209010750; // end 1717575960209010750
    data.structType = "launch";
    res.push_back(data);
    return res;
}

static OverlapAnalysisData MakeOverlapAnalysisData(uint16_t deviceId, uint64_t timestamp, uint64_t duration,
                                                   OverlapAnalysisType type)
{
    OverlapAnalysisData data;
    data.deviceId = deviceId;
    data.timestamp = timestamp;
    data.duration = duration;
    data.type = type;
    return data;
}

static std::vector<OverlapAnalysisData> GenerateOverlapAnalysisData()
{
    std::vector<OverlapAnalysisData> res;
    res.push_back(MakeOverlapAnalysisData(0, 1000, 2000, OverlapAnalysisType::COMPUTE));
    res.push_back(MakeOverlapAnalysisData(0, 4000, 3000, OverlapAnalysisType::COMMUNICATION));
    res.push_back(MakeOverlapAnalysisData(1, 9000, 1000, OverlapAnalysisType::COMM_NOT_OVERLAP_COMP));
    res.push_back(MakeOverlapAnalysisData(1, 12000, 4000, OverlapAnalysisType::FREE));
    return res;
}

static std::vector<CommunicationOpData> GenerateOpData()
{
    std::vector<CommunicationOpData> res;
    CommunicationOpData data;
    data.opName = "hcom_broadcast__674_0_1";
    data.groupName = "16898834563344171674";
    data.connectionId = 2762; // connectionId 2762
    data.opKey = "16898834563344171674-1-1-1-1";
    data.timestamp = 1717575960213957957; // start 1717575960213957957
    data.end = 1717575960214957957; // end 1717575960214957957
    data.relay = 0; // relay 0
    data.retry = 0; // retry 0
    data.dataType = 1; // dataType 1
    data.algType = "MESH-RING";
    data.count = 5; // count 5
    data.opType = "hcom_broadcast_";
    data.modelId = 4294967295; // modelId 4294967295
    data.deviceId = 0; // device 0
    res.push_back(data);
    return res;
}

static std::vector<KfcOpData> GenerateKfcOpData()
{
    std::vector<KfcOpData> res;
    KfcOpData data;
    data.opName = "hcom_allReduce__654_0_1";
    data.groupName = "1334992688230612654";
    data.connectionId = 1234; // connectionId 1234
    data.opKey = "1334992688230612654-0-1-1-1";
    data.timestamp = 1717575960213957958; // start 1717575960213957958
    data.end = 1717575960214957958; // end 1717575960214957958
    data.relay = 0; // relay 0
    data.retry = 0; // retry 0
    data.dataType = 1; // dataType 1
    data.algType = "MESH-RING";
    data.count = 5; // count 5
    data.opType = "allreduceAicpuKernel";
    data.modelId = 4294967295; // modelId 4294967295
    data.deviceId = 0; // device 0
    res.push_back(data);
    return res;
}

static std::vector<CommunicationTaskData> GenerateTaskData()
{
    std::vector<CommunicationTaskData> res;
    CommunicationTaskData data;
    data.planeId = 0; // planeId 0
    data.modelId = 4294967295; // modelId 4294967295
    data.streamId = 1; // streamId 1
    data.taskId = 1; // taskId 1
    data.contextId = 1; // contextId 1
    data.batchId = 1; // batchId 1
    data.srcRank = 0; // src 0
    data.dstRank = 1; // dst 1
    data.opKey = "16898834563344171674-1-1-1-1";
    data.deviceId = 0; // device 0
    data.opName = "hcom_broadcast__674_0_1";
    data.taskType = "Notify_Wait";
    data.groupName = "16898834563344171674";
    data.transportType = 2; // transport 2
    data.size = 3200; // size 3200
    data.dataType = UINT16_MAX;
    data.linkType = UINT16_MAX;
    data.notifyId = 456; // notifyId 456
    data.rdmaType = UINT16_MAX;
    data.timestamp = 1717575960213957957; // start 1717575960213957957
    data.duration = 1000000.0; // dur 1000000.0
    data.durationEstimated = 20.0; // es_dur 20.0
    data.bandwidth = 1.0; // bw 1.0
    res.push_back(data);
    return res;
}

static std::vector<KfcTaskData> GenerateKfcTaskData()
{
    std::vector<KfcTaskData> res;
    KfcTaskData data;
    data.planeId = 1; // planeId 1
    data.modelId = 4294967295; // modelId 4294967295
    data.streamId = 69; // streamId 69
    data.taskId = 1; // taskId 1
    data.contextId = 4294967295; // contextId 4294967295
    data.batchId = 1; // batchId 1
    data.srcRank = 0; // src 0
    data.dstRank = 1; // dst 1
    data.opKey = "1334992688230612654-1-1-1-1";
    data.deviceId = 0; // device 0
    data.opName = "hcom_allReduce__654_0_1";
    data.taskType = "Memcpy";
    data.groupName = "1334992688230612654";
    data.transportType = 2; // transport 2
    data.size = 3200; // size 3200
    data.dataType = UINT16_MAX;
    data.linkType = UINT16_MAX;
    data.notifyId = 456; // notifyId 456
    data.rdmaType = UINT16_MAX;
    data.timestamp = 1717575960213957958; // start 1717575960213957958
    data.duration = 1000000.0; // dur 1000000.0
    data.durationEstimated = 20.0; // es_dur 20.0
    data.bandwidth = 1.0; // bw 1.0
    res.push_back(data);
    return res;
}

static std::vector<AccPmuData> GenerateAccPmuData()
{
    std::vector<AccPmuData> res;
    AccPmuData data;
    data.accId = 128; // accID 128
    data.timestamp = 236368325745670; // timestampNs 236368325745670
    res.push_back(data);
    return res;
}

static std::vector<LowPowerData> GenerateAicoreFreqData()
{
    std::vector<LowPowerData> res;
    LowPowerData data;
    data.deviceId = 1; // device_1
    data.timestamp = 236368325745670; // timestamp 236368325745670
    data.freq = 50.0; // freq 50.0
    data.dieId = 1; // dieId 1
    res.push_back(data);
    return res;
}

static std::vector<DDRData> GenerateDDRData()
{
    std::vector<DDRData> res;
    DDRData data;
    data.deviceId = 2; // device_2
    data.timestamp = 236368325745670; // timestamp 236368325745670
    data.fluxRead = 100.0; // read 100.0
    data.fluxWrite = 50.0; // write 50.0
    res.push_back(data);
    return res;
}

static std::vector<HbmData> GenerateHbmData()
{
    std::vector<HbmData> res;
    HbmData data;
    data.deviceId = 0; // deviceId 0
    data.timestamp = 236368325745670; // localTime 236368325745670
    data.bandWidth = 100.0; // bandWidth 100.0
    data.hbmId = 1; // hbmId 50.0
    res.push_back(data);
    return res;
}

static std::vector<HccsData> GenerateHccsData()
{
    std::vector<HccsData> res;
    HccsData data;
    data.deviceId = 2; // deviceId 2
    data.timestamp = 236368325745660; // localTime 236368325745660
    data.txThroughput = 100; // txThroughput 100.0
    data.rxThroughput = 200; // rxThroughput 50.0
    res.push_back(data);
    return res;
}

static std::vector<LLcData> GenerateLlcData()
{
    std::vector<LLcData> res;
    LLcData data;
    data.deviceId = 2; // deviceId 2
    data.timestamp = 236368325745555; // localTime 236368325745555
    data.llcID = 10; // llcID 10
    data.hitRate = 30.5; // hitRate 30.5
    data.throughput = 50.0; // throughput 50.0
    res.push_back(data);
    return res;
}

static std::vector<MsprofTxHostData> GenerateMsprofTxData()
{
    std::vector<MsprofTxHostData> res;
    MsprofTxHostData data;
    data.timestamp = 236368325741111; // start 236368325741111
    data.end = 236368325742222; // end 236368325742222
    data.payloadValue = 10; // payloadValue 10
    data.category = 1962761985; // category 1962761985
    data.connectionId = UINT32_MAX; // connectionId 4294967295
    res.push_back(data);
    return res;
}

static std::vector<NpuMemData> GenerateNpuMemData()
{
    std::vector<NpuMemData> res;
    NpuMemData data;
    data.deviceId = 8; // deviceId 8
    data.event = "event"; // event "event"
    data.ddr = 10000000000; // ddr 10000000000
    data.hbm = 20000000000; // hbm 20000000000
    data.memory = 30000000000; // memory 30000000000
    data.timestamp = 236368325745555; // localTime 236368325745555
    res.push_back(data);
    return res;
}

static std::vector<PCIeData> GeneratePCIeData()
{
    std::vector<PCIeData> res;
    PCIeData data;
    data.deviceId = 1; // deviceId 1
    data.timestamp = 236368325745555; // timestamp 236368325745555
    BandwidthData txPost;
    txPost.min = 0; // min 0
    txPost.avg = 67; // avg 67
    txPost.min = 104; // min 104
    data.txPost = txPost;
    res.push_back(data);
    return res;
}

static std::vector<SocBandwidthData> GenerateSocData()
{
    std::vector<SocBandwidthData> res;
    SocBandwidthData data;
    data.deviceId = 1; // deviceId 1
    data.timestamp = 236368325745555; // timestamp 236368325745555
    data.l2BufferBwLevel = 2; // l2_buffer_bw_level 2
    data.mataBwLevel = 3; // mata_bw_level 3
    res.push_back(data);
    return res;
}

static std::vector<NicOriginalData> GenerateNicData()
{
    std::vector<SysIOOriginalData> res;
    SysIOOriginalData data;
    data.deviceId = 1; // deviceId 1
    data.timestamp = 236368325745555; // timestamp 236368325745555
    data.bandwidth = 12345; // bandwidth 12345.0
    data.rxPacketRate = 1200.0; // rxPacketRate 1200.0
    data.rxByteRate = 1300.0; // rxByteRate 1300.0
    res.push_back(data);
    NicOriginalData nicOriginal;
    nicOriginal.sysIOOriginalData = res;
    std::vector<NicOriginalData> nicOriginalData;
    nicOriginalData.push_back(nicOriginal);
    return nicOriginalData;
}

static std::vector<RoceOriginalData> GenerateRoceData()
{
    std::vector<SysIOOriginalData> res;
    SysIOOriginalData data;
    data.deviceId = 2; // deviceId 2
    data.timestamp = 236368325746666; // timestamp 236368325746666
    data.bandwidth = 12345; // bandwidth 12345.0
    data.rxPacketRate = 1200.0; // rxPacketRate 1200.0
    data.rxByteRate = 1300.0; // rxByteRate 1300.0
    res.push_back(data);
    RoceOriginalData roceOriginal;
    roceOriginal.sysIOOriginalData = res;
    std::vector<RoceOriginalData> roceOriginalData;
    roceOriginalData.push_back(roceOriginal);
    return roceOriginalData;
}

static std::vector<RoceOriginalData> GenerateEmptyRoceData()
{
    std::vector<SysIOOriginalData> res = {};
    RoceOriginalData roceOriginal;
    roceOriginal.sysIOOriginalData = res;
    std::vector<RoceOriginalData> roceOriginalData;
    roceOriginalData.push_back(roceOriginal);
    return roceOriginalData;
}

static std::vector<NpuOpMemData> GenerateNpuOpMemData()
{
    std::vector<NpuOpMemData> res;
    NpuOpMemData data;
    data.deviceId = 2; // deviceId 2
    data.operatorName = "NonZero57";
    data.addr = 20618011344896; // addr 20618011344896
    data.size = 1638400; // size 1638400
    data.timestamp = 1717575960208020750; // localTime 1717575960208020750
    data.threadId = 87144; // threadId 87144
    data.totalAllocateMemory = 1703936; // totalAllocateMemory 1703936
    data.totalReserveMemory = 625213440; // totalReserveMemory 625213440
    data.type = 100; // type 100
    res.push_back(data);
    return res;
}

static std::vector<NpuModuleMemData> GenerateNpuModuleMemData()
{
    std::vector<NpuModuleMemData> res;
    NpuModuleMemData data;
    data.deviceId = 2; // deviceId 2
    data.moduleId = 5; // moduleId 5
    data.timestamp = 236368325745555; // timestamp 236368325745555
    data.totalReserved = 1638400; // size 1638400
    res.push_back(data);
    return res;
}

static std::vector<AscendTaskData> GenerateAscendTaskData()
{
    std::vector<AscendTaskData> res;
    AscendTaskData data;
    data.deviceId = 0;  // device id 0
    data.indexId = -1; // index_id -1
    data.streamId = 1; // streamId 1
    data.taskId = 10; // taskId 10
    data.contextId = 1; // contextId 1
    data.batchId = 1; // batchId 1
    data.connectionId = 2345; // connectionId 2345
    data.timestamp = 1717575960208020758; // start 1717575960208020758
    data.duration = 450.78; // dur 450.78
    data.end = 1717575960208020759; // end 1717575960208020759
    data.hostType = "KERNEL_AICORE";
    data.deviceType = "AI_CORE";
    data.taskType = "KERNEL_AICORE";
    res.push_back(data);
    data.contextId = UINT32_MAX;
    res.push_back(data);
    return res;
}

static std::vector<MsprofTxDeviceData> GenerateDeviceTxData()
{
    std::vector<MsprofTxDeviceData> res;
    MsprofTxDeviceData data;
    data.deviceId = 0; // deviceId 0
    data.streamId = 2; // streamId 2
    data.taskId = 10; // taskId 10
    data.connectionId = 2346; // connectionId 2346
    data.timestamp = 1717575960208020759; // start 1717575960208020759
    data.duration = 450.78; // dur 450.78
    data.modelId = 1; // modelId 1
    res.push_back(data);
    return res;
}

static std::vector<MemcpyInfoData> GenerateMemcpyInfoData()
{
    std::vector<MemcpyInfoData> res;
    MemcpyInfoData data;
    res.push_back(data);
    return res;
}

static std::vector<TaskInfoData> GenerateComputeTaskInfo()
{
    std::vector<TaskInfoData> res;
    TaskInfoData data;
    data.deviceId = 0; // deviceId 0
    data.streamId = 1; // streamId 1
    data.taskId = 10; // taskId 10
    data.contextId = 1; // contextId 1
    data.batchId = 1; // batchId 1
    data.opName = "MatMulV3";
    data.inputFormats = "NC\"\"\"HW";
    res.push_back(data);
    data.contextId = UINT32_MAX;
    res.push_back(data);
    data.streamId = 2; // streamId 2
    res.push_back(data);
    return res;
}

static std::vector<MC2CommInfoData> GenerateKfcStreamData()
{
    std::vector<MC2CommInfoData> res;
    MC2CommInfoData data;
    data.aiCpuKfcStreamId = 2; // kfcStreamId 2
    data.commStreamIds = "3, 4, 5";
    res.push_back(data);
    return res;
}

static std::vector<UnifiedTaskPmu> GenerateUnifiedTaskPmuData()
{
    std::vector<UnifiedTaskPmu> res;
    UnifiedTaskPmu data;
    data.deviceId = 0; // deviceId 0
    data.streamId = 68; // streamId 68
    data.taskId = 22; // taskId 22
    data.subtaskId = 2344; // subtaskID 2344
    data.batchId = 4294967295; // batchId 4294967295
    data.header = "aic_total_time";
    data.value = 318360.0; // value 318360.0
    res.push_back(data);
    return res;
}

static std::vector<UnifiedSampleTimelinePmu> GenerateUnifiedSampleTimelinePmuData()
{
    std::vector<UnifiedSampleTimelinePmu> res;
    UnifiedSampleTimelinePmu data;
    data.deviceId = 0; // deviceId 0
    data.timestamp = 1701121739053206820; // timestamp 1701121739053206820
    data.totalCycle = 0; // totalCycle 0
    data.usage = 0.0; // usage 0.0
    data.freq = 1000.0; // freq 1000.0
    data.coreId = 0; // coreId 0
    data.coreType = 2;  // coreType 2
    res.push_back(data);
    return res;
}

static std::vector<UnifiedSampleSummaryPmu> GenerateUnifiedSampleSummaryPmuData()
{
    std::vector<UnifiedSampleSummaryPmu> res;
    UnifiedSampleSummaryPmu data;
    data.deviceId = 0; // deviceId 0
    data.metric = "total_time";
    data.value = 734951000.0; // value 734951000.0
    data.coreId = 0; // coreId 0
    data.coreType = 2; // coreType 2
    res.push_back(data);
    return res;
}

static std::vector<CpuUsageData> GenerateCpuUsageData()
{
    std::vector<CpuUsageData> res;
    CpuUsageData data;
    data.timestamp = 1719621074669030430; // start 1719621074669030430
    data.usage = 30; // usage 30
    data.cpuNo = "0"; // cpuNo 0
    res.push_back(data);

    data.timestamp = 1719621074669031430; // start 1719621074669031430
    data.usage = 18; // usage 18
    data.cpuNo = "Avg"; // cpuNo Avg
    res.push_back(data);

    data.timestamp = 1719621074669033430; // start 1719621074669033430
    data.usage = 32; // usage 32
    data.cpuNo = "Avg"; // cpuNo Avg
    res.push_back(data);

    data.timestamp = 1719621074669033430; // start 1719621074669033430
    data.usage = 31; // usage 31
    data.cpuNo = "sfjie"; // cpuNo sfjie
    res.push_back(data);
    return res;
}

static std::vector<MemUsageData> GenerateHostMemUsageData()
{
    std::vector<MemUsageData> res;
    MemUsageData data;
    data.timestamp = 1719621074669030430; // start 1719621074669030430
    data.usage = 30; // usage 30
    res.push_back(data);
    return res;
}

static std::vector<DiskUsageData> GenerateHostDiskUsageData()
{
    std::vector<DiskUsageData> res;
    DiskUsageData data;
    data.timestamp = 1719621074669030430; // start 1719621074669030430
    data.usage = 30; // usage 30
    data.readRate = 10; // readRate 10
    data.writeRate = 13; // writeRate 13
    res.push_back(data);
    return res;
}

static std::vector<NetWorkUsageData> GenerateHostNetWorkUsageData()
{
    std::vector<NetWorkUsageData> res;
    NetWorkUsageData data;
    data.timestamp = 1719621074669030430; // start 1719621074669030430
    data.usage = 30; // usage 30
    data.speed = 10; // speed 10
    res.push_back(data);
    return res;
}

static std::vector<OSRuntimeApiData> GenerateOSRuntimeApiData()
{
    std::vector<OSRuntimeApiData> res;
    OSRuntimeApiData data;
    data.timestamp = 1719621074669030430; // start 1719621074669030430
    data.endTime = 1719621074669040430; // end 1719621074669040430
    data.pid = 20; // pid 20
    data.tid = 123456; // tid 123456
    data.name = "clock_nanosleep"; // clock_nanosleep
    res.push_back(data);
    return res;
}

static std::vector<NetDevStatsEventData> GenerateNetDevStatsData()
{
    NetDevStatsEventData data;
    data.deviceId = 2; // deviceId 2
    data.timestamp = 1746706291651036160; // timestamp 1746706291651036160
    data.macTxPfcPkt = 5; // macTxPfcPkt 5
    data.macRxPfcPkt = 4; // macRxPfcPkt 4
    data.macTxByte = 128; // macTxByte 128
    data.macTxBandwidth = 6180.941; // macTxBandwidth 6180.941
    data.macRxByte = 64; // macRxByte 64
    data.macRxBandwidth = 3095.575; // macRxBandwidth 3095.575
    data.macTxBadByte = 16; // macTxBadByte 16
    data.macRxBadByte = 32; // macRxBadByte 32
    data.roceTxPkt = 35790; // roceTxPkt 35790
    data.roceRxPkt = 35633; // roceRxPkt 35633
    data.roceTxErrPkt = 10; // roceTxErrPkt 10
    data.roceRxErrPkt = 20; // roceRxErrPkt 20
    data.roceTxCnpPkt = 5; // roceTxCnpPkt 5
    data.roceRxCnpPkt = 5; // roceRxCnpPkt 5
    data.roceNewPktRty = 3; // roceNewPktRty 3
    data.nicTxByte = 2180; // nicTxByte 2180
    data.nicTxBandwidth = 105551.321; // nicTxBandwidth 105551.321
    data.nicRxByte = 2174; // nicRxByte 2174
    data.nicRxBandwidth = 105260.813; // nicRxBandwidth 105260.813
    return {data};
}

static std::vector<QosData> GenerateQosData()
{
    QosData data;
    std::vector<QosData> res;
    data.deviceId = 0; // deviceId 0
    data.timestamp = 1746706291651036160; // timestamp 1746706291651036160
    data.bw1 = 1; // bw1 1
    data.bw2 = 2; // bw2 2
    data.bw3 = 3; // bw3 3
    data.bw4 = 4; // bw4 4
    data.bw5 = 5; // bw5 5
    data.bw6 = 6; // bw6 6
    data.bw7 = 7; // bw7 7
    data.bw8 = 8; // bw8 8
    data.bw9 = 9; // bw9 9
    data.bw10 = 10; // bw10 10
    res.push_back(data);

    data.timestamp = 1746706291651050160; // timestamp 1746706291651050160
    res.push_back(data);

    data.deviceId = 1; // deviceId 1 data的deviceId和路径不匹配
    res.push_back(data);
    return res;
}

static std::vector<CCUMissionTimelineData> GenerateCCUData()
{
    std::vector<CCUMissionTimelineData> res;
    CCUMissionTimelineData loopData;
    loopData.deviceId = 0; // deviceId 0
    loopData.streamId = 7; // streamId 7
    loopData.taskId = 23; // taskId 23
    loopData.instructionId = 11; // instructionId 11
    loopData.timestamp = 1717575960208020750; // start 1717575960208020750
    loopData.duration = 1000.0; // duration 1000 ns
    loopData.timeType = CCU_TIME_TYPE_LOOP_GROUP;
    loopData.hasDieId = true;
    loopData.dieId = 1; // dieId 1
    loopData.hasDataSize = true;
    loopData.dataSize = 4096; // dataSize 4096
    loopData.hasBandwidth = true;
    loopData.bandwidth = 2.5; // bandwidth 2.5
    loopData.hasReduceInfo = true;
    loopData.reduceOpType = "sum";
    loopData.inputDataType = "fp16";
    loopData.outputDataType = "fp32";
    loopData.hasMask = true;
    loopData.mask = 3; // mask 3
    loopData.hasDelayChannel = true;
    loopData.maxDelayChannel = 4; // maxDelayChannel 4
    loopData.maxChannelDelay = 500; // maxChannelDelay 500
    res.push_back(loopData);

    CCUMissionTimelineData waitData;
    waitData.deviceId = 1; // deviceId 1
    waitData.streamId = 8; // streamId 8
    waitData.taskId = 24; // taskId 24
    waitData.instructionId = 12; // notify instructionId 12
    waitData.notifyRankId = 2; // notifyRankId 2
    waitData.timestamp = 1717575960209020750; // start 1717575960209020750
    waitData.duration = 2000.0; // duration 2000 ns
    waitData.timeType = CCU_TIME_TYPE_WAIT;
    res.push_back(waitData);
    return res;
}

static std::vector<DPUData> GenerateDPUData()
{
    std::vector<DPUData> res;
    DPUData taskData;
    taskData.dpuDeviceId = 2; // dpuDeviceId 2
    taskData.threadId = 123; // threadId 123
    taskData.timestamp = 1717575960208020750; // start 1717575960208020750
    taskData.endTime = 1717575960208021750; // end 1717575960208021750
    taskData.taskType = "DPU_TASK";
    taskData.streamId = 7; // streamId 7
    taskData.taskId = 23; // taskId 23
    taskData.opName = "dpu_kernel";
    taskData.isHccl = false;
    res.push_back(taskData);

    DPUData hcclData;
    hcclData.isHccl = true;
    hcclData.npuDeviceId = 0; // npuDeviceId 0
    hcclData.dpuDeviceId = 3; // dpuDeviceId 3
    hcclData.threadId = 124; // threadId 124
    hcclData.timestamp = 1717575960209020750; // start 1717575960209020750
    hcclData.endTime = 1717575960210020750; // end 1717575960210020750
    hcclData.opName = "dpu_hccl";
    hcclData.groupName = "group";
    hcclData.groupNameId = "100";
    hcclData.localRank = 1; // localRank 1
    hcclData.remoteRank = 2; // remoteRank 2
    hcclData.rankSize = 8; // rankSize 8
    hcclData.durationEstimated = 10.5; // durationEstimated 10.5
    hcclData.dataSize = 1024; // dataSize 1024
    hcclData.streamId = 8; // streamId 8
    hcclData.taskId = 24; // taskId 24
    hcclData.aicpuTaskId = 25; // aicpuTaskId 25
    hcclData.planeId = 1; // planeId 1
    hcclData.opType = "all_reduce";
    hcclData.dataType = "fp16";
    hcclData.linkType = "HCCS";
    hcclData.transportType = "RDMA";
    hcclData.rdmaType = "rdma";
    hcclData.notifyId = "notify";
    res.push_back(hcclData);
    return res;
}

static std::vector<SioData> GenerateSioData()
{
    std::vector<SioData> res;
    SioData data;
    data.deviceId = 0; // deviceId 0
    data.timestamp = 1746706291651036160; // timestamp 1746706291651036160
    data.name = "die 0";
    data.reqRxBandwidth = 1.0; // reqRxBandwidth 1 MB/s
    data.rspRxBandwidth = 2.0; // rspRxBandwidth 2 MB/s
    data.snpRxBandwidth = 3.0; // snpRxBandwidth 3 MB/s
    data.datRxBandwidth = 4.0; // datRxBandwidth 4 MB/s
    data.reqTxBandwidth = 5.0; // reqTxBandwidth 5 MB/s
    data.rspTxBandwidth = 6.0; // rspTxBandwidth 6 MB/s
    data.snpTxBandwidth = 7.0; // snpTxBandwidth 7 MB/s
    data.datTxBandwidth = 8.0; // datTxBandwidth 8 MB/s
    res.push_back(data);
    return res;
}

static std::vector<UbData> GenerateUbData()
{
    std::vector<UbData> res;
    UbData data;
    data.deviceId = 0; // deviceId 0
    data.portId = 3; // portId 3
    data.timestamp = 1746706291651036160; // timestamp 1746706291651036160
    data.udmaRxBind = 100; // udmaRxBind 100
    data.udmaTxBind = 200; // udmaTxBind 200
    res.push_back(data);
    return res;
}

static void InjectHcclData(DataInventory& dataInventory)
{
    std::shared_ptr<std::vector<CommunicationTaskData>> dataTaskS;
    std::shared_ptr<std::vector<CommunicationOpData>> dataOpS;
    std::shared_ptr<std::vector<KfcTaskData>> kfcTaskS;
    std::shared_ptr<std::vector<KfcOpData>> kfcOpS;
    auto dataOp = GenerateOpData();
    auto dataTask = GenerateTaskData();
    auto kfcOp = GenerateKfcOpData();
    auto kfcTask = GenerateKfcTaskData();
    MAKE_SHARED_NO_OPERATION(dataOpS, std::vector<CommunicationOpData>, dataOp);
    MAKE_SHARED_NO_OPERATION(dataTaskS, std::vector<CommunicationTaskData>, dataTask);
    MAKE_SHARED_NO_OPERATION(kfcTaskS, std::vector<KfcTaskData>, kfcTask);
    MAKE_SHARED_NO_OPERATION(kfcOpS, std::vector<KfcOpData>, kfcOp);
    dataInventory.Inject(dataOpS);
    dataInventory.Inject(dataTaskS);
    dataInventory.Inject(kfcOpS);
    dataInventory.Inject(kfcTaskS);
}

static void CheckEnumValueByTableName(const std::shared_ptr<DBRunner>& dbRunner, const std::string& tableName,
                                      const std::unordered_map<std::string, uint16_t>& enumTable)
{
    using EnumDataFormat = std::vector<std::tuple<uint32_t, std::string>>;
    EnumDataFormat checkData;
    std::string sqlStr = "SELECT id, name FROM " + tableName;
    const uint32_t ID_INDEX = 0;
    const uint32_t NAME_INDEX = 1;
    const uint16_t expectNum = enumTable.size();
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
    for (auto record : checkData) {
        EXPECT_EQ(std::get<ID_INDEX>(record), enumTable.find(std::get<NAME_INDEX>(record))->second);
    }
}

static void CheckStringId(std::vector<std::tuple<uint64_t, std::string>> data)
{
    const uint16_t stringIdIndex = 0;
    std::vector<uint64_t> stringIds;
    for (auto item : data) {
        stringIds.emplace_back(std::get<stringIdIndex>(item));
    }
    std::sort(stringIds.begin(), stringIds.end());
    for (int i = 0; i < 2; ++i) {
        EXPECT_EQ(stringIds[i], i);
    }
}

TEST_F(DBAssemblerUTest, TestRunApiDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    std::shared_ptr<std::vector<ApiData>> dataS;
    auto data = GenerateApiData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<ApiData>, data);
    auto dataInventory = DataInventory();
    dataInventory.Inject(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));

    // start, end, type, globalTid, connectionId, name
    using QueryDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t,
        uint64_t, uint64_t, uint64_t>>;
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, GetMsprofDbPath());
    ASSERT_NE(dbRunner, nullptr);
    std::string sqlStr = "SELECT startNs, endNs, type, globalTid, connectionId, name FROM " + TABLE_NAME_CANN_API;
    QueryDataFormat checkData;
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(1, checkData.size());
    auto start = std::get<0>(checkData[0]);
    uint64_t expectStart = 1717575960208020750;
    EXPECT_EQ(expectStart, start);
}

TEST_F(DBAssemblerUTest, TestRunApiDataShouldReturnFalseWhenReserveFailed)
{
    // start, end, type, globalTid, connectionId, name
    using SaveDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t,
        uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    std::shared_ptr<std::vector<ApiData>> dataS;
    auto data = GenerateApiData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<ApiData>, data);
    auto dataInventory = DataInventory();
    dataInventory.Inject(dataS);

    // Reserve failed
    StubReserveFailureForVector<SaveDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<SaveDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunHcclDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    InjectHcclData(dataInventory);
    EXPECT_TRUE(assembler.Run(dataInventory));

    uint64_t expectName = IdPool::GetInstance().GetUint64Id("hcom_broadcast__674_0_1");
    // 小算子数据
    // name, globalTaskId, taskType, planeId, groupName, notifyId, rdmaType, srcRank, dstRank, transportType,
    // size, dataType, linkType, opId, isMaster, bandwidth
    using CommunicationTaskDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t,
            uint64_t, uint64_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint32_t, uint16_t,
            double>>;
    CommunicationTaskDataFormat taskResult;
    std::string sql{"SELECT * FROM " + TABLE_NAME_COMMUNICATION_TASK_INFO};
    std::shared_ptr<DBRunner> msprofDBRunner;
    MAKE_SHARED0_NO_OPERATION(msprofDBRunner, DBRunner, GetMsprofDbPath());
    msprofDBRunner->QueryData(sql, taskResult);
    uint64_t opName = std::get<0>(taskResult[0]);
    EXPECT_EQ(expectName, opName);

    // 大算子数据
    // opName, start, end, connectionId, group_name, opId, relay, retry, data_type, alg_type, count, op_type, deviceId, rank_size
    using CommunicationOpDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        int32_t, int32_t, int32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint16_t, uint32_t>>;
    CommunicationOpDataFormat opResult;
    sql = "SELECT * FROM " + TABLE_NAME_COMMUNICATION_OP;
    msprofDBRunner->QueryData(sql, opResult);
    opName = std::get<0>(opResult[0]);
    EXPECT_EQ(expectName, opName);
}

TEST_F(DBAssemblerUTest, TestRunHcclDataShouldReturnFalseWhenReserveFailed)
{
    // 小算子数据
    // name, globalTaskId, taskType, planeId, groupName, notifyId, rdmaType, srcRank, dstRank, transportType,
    // size, dataType, linkType, opId, isMaster, bandwidth
    using CommunicationTaskDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t,
            uint64_t, uint64_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint32_t, uint16_t,
            double>>;
    // 大算子数据
    // opName, start, end, connectionId, group_name, opId, relay, retry, data_type, alg_type, count, op_type
    using CommunicationOpDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        uint32_t, int32_t, int32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    InjectHcclData(dataInventory);
    // Reserve CommunicationTaskDataFormat failed
    StubReserveFailureForVector<CommunicationTaskDataFormat>();
    // Reserve CommunicationOpDataFormat failed
    StubReserveFailureForVector<CommunicationOpDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<CommunicationTaskDataFormat>();
    ResetReserveFailureForVector<CommunicationOpDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunAccPmuDataShouldReturnTrueWhenRunSuccess)
{
    // accId, readBwLevel, writeBwLevel, readOstLevel, writeOstLevel, timestampNs, deviceId
    using AccPmuDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint32_t,
        uint32_t, uint32_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateAccPmuData();
    std::shared_ptr<std::vector<AccPmuData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<AccPmuData>, data);
    dataInventory.Inject<std::vector<AccPmuData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunAccPmuDataShouldReturnFalseWhenReserveFailed)
{
    // accId, readBwLevel, writeBwLevel, readOstLevel, writeOstLevel, timestampNs, deviceId
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint32_t,
        uint32_t, uint32_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateAccPmuData();
    std::shared_ptr<std::vector<AccPmuData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<AccPmuData>, data);
    dataInventory.Inject<std::vector<AccPmuData>>(dataS);
    // Reserve failed
    StubReserveFailureForVector<SaveDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<SaveDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunAicoreFreqDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateAicoreFreqData();
    std::shared_ptr<std::vector<LowPowerData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<LowPowerData>, data);
    dataInventory.Inject<std::vector<LowPowerData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));

    // deviceId, timestampNs, freq, dieId
    using AicoreFreqDataFormat = std::vector<std::tuple<uint16_t, uint64_t, double, int32_t>>;
    AicoreFreqDataFormat checkData;
    std::shared_ptr<DBRunner> msprofDBRunner;
    MAKE_SHARED0_NO_OPERATION(msprofDBRunner, DBRunner, GetMsprofDbPath());
    ASSERT_NE(msprofDBRunner, nullptr);
    std::string sql = "SELECT deviceId, timestampNs, freq, dieId FROM " + TABLE_NAME_AICORE_FREQ;
    EXPECT_TRUE(msprofDBRunner->QueryData(sql, checkData));
    ASSERT_EQ(1, checkData.size());
    EXPECT_EQ(1, std::get<0>(checkData[0]));
    EXPECT_EQ(236368325745670, std::get<1>(checkData[0]));
    EXPECT_DOUBLE_EQ(50.0, std::get<2>(checkData[0]));
    EXPECT_EQ(1, std::get<3>(checkData[0]));
}

TEST_F(DBAssemblerUTest, TestRunAicoreFreqDataShouldReturnFalseWhenReserveFailed)
{
    // deviceId, timestampNs, freq, dieId
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint64_t, double, int32_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateAicoreFreqData();
    std::shared_ptr<std::vector<LowPowerData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<LowPowerData>, data);
    dataInventory.Inject<std::vector<LowPowerData>>(dataS);
    // Reserve failed
    StubReserveFailureForVector<SaveDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<SaveDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunDDRDataShouldReturnTrueWhenRunSuccess)
{
    // device_id, timestamp, flux_read, flux_write
    using DDRDataFormat = std::vector<std::tuple<uint32_t, double, double, double>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateDDRData();
    std::shared_ptr<std::vector<DDRData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<DDRData>, data);
    dataInventory.Inject<std::vector<DDRData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunDDRDataShouldReturnFalseWhenReserveFailed)
{
    // device_id, timestamp, flux_read, flux_write
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateDDRData();
    std::shared_ptr<std::vector<DDRData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<DDRData>, data);
    dataInventory.Inject<std::vector<DDRData>>(dataS);
    // Reserve failed
    StubReserveFailureForVector<SaveDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<SaveDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunEnumDataShouldReturnTrueWhenProcessorRunSuccess)
{
    // 执行enum processor
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));

    // 校验processor生成的若干表及内容
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, GetMsprofDbPath());
    ASSERT_NE(dbRunner, nullptr);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_ENUM_API_TYPE, API_LEVEL_TABLE);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_ENUM_MODULE, MODULE_NAME_TABLE);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_ENUM_HCCL_DATA_TYPE, HCCL_DATA_TYPE_TABLE);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_ENUM_HCCL_LINK_TYPE, HCCL_LINK_TYPE_TABLE);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_ENUM_HCCL_TRANSPORT_TYPE, HCCL_TRANSPORT_TYPE_TABLE);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_ENUM_HCCL_RDMA_TYPE, HCCL_RDMA_TYPE_TABLE);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_MSTX_EVENT_TYPE, MSTX_EVENT_TYPE_TABLE);
}

TEST_F(DBAssemblerUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using SaveDataFormat = std::tuple<uint16_t, std::string>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    StubReserveFailureForElem<SaveDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForElem<SaveDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunHbmDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateHbmData();
    std::shared_ptr<std::vector<HbmData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<HbmData>, data);
    dataInventory.Inject<std::vector<HbmData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunHbmDataShouldReturnFalseWhenReserveFailed)
{
    // device_id, timestamp, flux_read, flux_write
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint8_t, uint64_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateHbmData();
    std::shared_ptr<std::vector<HbmData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<HbmData>, data);
    dataInventory.Inject<std::vector<HbmData>>(dataS);
    // Reserve failed
    StubReserveFailureForVector<SaveDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<SaveDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunHostInfoShouldReturnTrueWhenProcessorRunSuccess)
{
    using HostInfoDataFormat = std::vector<std::tuple<std::string, std::string>>;
    std::vector<std::string> hostDirs = {"host"};
    std::string hostUid = "123456789";
    std::string hostName = "localhost";
    MOCKER_CPP(&File::GetFilesWithPrefix).stubs().will(returnValue(hostDirs));
    MOCKER_CPP(&Context::GetHostUid).stubs().will(returnValue(hostUid));
    MOCKER_CPP(&Context::GetHostName).stubs().will(returnValue(hostName));

    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));
    MOCKER_CPP(&File::GetFilesWithPrefix).reset();
    MOCKER_CPP(&Context::GetHostUid).reset();
    MOCKER_CPP(&Context::GetHostName).reset();
    std::shared_ptr<DBRunner> dbRunner;

    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, GetMsprofDbPath());
    HostInfoDataFormat checkData;
    HostInfoDataFormat expectData = {
        {hostUid, hostName}
    };
    std::string sqlStr = "SELECT hostUid, hostName FROM " + TABLE_NAME_HOST_INFO;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectData, checkData);
}

TEST_F(DBAssemblerUTest, TestRunHostInfoShouldReturnTrueWhenNoHost)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));
}


TEST_F(DBAssemblerUTest, TestRunHccsDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateHccsData();
    std::shared_ptr<std::vector<HccsData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<HccsData>, data);
    dataInventory.Inject<std::vector<HccsData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunHccsDataShouldReturnFalseWhenReserveFailed)
{
    // deviceId, timestampNs, txThroughput, rxThroughput
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateHccsData();
    std::shared_ptr<std::vector<HccsData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<HccsData>, data);
    dataInventory.Inject<std::vector<HccsData>>(dataS);
    // Reserve failed
    StubReserveFailureForVector<SaveDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<SaveDataFormat>();
}


TEST_F(DBAssemblerUTest, TestRunLLcDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateLlcData();
    std::shared_ptr<std::vector<LLcData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<LLcData>, data);
    dataInventory.Inject<std::vector<LLcData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunLLcDataShouldReturnFalseWhenReserveFailed)
{
    // deviceId, llcID, timestamp, hitRate, throughput, mode
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint64_t, double, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateLlcData();
    std::shared_ptr<std::vector<LLcData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<LLcData>, data);
    dataInventory.Inject<std::vector<LLcData>>(dataS);
    // Reserve failed
    StubReserveFailureForVector<SaveDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<SaveDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunMsprofTxDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateMsprofTxData();
    std::shared_ptr<std::vector<MsprofTxHostData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<MsprofTxHostData>, data);
    dataInventory.Inject<std::vector<MsprofTxHostData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunMsprofTxDataShouldReturnFalseWhenReserveFailed)
{
    // startNs, endNs, eventType, rangeId, category, message, globalTid, endGlobalTid, domainId, connectionId
    using SaveDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t,
        uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateMsprofTxData();
    std::shared_ptr<std::vector<MsprofTxHostData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<MsprofTxHostData>, data);
    dataInventory.Inject<std::vector<MsprofTxHostData>>(dataS);
    // Reserve failed
    StubReserveFailureForVector<SaveDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<SaveDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunNpuDataShouldReturnTrueWhenProcessorRunSuccess)
{
    using NpuInfoDataFormat = std::vector<std::tuple<uint32_t, std::string>>;
    using RankDeviceMapFormat = std::vector<std::tuple<int32_t, uint32_t>>;
    std::vector<std::string> deviceDirs = {"device_0", "device_1", "device_2", "device_3", "device_4", "device_5"};
    uint16_t chip0 = 0;
    uint16_t chip1 = 1;
    uint16_t chip4 = 4;
    uint16_t chip5 = 5;
    uint16_t chip7 = 7;
    uint16_t chipX = 20;
    MOCKER_CPP(&File::GetFilesWithPrefix).stubs().will(returnValue(deviceDirs));
    MOCKER_CPP(&Context::GetPlatformVersion)
        .stubs()
        .will(returnValue(chip0))
        .then(returnValue(chip1))
        .then(returnValue(chip4))
        .then(returnValue(chip5))
        .then(returnValue(chip7))
        .then(returnValue(chipX));
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));
    MOCKER_CPP(&File::GetFilesWithPrefix).reset();
    MOCKER_CPP(&Context::GetPlatformVersion).reset();
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, GetMsprofDbPath());
    NpuInfoDataFormat checkNpuInfoData;
    NpuInfoDataFormat expectNpuInfoData = {
        {0, "Ascend310"},
        {1, "Ascend910A"},
        {2, "Ascend310P"},
        {3, "Ascend910B"},
        {4, "Ascend310B"},
        {5, "UNKNOWN"},
    };
    std::string sqlStr = "SELECT id, name FROM " + TABLE_NAME_NPU_INFO;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkNpuInfoData));
    EXPECT_EQ(expectNpuInfoData, checkNpuInfoData);

    RankDeviceMapFormat checkRankDeviceMapData;
    RankDeviceMapFormat expectRankDeviceMapData = {
        {-1, 0},
        {-1, 1},
        {-1, 2},
        {-1, 3},
        {-1, 4},
        {-1, 5},
    };
    sqlStr = "SELECT rankId, deviceId FROM " + TABLE_NAME_RANK_DEVICE_MAP;
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkRankDeviceMapData));
    EXPECT_EQ(expectRankDeviceMapData, checkRankDeviceMapData);
}

TEST_F(DBAssemblerUTest, TestRunNpuDataShouldReturnTrueWhenNoDevice)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunNpuMemDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateNpuMemData();
    std::shared_ptr<std::vector<NpuMemData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<NpuMemData>, data);
    dataInventory.Inject<std::vector<NpuMemData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunNpuMemDataShouldReturnFalseWhenReserveFailed)
{
    // type, ddr, hbm, timestamp, deviceId
    using SaveDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateNpuMemData();
    std::shared_ptr<std::vector<NpuMemData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<NpuMemData>, data);
    dataInventory.Inject<std::vector<NpuMemData>>(dataS);
    // Reserve failed
    StubReserveFailureForVector<SaveDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<SaveDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunPCIeDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GeneratePCIeData();
    std::shared_ptr<std::vector<PCIeData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<PCIeData>, data);
    dataInventory.Inject<std::vector<PCIeData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunPCIeDataShouldReturnFalseWhenReserveFailed)
{
    // deviceId, timestampNs, txPostMin, txPostMax, txPostAvg, txNonpostMin, txNonpostMax, txNonpostAvg,
    // txCplMin, txCplMax, txCplAvg, txNonpostLatencyMin, txNonpostLatencyMax, txNonpostLatencyAvg,
    // rxPostMin, rxPostMax, rxPostAvg, rxNonpostMin, rxNonpostMax, rxNonpostAvg, rxCplMin, rxCplMax, rxCplAvg
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GeneratePCIeData();
    std::shared_ptr<std::vector<PCIeData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<PCIeData>, data);
    dataInventory.Inject<std::vector<PCIeData>>(dataS);
    // Reserve failed
    StubReserveFailureForVector<SaveDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<SaveDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunSessionTimeInfoShouldReturnTrueWhenProcessorRunSuccess)
{
    using TimeDataFormat = std::vector<std::tuple<uint64_t, uint64_t>>;
    Utils::ProfTimeRecord expectRecord{1715760307197379000, 1715760313397397000, UINT64_MAX};
    auto dataInventory = DataInventory();
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, GetMsprofDbPath());
    TimeDataFormat checkData;
    TimeDataFormat expectData = {{expectRecord.startTimeNs, expectRecord.endTimeNs}};
    std::string sqlStr = "SELECT startTimeNs, endTimeNs FROM " + TABLE_NAME_SESSION_TIME_INFO;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectData, checkData);
}

TEST_F(DBAssemblerUTest, TestRunSessionTimeInfoShouldReturnFalseWhenGetTimeFailed)
{
    using TimeDataFormat = std::vector<std::tuple<uint64_t, uint64_t>>;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    auto dataInventory = DataInventory();
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
}

TEST_F(DBAssemblerUTest, TestRunSocShouldReturnTrueWhenProcessorRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateSocData();
    std::shared_ptr<std::vector<SocBandwidthData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<SocBandwidthData>, data);
    dataInventory.Inject<std::vector<SocBandwidthData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSocShouldReturnFalseWhenGetTimeFailed)
{
    // l2_buffer_bw_level, mata_bw_level, timestamp, deviceId
    using SaveDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateSocData();
    std::shared_ptr<std::vector<SocBandwidthData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<SocBandwidthData>, data);
    dataInventory.Inject<std::vector<SocBandwidthData>>(dataS);
    // Reserve failed
    StubReserveFailureForVector<SaveDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<SaveDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunStringIdsShouldReturnTrueWhenProcessorRunSuccess)
{
    using IDS_DATA_FORMAT = std::vector<std::tuple<uint64_t, std::string>>;
    IDS_DATA_FORMAT result;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    IdPool::GetInstance().GetUint64Id("pool");
    IdPool::GetInstance().GetUint64Id("Conv2d");
    EXPECT_TRUE(assembler.Run(dataInventory));

    std::string sql{"SELECT * FROM STRING_IDS"};
    std::shared_ptr<DBRunner> MsprofDBRunner;
    MAKE_SHARED0_NO_OPERATION(MsprofDBRunner, DBRunner, GetMsprofDbPath());
    MsprofDBRunner->QueryData(sql, result);
    CheckStringId(result);
}

TEST_F(DBAssemblerUTest, TestRunStringIdsShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<uint64_t, std::string>;
    using ProcessedDataFormat = std::vector<std::tuple<uint64_t, std::string>>;
    StubReserveFailureForVector<ProcessedDataFormat>();
    IdPool::GetInstance().GetUint64Id("pool");
    IdPool::GetInstance().GetUint64Id("Conv2d");
    auto dataInventory = DataInventory();
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<ProcessedDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunSysIOShouldReturnTrueWhenProcessorRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto nicData = GenerateNicData();
    auto roceData = GenerateRoceData();
    std::shared_ptr<std::vector<NicOriginalData>> nicDataS;
    std::shared_ptr<std::vector<RoceOriginalData>> roceDataS;
    MAKE_SHARED0_NO_OPERATION(nicDataS, std::vector<NicOriginalData>, nicData);
    MAKE_SHARED0_NO_OPERATION(roceDataS, std::vector<RoceOriginalData>, roceData);

    dataInventory.Inject(nicDataS);
    dataInventory.Inject(roceDataS);
    EXPECT_TRUE(assembler.Run(dataInventory));

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, GetMsprofDbPath());
    std::string sqlStrNic = "SELECT deviceId, timestampNs FROM " + TABLE_NAME_NIC;
    std::string sqlStrRoce = "SELECT deviceId, timestampNs FROM " + TABLE_NAME_ROCE;
    ASSERT_NE(dbRunner, nullptr);
    std::vector<std::tuple<uint16_t, uint64_t>> resNic;
    std::vector<std::tuple<uint16_t, uint64_t>> resRoce;
    std::vector<std::tuple<uint16_t, uint64_t>> expectNic = {{1, 236368325745555}};
    std::vector<std::tuple<uint16_t, uint64_t>> expectRoce = {{2, 236368325746666}};

    EXPECT_TRUE(dbRunner->QueryData(sqlStrNic, resNic));
    EXPECT_EQ(expectNic, resNic);

    EXPECT_TRUE(dbRunner->QueryData(sqlStrRoce, resRoce));
    EXPECT_EQ(expectRoce, resRoce);
}

TEST_F(DBAssemblerUTest, TestRunSysIOShouldReturnTrueWhenRoceDataExistButSysIOOriginalDataEmpty)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto emptyData = GenerateEmptyRoceData();
    std::shared_ptr<std::vector<RoceOriginalData>> emptyDataS;
    MAKE_SHARED0_NO_OPERATION(emptyDataS, std::vector<RoceOriginalData>, emptyData);
    // test Run when roceData exist but sysIOOriginalData is empty
    dataInventory.Inject(emptyDataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}


TEST_F(DBAssemblerUTest, TestRunSysIOShouldReturnFalseWhenGetTimeFailed)
{
    // deviceId, timestamp, bandwidth, rxPacketRate, rxByteRate, rxPackets, rxBytes, rxErrors, rxDropped
    // txPacketRate, txByteRate, txPackets, txBytes, txErrors, txDropped, funcId
    using SysIODataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, double, double, uint32_t,
        uint32_t, uint32_t, uint32_t, double, double, uint32_t, uint32_t, uint32_t, uint32_t, uint16_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto nicData = GenerateNicData();
    std::shared_ptr<std::vector<NicOriginalData>> nicDataS;
    MAKE_SHARED0_NO_OPERATION(nicDataS, std::vector<NicOriginalData>, nicData);
    dataInventory.Inject(nicDataS);

    // Reserve failed
    StubReserveFailureForVector<SysIODataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<SysIODataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunShouldReturnTrueWhenDataIsEmpty)
{
    IdPool::GetInstance().Clear();
    auto dataInventory = DataInventory();
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveNpuOpMemDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateNpuOpMemData();
    std::shared_ptr<std::vector<NpuOpMemData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<NpuOpMemData>, data);
    dataInventory.Inject<std::vector<NpuOpMemData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveNpuModuleMemDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateNpuModuleMemData();
    std::shared_ptr<std::vector<NpuModuleMemData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<NpuModuleMemData>, data);
    dataInventory.Inject<std::vector<NpuModuleMemData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveAscendTaskDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateAscendTaskData();
    auto deviceTx = GenerateDeviceTxData();
    std::shared_ptr<std::vector<MsprofTxDeviceData>> txDataS;
    std::shared_ptr<std::vector<AscendTaskData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<AscendTaskData>, data);
    MAKE_SHARED0_NO_OPERATION(txDataS, std::vector<MsprofTxDeviceData>, deviceTx);
    dataInventory.Inject<std::vector<AscendTaskData>>(dataS);
    dataInventory.Inject<std::vector<MsprofTxDeviceData>>(txDataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveComputeTaskInfoShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateComputeTaskInfo();
    auto kfcStream = GenerateKfcStreamData();
    std::shared_ptr<std::vector<MC2CommInfoData>> kfcStreamS;
    std::shared_ptr<std::vector<TaskInfoData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<TaskInfoData>, data);
    MAKE_SHARED0_NO_OPERATION(kfcStreamS, std::vector<MC2CommInfoData>, kfcStream);
    dataInventory.Inject<std::vector<TaskInfoData>>(dataS);
    dataInventory.Inject(kfcStreamS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveMemcpyInfoDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateMemcpyInfoData();
    std::shared_ptr<std::vector<MemcpyInfoData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<MemcpyInfoData>, data);
    dataInventory.Inject<std::vector<MemcpyInfoData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveMemcpyInfoDataShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateMemcpyInfoData();
    std::shared_ptr<std::vector<MemcpyInfoData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<MemcpyInfoData>, data);
    dataInventory.Inject<std::vector<MemcpyInfoData>>(dataS);
    StubReserveFailureForVector<ProcessedDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<ProcessedDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunAssemblerShouldReturnFalseWhenProfPathIsEmpty)
{
    auto assembler = DBAssembler("", OUTPUT_PATH);
    auto dataInventory = DataInventory();
    EXPECT_FALSE(assembler.Run(dataInventory));
}

 	 
TEST_F(DBAssemblerUTest, TestSaveMetaDataShouldReturnFalseWhenReserveFailed)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    using DataFormat = std::vector<std::tuple<std::string, std::string>>;
    StubReserveFailureForVector<DataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<DataFormat>();
}

TEST_F(DBAssemblerUTest, TestSaveNpuOpMemDataShouldReturnFalseWhenReserveFailed)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    using NpuOpMemDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                                      uint64_t, uint64_t, uint64_t, uint16_t>>;
    std::vector<NpuOpMemData> res;
    NpuOpMemData data;
    res.push_back(data);
    std::shared_ptr<std::vector<NpuOpMemData>> dataSave;
    MAKE_SHARED0_NO_OPERATION(dataSave, std::vector<NpuOpMemData>, res);
    dataInventory.Inject<std::vector<NpuOpMemData>>(dataSave);
    StubReserveFailureForVector<NpuOpMemDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<NpuOpMemDataFormat>();
}

TEST_F(DBAssemblerUTest, TestSaveComputeTaskInfoShouldReturnFalseWhenReserveFailed)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    using ComputeTaskInfoFormat = std::vector<std::tuple<uint64_t, uint64_t, uint32_t, uint32_t,
                                                         uint64_t, uint64_t, uint64_t, uint64_t,
                                                         uint64_t, uint64_t, uint64_t, uint64_t,
                                                         uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
    std::vector<TaskInfoData> res;
    TaskInfoData data;
    res.push_back(data);
    std::shared_ptr<std::vector<TaskInfoData>> dataSave;
    MAKE_SHARED0_NO_OPERATION(dataSave, std::vector<TaskInfoData>, res);
    dataInventory.Inject<std::vector<TaskInfoData>>(dataSave);
    StubReserveFailureForVector<ComputeTaskInfoFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<ComputeTaskInfoFormat>();
}

TEST_F(DBAssemblerUTest, TestSaveAscendTaskDataShouldReturnFalseWhenReserveFailed)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    using ascendTaskDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint32_t, int64_t, uint64_t,
                                                       uint32_t, uint32_t, uint32_t, int32_t, uint32_t, uint32_t>>;
    std::vector<AscendTaskData> res;
    AscendTaskData data;
    res.push_back(data);
    std::shared_ptr<std::vector<AscendTaskData>> dataSave;
    MAKE_SHARED0_NO_OPERATION(dataSave, std::vector<AscendTaskData>, res);
    dataInventory.Inject<std::vector<AscendTaskData>>(dataSave);
    StubReserveFailureForVector<ascendTaskDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<ascendTaskDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunSaveTaskPmuDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateUnifiedTaskPmuData();
    std::shared_ptr<std::vector<UnifiedTaskPmu>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<UnifiedTaskPmu>, data);
    dataInventory.Inject<std::vector<UnifiedTaskPmu>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveSamplePmuTimelineDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateUnifiedSampleTimelinePmuData();
    std::shared_ptr<std::vector<UnifiedSampleTimelinePmu>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<UnifiedSampleTimelinePmu>, data);
    dataInventory.Inject<std::vector<UnifiedSampleTimelinePmu>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveSamplePmuSummaryDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateUnifiedSampleSummaryPmuData();
    std::shared_ptr<std::vector<UnifiedSampleSummaryPmu>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<UnifiedSampleSummaryPmu>, data);
    dataInventory.Inject<std::vector<UnifiedSampleSummaryPmu>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveTaskPmuDataShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using PTFormat = std::vector<std::tuple<uint64_t, uint64_t, double>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateUnifiedTaskPmuData();
    std::shared_ptr<std::vector<UnifiedTaskPmu>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<UnifiedTaskPmu>, data);
    dataInventory.Inject<std::vector<UnifiedTaskPmu>>(dataS);
    StubReserveFailureForVector<PTFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<PTFormat>();
}

TEST_F(DBAssemblerUTest, TestRunSaveSamplePmuTimelineDataShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using PSTFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, double, double, uint16_t, uint64_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateUnifiedSampleTimelinePmuData();
    std::shared_ptr<std::vector<UnifiedSampleTimelinePmu>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<UnifiedSampleTimelinePmu>, data);
    dataInventory.Inject<std::vector<UnifiedSampleTimelinePmu>>(dataS);
    StubReserveFailureForVector<PSTFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<PSTFormat>();
}

TEST_F(DBAssemblerUTest, TestRunSaveSamplePmuSummaryDataShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using PSSFormat = std::vector<std::tuple<uint16_t, uint64_t, double, uint16_t, uint64_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateUnifiedSampleSummaryPmuData();
    std::shared_ptr<std::vector<UnifiedSampleSummaryPmu>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<UnifiedSampleSummaryPmu>, data);
    dataInventory.Inject<std::vector<UnifiedSampleSummaryPmu>>(dataS);
    StubReserveFailureForVector<PSSFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<PSSFormat>();
}

TEST_F(DBAssemblerUTest, TestRunSaveTaskPmuDataShouldReturnTrueWhenTaskPmuDataIsNotExist)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveSamplePmuTimelineDataShouldReturnTrueWhenSamplePmuTimelineDataIsNotExist)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveSamplePmuSummaryDataShouldReturnTrueWhenSamplePmuSummaryDataIsNotExist)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveCpuUsageDataShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using format = std::vector<std::tuple<uint64_t, uint64_t, double>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateCpuUsageData();
    std::shared_ptr<std::vector<CpuUsageData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<CpuUsageData>, data);
    dataInventory.Inject<std::vector<CpuUsageData>>(dataS);
    StubReserveFailureForVector<format>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<format>();
}

TEST_F(DBAssemblerUTest, TestRunSaveCpuUsageDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateCpuUsageData();
    std::shared_ptr<std::vector<CpuUsageData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<CpuUsageData>, data);
    dataInventory.Inject<std::vector<CpuUsageData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveHostMemUsageDataShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using format = std::vector<std::tuple<uint64_t, double>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateHostMemUsageData();
    std::shared_ptr<std::vector<MemUsageData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<MemUsageData>, data);
    dataInventory.Inject<std::vector<MemUsageData>>(dataS);
    StubReserveFailureForVector<format>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<format>();
}

TEST_F(DBAssemblerUTest, TestRunSaveHostMemUsageDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateHostMemUsageData();
    std::shared_ptr<std::vector<MemUsageData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<MemUsageData>, data);
    dataInventory.Inject<std::vector<MemUsageData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveHostDiskUsageDataShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using format = std::vector<std::tuple<uint64_t, double, double, double>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateHostDiskUsageData();
    std::shared_ptr<std::vector<DiskUsageData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<DiskUsageData>, data);
    dataInventory.Inject<std::vector<DiskUsageData>>(dataS);
    StubReserveFailureForVector<format>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<format>();
}

TEST_F(DBAssemblerUTest, TestRunSaveHostDiskUsageDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateHostDiskUsageData();
    std::shared_ptr<std::vector<DiskUsageData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<DiskUsageData>, data);
    dataInventory.Inject<std::vector<DiskUsageData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveHostNetworkUsageDataShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using format = std::vector<std::tuple<uint64_t, double, double>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateHostNetWorkUsageData();
    std::shared_ptr<std::vector<NetWorkUsageData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<NetWorkUsageData>, data);
    dataInventory.Inject<std::vector<NetWorkUsageData>>(dataS);
    StubReserveFailureForVector<format>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<format>();
}

TEST_F(DBAssemblerUTest, TestRunSaveHostNetworkUsageDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateHostNetWorkUsageData();
    std::shared_ptr<std::vector<NetWorkUsageData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<NetWorkUsageData>, data);
    dataInventory.Inject<std::vector<NetWorkUsageData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveOSRuntimeApiDataShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using format = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateOSRuntimeApiData();
    std::shared_ptr<std::vector<OSRuntimeApiData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<OSRuntimeApiData>, data);
    dataInventory.Inject<std::vector<OSRuntimeApiData>>(dataS);
    StubReserveFailureForVector<format>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<format>();
}

TEST_F(DBAssemblerUTest, TestRunSaveOSRuntimeApiDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateOSRuntimeApiData();
    std::shared_ptr<std::vector<OSRuntimeApiData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<OSRuntimeApiData>, data);
    dataInventory.Inject<std::vector<OSRuntimeApiData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunNetDevStatDataShouldReturnFalseWhenReserveFailed)
{
    // deviceId, timestampNs, macTxPfcPkt, macRxPfcPkt, macTxByte, macTxBandwidth,
    // macRxByte, macRxBandwidth, macTxBadByte, macRxBadByte, roceTxPkt, roceRxPkt, roceTxErrPkt, roceRxErrPkt,
    // roceTxCnpPkt, roceRxCnpPkt, roceNewPktRty, nicTxByte, nicTxBandwidth, nicRxByte, nicRxBandwidth
    using NetDevStatEventDataFormat =
        std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t, uint64_t, double,
                               uint64_t, double, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                               uint64_t, uint64_t, uint64_t, uint64_t, double, uint64_t, double>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    // Empty data
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto data = GenerateNetDevStatsData();
    std::shared_ptr<std::vector<NetDevStatsEventData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<NetDevStatsEventData>, data);
    dataInventory.Inject<std::vector<NetDevStatsEventData>>(dataS);
    // Reserve failed
    StubReserveFailureForVector<NetDevStatEventDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<NetDevStatEventDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunSaveNetDevStatsDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateNetDevStatsData();
    std::shared_ptr<std::vector<NetDevStatsEventData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<NetDevStatsEventData>, data);
    dataInventory.Inject<std::vector<NetDevStatsEventData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveQosDataShouldReturnTrueWhenReserveFailed)
{
    // deviceId, eventName, bandwidth, timestampNs
    using QosDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, int32_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateQosData();
    std::shared_ptr<std::vector<QosData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<QosData>, data);
    dataInventory.Inject<std::vector<QosData>>(dataS);
    // Reserve failed
    StubReserveFailureForVector<QosDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<QosDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunSaveQosDataShouldReturnTrueWhenDataNotExistOrRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    // Empty data
    EXPECT_TRUE(assembler.Run(dataInventory));

    // Run success
    auto data = GenerateQosData();
    std::shared_ptr<std::vector<QosData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<QosData>, data);
    dataInventory.Inject<std::vector<QosData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveDPUDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateDPUData();
    std::shared_ptr<std::vector<DPUData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<DPUData>, data);
    dataInventory.Inject<std::vector<DPUData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));

    // dpuDeviceId, globalTid, startNs, endNs, globalTaskId, streamId, taskId, opName, args
    using DPUDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint64_t, uint64_t,
        uint64_t, uint16_t, uint32_t, uint64_t, uint64_t>>;
    DPUDataFormat dpuResult;
    std::shared_ptr<DBRunner> msprofDBRunner;
    MAKE_SHARED0_NO_OPERATION(msprofDBRunner, DBRunner, GetMsprofDbPath());
    ASSERT_NE(msprofDBRunner, nullptr);
    std::string sql = "SELECT dpuDeviceId, globalTid, startNs, endNs, globalTaskId, streamId, taskId, opName, args FROM "
        + TABLE_NAME_DPU_TASK + " ORDER BY startNs";
    EXPECT_TRUE(msprofDBRunner->QueryData(sql, dpuResult));
    ASSERT_EQ(2, dpuResult.size());
    EXPECT_EQ(2, std::get<0>(dpuResult[0]));
    EXPECT_EQ(123, std::get<1>(dpuResult[0]));
    EXPECT_EQ(1717575960208020750, std::get<2>(dpuResult[0]));
    EXPECT_EQ(1717575960208021750, std::get<3>(dpuResult[0]));
    EXPECT_EQ(7, std::get<5>(dpuResult[0]));
    EXPECT_EQ(23, std::get<6>(dpuResult[0]));
    EXPECT_EQ(3, std::get<0>(dpuResult[1]));
    EXPECT_EQ(124, std::get<1>(dpuResult[1]));
    EXPECT_EQ(8, std::get<5>(dpuResult[1]));
    EXPECT_EQ(24, std::get<6>(dpuResult[1]));

    using StringIdFormat = std::vector<std::tuple<uint64_t, std::string>>;
    StringIdFormat stringIds;
    sql = "SELECT id, value FROM " + TABLE_NAME_STRING_IDS;
    EXPECT_TRUE(msprofDBRunner->QueryData(sql, stringIds));
    std::unordered_map<uint64_t, std::string> stringIdMap;
    for (const auto& item : stringIds) {
        stringIdMap[std::get<0>(item)] = std::get<1>(item);
    }

    EXPECT_EQ("dpu_kernel", stringIdMap[std::get<7>(dpuResult[0])]);
    EXPECT_EQ("dpu_hccl", stringIdMap[std::get<7>(dpuResult[1])]);
    const auto& taskArgs = stringIdMap[std::get<8>(dpuResult[0])];
    EXPECT_NE(std::string::npos, taskArgs.find("Thread Id"));
    EXPECT_NE(std::string::npos, taskArgs.find("Task Type"));
    const auto& hcclArgs = stringIdMap[std::get<8>(dpuResult[1])];
    EXPECT_NE(std::string::npos, hcclArgs.find("OP Type"));
    EXPECT_NE(std::string::npos, hcclArgs.find("AI CPU Device Id"));
    EXPECT_NE(std::string::npos, hcclArgs.find("Group Name"));
    EXPECT_NE(std::string::npos, hcclArgs.find("Bandwidth(B/s)"));
    EXPECT_NE(std::string::npos, hcclArgs.find("1024000"));
}

TEST_F(DBAssemblerUTest, TestRunSaveDPUDataShouldReturnFalseWhenReserveFailed)
{
    using DPUDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint64_t, uint64_t,
        uint64_t, uint16_t, uint32_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateDPUData();
    std::shared_ptr<std::vector<DPUData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<DPUData>, data);
    dataInventory.Inject<std::vector<DPUData>>(dataS);

    StubReserveFailureForVector<DPUDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<DPUDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunSaveSIODataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateSioData();
    std::shared_ptr<std::vector<SioData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<SioData>, data);
    dataInventory.Inject<std::vector<SioData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));

    using SioDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t,
        double, double, double, double, double, double, double, double>>;
    SioDataFormat sioResult;
    std::shared_ptr<DBRunner> msprofDBRunner;
    MAKE_SHARED0_NO_OPERATION(msprofDBRunner, DBRunner, GetMsprofDbPath());
    ASSERT_NE(msprofDBRunner, nullptr);
    std::string sql = "SELECT deviceId, name, timestampNs, rxReq, rxRsp, rxSnp, rxDat, txReq, txRsp, txSnp, txDat FROM "
        + TABLE_NAME_SIO;
    EXPECT_TRUE(msprofDBRunner->QueryData(sql, sioResult));
    ASSERT_EQ(1, sioResult.size());
    EXPECT_EQ(0, std::get<0>(sioResult[0]));
    EXPECT_EQ(1746706291651036160, std::get<2>(sioResult[0]));
    EXPECT_DOUBLE_EQ(1.0 * BYTE_SIZE * BYTE_SIZE, std::get<3>(sioResult[0]));
    EXPECT_DOUBLE_EQ(2.0 * BYTE_SIZE * BYTE_SIZE, std::get<4>(sioResult[0]));
    EXPECT_DOUBLE_EQ(8.0 * BYTE_SIZE * BYTE_SIZE, std::get<10>(sioResult[0]));

    using StringIdFormat = std::vector<std::tuple<uint64_t, std::string>>;
    StringIdFormat stringIds;
    sql = "SELECT id, value FROM " + TABLE_NAME_STRING_IDS;
    EXPECT_TRUE(msprofDBRunner->QueryData(sql, stringIds));
    std::unordered_map<uint64_t, std::string> stringIdMap;
    for (const auto& item : stringIds) {
        stringIdMap[std::get<0>(item)] = std::get<1>(item);
    }
    EXPECT_EQ("die 0", stringIdMap[std::get<1>(sioResult[0])]);
}

TEST_F(DBAssemblerUTest, TestRunSaveSIODataShouldReturnFalseWhenReserveFailed)
{
    using SioDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t,
        double, double, double, double, double, double, double, double>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateSioData();
    std::shared_ptr<std::vector<SioData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<SioData>, data);
    dataInventory.Inject<std::vector<SioData>>(dataS);

    StubReserveFailureForVector<SioDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<SioDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunSaveUBDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateUbData();
    std::shared_ptr<std::vector<UbData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<UbData>, data);
    dataInventory.Inject<std::vector<UbData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));

    using UbDataFormat = std::vector<std::tuple<uint16_t, uint16_t, uint64_t, uint64_t, uint64_t>>;
    UbDataFormat ubResult;
    std::shared_ptr<DBRunner> msprofDBRunner;
    MAKE_SHARED0_NO_OPERATION(msprofDBRunner, DBRunner, GetMsprofDbPath());
    ASSERT_NE(msprofDBRunner, nullptr);
    std::string sql = "SELECT deviceId, portId, timestampNs, rxUdmaBind, txUdmaBind FROM " + TABLE_NAME_UB;
    EXPECT_TRUE(msprofDBRunner->QueryData(sql, ubResult));
    ASSERT_EQ(1, ubResult.size());
    EXPECT_EQ(0, std::get<0>(ubResult[0]));
    EXPECT_EQ(3, std::get<1>(ubResult[0]));
    EXPECT_EQ(1746706291651036160, std::get<2>(ubResult[0]));
    EXPECT_EQ(100, std::get<3>(ubResult[0]));
    EXPECT_EQ(200, std::get<4>(ubResult[0]));
}

TEST_F(DBAssemblerUTest, TestRunSaveUBDataShouldReturnFalseWhenReserveFailed)
{
    using UbDataFormat = std::vector<std::tuple<uint16_t, uint16_t, uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateUbData();
    std::shared_ptr<std::vector<UbData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<UbData>, data);
    dataInventory.Inject<std::vector<UbData>>(dataS);

    StubReserveFailureForVector<UbDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<UbDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunSaveOverlapAnalysisDataShouldReturnTrueWhenRunSuccess)
{
    using OverlapAnalysisDataFormat = std::vector<std::tuple<uint64_t, uint16_t, uint64_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateOverlapAnalysisData();
    std::shared_ptr<std::vector<OverlapAnalysisData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<OverlapAnalysisData>, data);
    dataInventory.Inject<std::vector<OverlapAnalysisData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));

    OverlapAnalysisDataFormat overlapResult;
    const OverlapAnalysisDataFormat expectResult = {
        std::make_tuple(0, 0, 1000, 3000, static_cast<uint16_t>(OverlapAnalysisType::COMPUTE)),
        std::make_tuple(1, 0, 4000, 7000, static_cast<uint16_t>(OverlapAnalysisType::COMMUNICATION)),
        std::make_tuple(2, 1, 9000, 10000, static_cast<uint16_t>(OverlapAnalysisType::COMM_NOT_OVERLAP_COMP)),
        std::make_tuple(3, 1, 12000, 16000, static_cast<uint16_t>(OverlapAnalysisType::FREE))};
    std::shared_ptr<DBRunner> msprofDBRunner;
    MAKE_SHARED0_NO_OPERATION(msprofDBRunner, DBRunner, GetMsprofDbPath());
    ASSERT_NE(msprofDBRunner, nullptr);
    std::string sql = "SELECT id, deviceId, startNs, endNs, type FROM " + TABLE_NAME_OVERLAP_ANALYSIS +
        " ORDER BY id";
    EXPECT_TRUE(msprofDBRunner->QueryData(sql, overlapResult));
    EXPECT_EQ(expectResult, overlapResult);
}

TEST_F(DBAssemblerUTest, TestRunSaveOverlapAnalysisDataShouldReturnFalseWhenReserveFailed)
{
    using OverlapAnalysisDataFormat = std::vector<std::tuple<uint64_t, uint16_t, uint64_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateOverlapAnalysisData();
    std::shared_ptr<std::vector<OverlapAnalysisData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<OverlapAnalysisData>, data);
    dataInventory.Inject<std::vector<OverlapAnalysisData>>(dataS);

    StubReserveFailureForVector<OverlapAnalysisDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<OverlapAnalysisDataFormat>();
}

TEST_F(DBAssemblerUTest, TestRunSaveCCUDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateCCUData();
    std::shared_ptr<std::vector<CCUMissionTimelineData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<CCUMissionTimelineData>, data);
    dataInventory.Inject<std::vector<CCUMissionTimelineData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));

    // deviceId, globalTaskId, name, startNs, endNs, args
    using CCUDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
    CCUDataFormat ccuResult;
    std::shared_ptr<DBRunner> msprofDBRunner;
    MAKE_SHARED0_NO_OPERATION(msprofDBRunner, DBRunner, GetMsprofDbPath());
    ASSERT_NE(msprofDBRunner, nullptr);
    std::string sql = "SELECT deviceId, globalTaskId, name, startNs, endNs, args FROM " + TABLE_NAME_CCU +
        " ORDER BY startNs";
    EXPECT_TRUE(msprofDBRunner->QueryData(sql, ccuResult));
    ASSERT_EQ(2, ccuResult.size());
    EXPECT_EQ(0, std::get<0>(ccuResult[0]));
    EXPECT_EQ(1717575960208020750, std::get<3>(ccuResult[0]));
    EXPECT_EQ(1717575960208021750, std::get<4>(ccuResult[0]));
    EXPECT_EQ(1, std::get<0>(ccuResult[1]));
    EXPECT_EQ(1717575960209020750, std::get<3>(ccuResult[1]));
    EXPECT_EQ(1717575960209022750, std::get<4>(ccuResult[1]));

    using StringIdFormat = std::vector<std::tuple<uint64_t, std::string>>;
    StringIdFormat stringIds;
    sql = "SELECT id, value FROM " + TABLE_NAME_STRING_IDS;
    EXPECT_TRUE(msprofDBRunner->QueryData(sql, stringIds));
    std::unordered_map<uint64_t, std::string> stringIdMap;
    for (const auto& item : stringIds) {
        stringIdMap[std::get<0>(item)] = std::get<1>(item);
    }

    EXPECT_EQ(CCU_TIME_TYPE_LOOP_GROUP, stringIdMap[std::get<2>(ccuResult[0])]);
    EXPECT_EQ(CCU_TIME_TYPE_WAIT, stringIdMap[std::get<2>(ccuResult[1])]);
    const auto& loopArgs = stringIdMap[std::get<5>(ccuResult[0])];
    EXPECT_NE(std::string::npos, loopArgs.find("Instruction ID"));
    EXPECT_NE(std::string::npos, loopArgs.find("Die Id"));
    EXPECT_NE(std::string::npos, loopArgs.find("Data Size"));
    EXPECT_NE(std::string::npos, loopArgs.find("Bandwidth (B/s)"));
    EXPECT_NE(std::string::npos, loopArgs.find("Reduce Op Type"));
    EXPECT_NE(std::string::npos, loopArgs.find("Mask"));
    EXPECT_NE(std::string::npos, loopArgs.find("Maximum Delay Channel"));

    const auto& waitArgs = stringIdMap[std::get<5>(ccuResult[1])];
    EXPECT_NE(std::string::npos, waitArgs.find("Notify Instruction ID"));
    EXPECT_NE(std::string::npos, waitArgs.find("Notify Rank ID"));
    EXPECT_EQ(std::string::npos, waitArgs.find("Data Size"));
}

TEST_F(DBAssemblerUTest, TestRunSaveCCUDataShouldReturnTrueWhenDataNotExistOrEmpty)
{
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));
    EXPECT_TRUE(File::RemoveDir(OUTPUT_PATH, DEPTH));
    EXPECT_TRUE(File::CreateDir(OUTPUT_PATH));

    std::vector<CCUMissionTimelineData> data;
    std::shared_ptr<std::vector<CCUMissionTimelineData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<CCUMissionTimelineData>, data);
    dataInventory.Inject<std::vector<CCUMissionTimelineData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSaveCCUDataShouldReturnFalseWhenReserveFailed)
{
    // deviceId, globalTaskId, name, startNs, endNs, args
    using CCUDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(PROF, OUTPUT_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateCCUData();
    std::shared_ptr<std::vector<CCUMissionTimelineData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<CCUMissionTimelineData>, data);
    dataInventory.Inject<std::vector<CCUMissionTimelineData>>(dataS);

    StubReserveFailureForVector<CCUDataFormat>();
    EXPECT_FALSE(assembler.Run(dataInventory));
    ResetReserveFailureForVector<CCUDataFormat>();
}
