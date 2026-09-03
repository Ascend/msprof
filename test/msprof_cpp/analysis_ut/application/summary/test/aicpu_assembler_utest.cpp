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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/summary/aicpu_assembler.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/application/summary/summary_factory.h"
#include "analysis/csrc/application/summary/summary_manager.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/aicpu_summary_data.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Utils;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./aicpu_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});

const std::string AICPU_HEADER =
    "Timestamp(us),Node,Compute_time(us),Memcpy_time(us),Task_time(us),Dispatch_time(us),Total_time(us),Stream ID,Task "
    "ID";
const std::string DP_HEADER = "Timestamp(us),Action,Source,Cached Buffer Size";
const std::string MI_HEADER = "Node Name,Start Time(us),End Time(us),Queue Size";

std::string FindCsvByName(const std::string &name)
{
    const std::vector<std::string> files = File::GetOriginData(RESULT_PATH, {name}, {});
    for (size_t i = 0; i < files.size(); ++i) {
        const std::string base = File::BaseName(files[i]);
        if (name == AICPU_NAME && base.find(AICPU_MI_NAME) == 0) {
            continue;
        }
        if (base.find(name) == 0) {
            return files[i];
        }
    }
    return "";
}

std::vector<std::string> ReadCsvLines(const std::string &filePath)
{
    FileReader reader(filePath);
    std::vector<std::string> lines;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(lines));
    return lines;
}

template <typename T>
void InjectVector(DataInventory &dataInventory, const std::vector<T> &data)
{
    std::shared_ptr<std::vector<T>> holder;
    MAKE_SHARED_NO_OPERATION(holder, std::vector<T>, data);
    dataInventory.Inject(holder);
}

std::vector<AicpuSummaryData> GenerateAicpuData()
{
    std::vector<AicpuSummaryData> res;
    AicpuSummaryData first;
    first.deviceId = 0;
    first.timestampNs = 1000000;
    first.nodeName = "Conv2D";
    first.computeTimeUs = 1.5;
    first.memcpyTimeUs = 2.5;
    first.taskTimeUs = 500.0;
    first.dispatchTimeUs = 0.5;
    first.totalTimeUs = 10.5;
    first.streamId = 10;
    first.taskId = 20;
    res.push_back(first);

    AicpuSummaryData second;
    second.deviceId = 0;
    second.timestampNs = 2000000;
    second.nodeName = "N/A";
    second.computeTimeUs = 3.0;
    second.memcpyTimeUs = 4.0;
    second.taskTimeUs = 8.0;
    second.dispatchTimeUs = 1.0;
    second.totalTimeUs = 20.0;
    second.streamId = 11;
    second.taskId = 30;
    res.push_back(second);
    return res;
}

std::vector<AicpuDpData> GenerateDpData()
{
    std::vector<AicpuDpData> res;
    AicpuDpData first;
    first.timestamp = 1000000;
    first.action = "enqueue";
    first.source = "src0";
    first.bufferSize = 128;
    res.push_back(first);

    AicpuDpData second;
    second.timestamp = 2500000;
    second.action = "dequeue";
    second.source = "src1";
    second.bufferSize = 256;
    res.push_back(second);
    return res;
}

std::vector<AicpuMiData> GenerateMiData()
{
    std::vector<AicpuMiData> res;
    AicpuMiData first;
    first.nodeName = "QueueA";
    first.startTime = 100;
    first.endTime = 200;
    first.queueSize = 8;
    res.push_back(first);

    AicpuMiData second;
    second.nodeName = "QueueB";
    second.startTime = 300;
    second.endTime = 400;
    second.queueSize = 16;
    res.push_back(second);
    return res;
}
}  // namespace

class AicpuAssemblerUTest : public testing::Test {
protected:
    void SetUp() override
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }

    void TearDown() override
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        GlobalMockObject::verify();
    }
};

TEST_F(AicpuAssemblerUTest, ShouldReturnTrueWhenDataNotExist)
{
    AicpuAssembler assembler(PROCESSOR_NAME_AICPU, PROF_PATH);
    DataInventory dataInventory;
    EXPECT_TRUE(assembler.Run(dataInventory));
    EXPECT_TRUE(FindCsvByName(AICPU_NAME).empty());
    EXPECT_TRUE(FindCsvByName(AICPU_DP_NAME).empty());
    EXPECT_TRUE(FindCsvByName(AICPU_MI_NAME).empty());
}

