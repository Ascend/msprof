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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "analysis/csrc/domain/services/persistence/device/ts_track_persistence.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/domain/services/parser/track/include/ts_track_parser.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace testing;
using namespace Analysis::Utils;

namespace Analysis {
using namespace Analysis;
using namespace Analysis::Infra;
using namespace Analysis::Domain;
using namespace Analysis::Utils;

namespace {
// timestamp stream_id task_id task_type task_state
using TaskTypeDataFormat = std::tuple<uint64_t, uint16_t, uint16_t, uint64_t, uint16_t>;
// index_id model_id timestamp stream_id task_id tag_id
using StepTraceDataFormat = std::tuple<uint64_t, uint64_t, uint64_t, uint16_t, uint16_t, uint16_t>;
// timestamp stream_id task_id task_state
using TaskMemcpyDataFormat = std::tuple<uint64_t, uint16_t, uint16_t, uint64_t>;
// timestamp stream_id task_id block_num
using BlockNumDataFormat = std::tuple<uint64_t, uint16_t, uint16_t, uint32_t>;
const std::vector<TaskTypeDataFormat> TASK_TYPE_DATA{};
const std::vector<StepTraceDataFormat> STEP_TRACE_DATA{};
const std::vector<TaskMemcpyDataFormat> TS_MEMCPY_DATA{};
const std::vector<BlockNumDataFormat> BLOCK_NUM_DATA{};
const std::string DEVICE_PATH = "./device";
const std::string STEP_TRACE_DB_PATH = File::PathJoin({DEVICE_PATH, "sqlite", "step_trace.db"});
const int BLOCK_NUM_SIZE = 15;
const int TASK_TYPE_SIZE = 10;
const int TS_MEMCPY_SIZE = 9;
const int STEP_TRACE_SIZE = 8;
}

class TsTrackPersistenceUtest : public testing::Test {
protected:
    void SetUp() override
    {
        auto halTrackData = std::make_shared<std::vector<HalTrackData>>();
        EXPECT_TRUE(dataInventory_.Inject(halTrackData));
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({DEVICE_PATH, "sqlite"})));
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(DEVICE_PATH, 0));
    }

    std::vector<HalTrackData> CreateHalTaskType()
    {
        std::vector<HalTrackData> ans;
        for (int i = 0; i < TASK_TYPE_SIZE; i++) {
            HalTrackData trackData{};
            trackData.type = HalTrackType::TS_TASK_TYPE;
            trackData.hd.taskId.taskId = i;
            trackData.hd.taskId.streamId = i;
            trackData.taskType.taskType = i;
            trackData.taskType.taskStatus = i;
            ans.emplace_back(std::move(trackData));
        }
        return ans;
    }

    std::vector<HalTrackData> CreateHalTsMemcpy()
    {
        std::vector<HalTrackData> ans;
        for (int i = 0; i < TS_MEMCPY_SIZE; i++) {
            HalTrackData trackData{};
            trackData.type = HalTrackType::TS_MEMCPY;
            trackData.hd.taskId.taskId = i;
            trackData.hd.taskId.streamId = i;
            trackData.taskType.taskStatus = i;
            ans.emplace_back(std::move(trackData));
        }
        return ans;
    }

    std::vector<HalTrackData> CreateHalStepTrace()
    {
        std::vector<HalTrackData> ans;
        for (int i = 0; i < STEP_TRACE_SIZE; i++) {
            HalTrackData trackData{};
            trackData.type = HalTrackType::STEP_TRACE;
            trackData.hd.taskId.taskId = i;
            trackData.hd.taskId.streamId = i;
            trackData.stepTrace.timestamp = i;
            trackData.stepTrace.indexId = i;
            trackData.stepTrace.modelId = i;
            trackData.stepTrace.tagId = i;
            ans.emplace_back(std::move(trackData));
        }
        return ans;
    }

    std::vector<HalTrackData> CreateHalBlockNum()
    {
        std::vector<HalTrackData> ans;
        for (int i = 0; i < BLOCK_NUM_SIZE; i++) {
            HalTrackData trackData{};
            trackData.type = HalTrackType::BLOCK_NUM;
            trackData.hd.taskId.taskId = i;
            trackData.hd.taskId.streamId = i;
            trackData.blockNum.timestamp = i;
            trackData.blockNum.blockNum = i;
            ans.emplace_back(std::move(trackData));
        }
        return ans;
    }

