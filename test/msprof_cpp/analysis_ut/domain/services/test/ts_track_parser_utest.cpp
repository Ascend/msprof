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
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/track/include/ts_track_parser.h"
#include "analysis/csrc/domain/services/parser/parser_item/task_flip_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/step_trace_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/task_memcpy_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/task_type_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/task_block_num_parser_item.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"

using namespace testing;
using namespace Analysis::Utils;

namespace Analysis {
using namespace Analysis;
using namespace Analysis::Infra;
using namespace Analysis::Domain;
using namespace Analysis::Utils;

namespace {
const std::string TS_TRACK_PATH = "./ts_track";
const long INVALID_FILE_SIZE = -1;
}

class TsTrackParserUtest : public Test {
protected:
    void SetUp() override
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::CreateDir(TS_TRACK_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({TS_TRACK_PATH, "data"})));
    }
    void TearDown() override
    {
        GlobalMockObject::verify();
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(TS_TRACK_PATH, 0));
    }
    TaskFlip CreateTaskFlip(int funcType, int taskId, int streamId, int timestamp, int flipNum)
    {
        TaskFlip taskFlip{};
        taskFlip.funcType = funcType;
        taskFlip.taskId = taskId;
        taskFlip.streamId = streamId;
        taskFlip.timestamp = timestamp;
        taskFlip.flipNum = flipNum;
        return taskFlip;
    }

    StepTrace CreateStepTrace(int funcType, int taskId, int streamId, int timestamp, int tagId)
    {
        StepTrace stepTrace{};
        stepTrace.funcType = funcType;
        stepTrace.taskId = taskId;
        stepTrace.streamId = streamId;
        stepTrace.timestamp = timestamp;
        stepTrace.tagId = tagId;
        return stepTrace;
    }

    TaskType CreateTsTaskType(int funcType, int taskId, int streamId, int timestamp, int taskStatus)
    {
        TaskType taskType{};
        taskType.funcType = funcType;
        taskType.taskId = taskId;
        taskType.streamId = streamId;
        taskType.timestamp = timestamp;
        taskType.taskStatus = taskStatus;
        return taskType;
    }

    TaskMemcpy CreateTsTaskMemcpy(int funcType, int taskId, int timestamp, int taskStatus)
    {
        TaskMemcpy taskMemcpy{};
        taskMemcpy.funcType = funcType;
        taskMemcpy.taskId = taskId;
        taskMemcpy.timestamp = timestamp;
        taskMemcpy.taskStatus = taskStatus;
        return taskMemcpy;
    }

    BlockNum CreateTsBlockNum(int funcType, int taskId, int streamId, int timestamp)
    {
        BlockNum blockNum{};
        blockNum.funcType = funcType;
        blockNum.streamId = streamId;
        blockNum.taskId = taskId;
        blockNum.timestamp = timestamp;
        return blockNum;
    }

protected:
    Infra::DataInventory dataInventory_;
};