TEST_F(AicpuAssemblerUTest, ShouldReturnTrueWhenEmptyVectorsInjected)
{
    DataInventory dataInventory;
    InjectVector(dataInventory, std::vector<AicpuSummaryData>());
    InjectVector(dataInventory, std::vector<AicpuDpData>());
    InjectVector(dataInventory, std::vector<AicpuMiData>());

    AicpuAssembler assembler(PROCESSOR_NAME_AICPU, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    EXPECT_TRUE(FindCsvByName(AICPU_NAME).empty());
    EXPECT_TRUE(FindCsvByName(AICPU_DP_NAME).empty());
    EXPECT_TRUE(FindCsvByName(AICPU_MI_NAME).empty());
}

TEST_F(AicpuAssemblerUTest, ShouldWriteThreeCsvWhenAllDataExist)
{
    DataInventory dataInventory;
    InjectVector(dataInventory, GenerateAicpuData());
    InjectVector(dataInventory, GenerateDpData());
    InjectVector(dataInventory, GenerateMiData());

    AicpuAssembler assembler(PROCESSOR_NAME_AICPU, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));

    const std::string aicpuFile = FindCsvByName(AICPU_NAME);
    const std::string dpFile = FindCsvByName(AICPU_DP_NAME);
    const std::string miFile = FindCsvByName(AICPU_MI_NAME);
    ASSERT_FALSE(aicpuFile.empty());
    ASSERT_FALSE(dpFile.empty());
    ASSERT_FALSE(miFile.empty());

    std::vector<std::string> aicpuLines = ReadCsvLines(aicpuFile);
    ASSERT_EQ(3ul, aicpuLines.size());
    EXPECT_EQ(AICPU_HEADER, aicpuLines[0]);
    EXPECT_EQ("1000.000,Conv2D,1.5,2.5,500,0.5,10.5,10,20", aicpuLines[1]);
    EXPECT_EQ("2000.000,N/A,3,4,8,1,20,11,30", aicpuLines[2]);

    std::vector<std::string> dpLines = ReadCsvLines(dpFile);
    ASSERT_EQ(3ul, dpLines.size());
    EXPECT_EQ(DP_HEADER, dpLines[0]);
    EXPECT_EQ("1000.000,enqueue,src0,128", dpLines[1]);
    EXPECT_EQ("2500.000,dequeue,src1,256", dpLines[2]);

    std::vector<std::string> miLines = ReadCsvLines(miFile);
    ASSERT_EQ(3ul, miLines.size());
    EXPECT_EQ(MI_HEADER, miLines[0]);
    EXPECT_EQ("QueueA,100,200,8", miLines[1]);
    EXPECT_EQ("QueueB,300,400,16", miLines[2]);
}

TEST_F(AicpuAssemblerUTest, ShouldWriteAicpuOnly)
{
    DataInventory dataInventory;
    InjectVector(dataInventory, GenerateAicpuData());

    AicpuAssembler assembler(PROCESSOR_NAME_AICPU, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    EXPECT_FALSE(FindCsvByName(AICPU_NAME).empty());
    EXPECT_TRUE(FindCsvByName(AICPU_DP_NAME).empty());
    EXPECT_TRUE(FindCsvByName(AICPU_MI_NAME).empty());
}

TEST_F(AicpuAssemblerUTest, ShouldWriteDpOnly)
{
    DataInventory dataInventory;
    InjectVector(dataInventory, GenerateDpData());

    AicpuAssembler assembler(PROCESSOR_NAME_AICPU, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    EXPECT_TRUE(FindCsvByName(AICPU_NAME).empty());
    EXPECT_FALSE(FindCsvByName(AICPU_DP_NAME).empty());
    EXPECT_TRUE(FindCsvByName(AICPU_MI_NAME).empty());
}

TEST_F(AicpuAssemblerUTest, ShouldWriteMiOnly)
{
    DataInventory dataInventory;
    InjectVector(dataInventory, GenerateMiData());

    AicpuAssembler assembler(PROCESSOR_NAME_AICPU, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    EXPECT_TRUE(FindCsvByName(AICPU_NAME).empty());
    EXPECT_TRUE(FindCsvByName(AICPU_DP_NAME).empty());
    EXPECT_FALSE(FindCsvByName(AICPU_MI_NAME).empty());
}

TEST_F(AicpuAssemblerUTest, ShouldGetAicpuAssemblerFromFactory)
{
    auto assembler = SummaryFactory::GetAssemblerByName(PROCESSOR_NAME_AICPU, PROF_PATH);
    EXPECT_NE(nullptr, assembler);
}

TEST_F(AicpuAssemblerUTest, ShouldMapThreeDeliverablesToOneAssembler)
{
    std::vector<std::string> assemblers;
    ASSERT_TRUE(SummaryManager::GetAssemblerList({"aicpu", "dp", "aicpu_mi", "aicpu"}, assemblers));
    ASSERT_EQ(1ul, assemblers.size());
    EXPECT_EQ(PROCESSOR_NAME_AICPU, assemblers[0]);
}