protected:
    DataInventory dataInventory_;
};

TEST_F(TsTrackPersistenceUtest, ReturnSuccessWhenOnlyTaskType)
{
    auto halTaskType = CreateHalTaskType();
    auto halTrackData = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    halTrackData->swap(halTaskType);

    TsTrackPersistence tsTrackPersistence;
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    ASSERT_EQ(ANALYSIS_OK, tsTrackPersistence.Run(dataInventory_, deviceContext));

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, STEP_TRACE_DB_PATH);

    std::vector<TaskTypeDataFormat> taskTypeDatas;
    EXPECT_TRUE(dbRunner->QueryData("SELECT * from TaskType", taskTypeDatas));
    ASSERT_EQ(TASK_TYPE_SIZE, taskTypeDatas.size());
}

TEST_F(TsTrackPersistenceUtest, ReturnSuccessWhenOnlyBlockNum)
{
    auto halBlockNum = CreateHalBlockNum();
    auto halTrackData = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    halTrackData->swap(halBlockNum);

    TsTrackPersistence tsTrackPersistence;
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    ASSERT_EQ(ANALYSIS_OK, tsTrackPersistence.Run(dataInventory_, deviceContext));

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, STEP_TRACE_DB_PATH);

    std::vector<BlockNumDataFormat> blockNumDatas;
    EXPECT_TRUE(dbRunner->QueryData("SELECT * from TsBlockNum", blockNumDatas));
    ASSERT_EQ(BLOCK_NUM_SIZE, blockNumDatas.size());
}

TEST_F(TsTrackPersistenceUtest, ReturnSuccessWhenMultiTypeData)
{
    auto halTaskType = CreateHalTaskType();
    auto halTsMemcpy = CreateHalTsMemcpy();
    auto halStepTrace = CreateHalStepTrace();
    auto halBlockNum = CreateHalBlockNum();
    halTaskType.insert(halTaskType.begin(), halTsMemcpy.begin(), halTsMemcpy.end());
    halTaskType.insert(halTaskType.begin(), halStepTrace.begin(), halStepTrace.end());
    halTaskType.insert(halTaskType.begin(), halBlockNum.begin(), halBlockNum.end());
    auto halTrackData = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    halTrackData->swap(halTaskType);

    TsTrackPersistence tsTrackPersistence;
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    ASSERT_EQ(ANALYSIS_OK, tsTrackPersistence.Run(dataInventory_, deviceContext));

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, STEP_TRACE_DB_PATH);

    std::vector<TaskTypeDataFormat> taskTypeDatas;
    EXPECT_TRUE(dbRunner->QueryData("SELECT * from TaskType", taskTypeDatas));
    ASSERT_EQ(TASK_TYPE_SIZE, taskTypeDatas.size());

    std::vector<TaskMemcpyDataFormat> taskMemcpyDatas;
    EXPECT_TRUE(dbRunner->QueryData("SELECT * from TsMemcpy", taskMemcpyDatas));
    ASSERT_EQ(TS_MEMCPY_SIZE, taskMemcpyDatas.size());

    std::vector<StepTraceDataFormat> stepTraceDatas;
    EXPECT_TRUE(dbRunner->QueryData("SELECT * from StepTrace", stepTraceDatas));
    ASSERT_EQ(STEP_TRACE_SIZE, stepTraceDatas.size());

    std::vector<BlockNumDataFormat> blockNumDatas;
    EXPECT_TRUE(dbRunner->QueryData("SELECT * from TsBlockNum", blockNumDatas));
    ASSERT_EQ(BLOCK_NUM_SIZE, blockNumDatas.size());
}

TEST_F(TsTrackPersistenceUtest, TestRunShouldReturnErrorWhenDataIsNull)
{
    dataInventory_.RemoveRestData({});
    TsTrackPersistence tsTrackPersistence;
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    ASSERT_EQ(ANALYSIS_ERROR, tsTrackPersistence.Run(dataInventory_, deviceContext));
}
}