TEST_F(TsTrackParserUtest, ShouldReturnTaskFlipDataWhenParserRun)
{
    std::vector<int> expectTimestamp{100, INVALID_TIMESTAMP};
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = TS_TRACK_PATH;
    std::vector<TaskFlip> taskFlip{CreateTaskFlip(0x0e, 1, 1, 100, 5),
                                   CreateTaskFlip(0x0e, 1, 1, INVALID_TIMESTAMP, 5)};
    WriteBin(taskFlip, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_0");
    ASSERT_EQ(Analysis::ANALYSIS_OK, tsTrackParser.Run(dataInventory_, context));
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    ASSERT_EQ(2ul, data->size());
    for (int i = 0; i < data->size(); i++) {
        ASSERT_EQ(expectTimestamp[i], data->data()[i].hd.timestamp);
    }
}

TEST_F(TsTrackParserUtest, ShouldReturnTaskFlipDataWhenMultiData)
{
    std::vector<int> expectTimestamp{10, 20, 100, INVALID_TIMESTAMP};
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = TS_TRACK_PATH;
    std::vector<TaskFlip> taskFlip1{CreateTaskFlip(0x0e, 1, 1, 10, 5), CreateTaskFlip(0x0e, 1, 1, 20, 5)};
    WriteBin(taskFlip1, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_0");
    std::vector<TaskFlip> taskFlip2{CreateTaskFlip(0x0e, 1, 1, 100,  5),
                                    CreateTaskFlip(0x0e, 1, 1, INVALID_TIMESTAMP, 5)};
    WriteBin(taskFlip2, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_1");
    ASSERT_EQ(tsTrackParser.Run(dataInventory_, context), Analysis::ANALYSIS_OK);
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    ASSERT_EQ(4ul, data->size());
    for (int i = 0; i < data->size(); i++) {
        ASSERT_EQ(expectTimestamp[i], data->data()[i].hd.timestamp);
    }
}

TEST_F(TsTrackParserUtest, ShouldReturnStepTraceDataWhenParser)
{
    std::vector<int> expectTimestamp{10, 20};
    std::vector<int> expectTagId{1, 2};
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = TS_TRACK_PATH;
    std::vector<StepTrace> taskFlip1{CreateStepTrace(0x0A, 1, 1, 10, 1), CreateStepTrace(0x0A, 1, 1, 20, 2)};
    WriteBin(taskFlip1, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_0");
    ASSERT_EQ(tsTrackParser.Run(dataInventory_, context), Analysis::ANALYSIS_OK);
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    ASSERT_EQ(2ul, data->size());
    for (int i = 0; i < data->size(); i++) {
        ASSERT_EQ(expectTimestamp[i], data->data()[i].hd.timestamp);
        ASSERT_EQ(expectTagId[i], data->data()[i].stepTrace.tagId);
    }
}

TEST_F(TsTrackParserUtest, ShouldReturnTaskMemcpyDataWhenParser)
{
    std::vector<int> expectTimestamp{10, 20};
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = TS_TRACK_PATH;
    std::vector<TaskMemcpy> taskFlip1{CreateTsTaskMemcpy(0x0B, 1, 10, 1), CreateTsTaskMemcpy(0x0B, 1, 20, 1)};
    WriteBin(taskFlip1, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_0");
    ASSERT_EQ(tsTrackParser.Run(dataInventory_, context), Analysis::ANALYSIS_OK);
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    ASSERT_EQ(2ul, data->size());
    for (int i = 0; i < data->size(); i++) {
        ASSERT_EQ(expectTimestamp[i], data->data()[i].hd.timestamp);
    }
}

TEST_F(TsTrackParserUtest, ShouldReturnTaskTypeDataWhenParser)
{
    std::vector<int> expectTimestamp{10, 20};
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = TS_TRACK_PATH;
    std::vector<TaskType> taskFlip1{CreateTsTaskType(0x0C, 1, 1, 10, 1), CreateTsTaskType(0x0C, 1, 1, 20, 2)};
    WriteBin(taskFlip1, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_0");
    ASSERT_EQ(tsTrackParser.Run(dataInventory_, context), Analysis::ANALYSIS_OK);
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    ASSERT_EQ(2ul, data->size());
    for (int i = 0; i < data->size(); i++) {
        ASSERT_EQ(expectTimestamp[i], data->data()[i].hd.timestamp);
    }
}

TEST_F(TsTrackParserUtest, ShouldReturnTaskFlipAndStepTraceDataWhenParser)
{
    std::vector<int> expectTimestamp{10, 20, 100, INVALID_TIMESTAMP};
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = TS_TRACK_PATH;
    std::vector<TaskFlip> taskFlip{CreateTaskFlip(0x0e, 1, 1, 10, 5), CreateTaskFlip(0x0e, 1, 1, 20, 5)};
    WriteBin(taskFlip, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_0");
    std::vector<StepTrace> stepTrace{CreateStepTrace(0x0A, 1, 1, 100, 1),
                                     CreateStepTrace(0x0A, 1, 1, INVALID_TIMESTAMP, 2)};
    WriteBin(stepTrace, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_0");
    ASSERT_EQ(tsTrackParser.Run(dataInventory_, context), Analysis::ANALYSIS_OK);
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    ASSERT_EQ(4ul, data->size());
    for (int i = 0; i < data->size(); i++) {
        ASSERT_EQ(expectTimestamp[i], data->data()[i].hd.timestamp);
    }
}

TEST_F(TsTrackParserUtest, ShouldReturnNoDataWhenPathError)
{
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = "";
    ASSERT_EQ(Analysis::ANALYSIS_OK, tsTrackParser.Run(dataInventory_, context));
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    ASSERT_EQ(0ul, data->size());
}

TEST_F(TsTrackParserUtest, ShouldParseErrorWhenResizeException)
{
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = TS_TRACK_PATH;
    std::vector<StepTrace> taskFlip1{CreateStepTrace(0x0A, 1, 1, 10, 1), CreateStepTrace(0x0A, 1, 1, 20, 2)};
    WriteBin(taskFlip1, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_0");
    MOCKER_CPP(&Resize<HalTrackData>).stubs().will(returnValue(false));
    ASSERT_EQ(Analysis::PARSER_PARSE_DATA_ERROR, tsTrackParser.Run(dataInventory_, context));
    MOCKER_CPP(&Resize<HalTrackData>).reset();
}

TEST_F(TsTrackParserUtest, ShouldNormalRunWhenGetOneFileSizeFail)
{
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = TS_TRACK_PATH;
    std::vector<StepTrace> taskFlip1{CreateStepTrace(0x0A, 1, 1, 10, 1), CreateStepTrace(0x0A, 1, 1, 20, 2)};
    WriteBin(taskFlip1, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_0");
    MOCKER_CPP(&ftell).stubs().will(returnValue(INVALID_FILE_SIZE));
    ASSERT_EQ(Analysis::ANALYSIS_OK, tsTrackParser.Run(dataInventory_, context));
}

TEST_F(TsTrackParserUtest, ShouldReturnBlockNumDataWhenParser)
{
    std::vector<int> expectTimestamp{10, 20};
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = TS_TRACK_PATH;
    std::vector<BlockNum> blockNumData{CreateTsBlockNum(0x0F, 1, 1, 10), CreateTsBlockNum(0x0F, 1, 2, 20)};
    WriteBin(blockNumData, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_0");
    ASSERT_EQ(Analysis::ANALYSIS_OK, tsTrackParser.Run(dataInventory_, context));
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    ASSERT_EQ(2ul, data->size());
    for (int i = 0; i < data->size(); i++) {
        ASSERT_EQ(expectTimestamp[i], data->data()[i].hd.timestamp);
    }
}

TEST_F(TsTrackParserUtest, ShouldAnalyzeErrorWhenFilesSizeMoreThan10GB)
{
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = TS_TRACK_PATH;
    std::vector<StepTrace> taskFlip1{CreateStepTrace(0x0A, 1, 1, 10, 1), CreateStepTrace(0x0A, 1, 1, 20, 2)};
    WriteBin(taskFlip1, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_0");
    MOCKER_CPP(&Parser::GetFilesSize).stubs().will(returnValue(10737418241)); // 超过10GB, 10737418241
    ASSERT_EQ(Analysis::PARSER_READ_DATA_ERROR, tsTrackParser.Run(dataInventory_, context));
}
}
