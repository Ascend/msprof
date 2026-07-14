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

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/application/summary/page_fault_assembler.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/application/summary/summary_factory.h"
#include "analysis/csrc/application/summary/summary_manager.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/page_fault_data.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Utils;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./page_fault_assembler_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
}

class PageFaultAssemblerUTest : public testing::Test {
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

static std::vector<PageFaultData> GeneratePageFaultData()
{
    std::vector<PageFaultData> res;
    PageFaultData data;
    data.deviceId = 0;
    data.streamId = 15;
    data.taskId = 31;
    data.pageFaultNum = 3;
    data.opName = "Conv2D";
    res.emplace_back(data);
    data.deviceId = 0;
    data.streamId = 17;
    data.taskId = 63;
    data.pageFaultNum = 1;
    data.opName = "Add";
    res.emplace_back(data);
    return res;
}

TEST_F(PageFaultAssemblerUTest, ShouldReturnTrueWhenDataNotExist)
{
    PageFaultAssembler assembler(PROCESSOR_NAME_PAGE_FAULT, PROF_PATH);
    DataInventory dataInventory;
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {PAGE_FAULT_NAME}, {});
    EXPECT_EQ(0ul, files.size());
}

TEST_F(PageFaultAssemblerUTest, ShouldReturnTrueWhenPageFaultAssembleSuccess)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<PageFaultData>> pageFaultDataS;
    auto pageFaultData = GeneratePageFaultData();
    MAKE_SHARED_NO_OPERATION(pageFaultDataS, std::vector<PageFaultData>, pageFaultData);
    dataInventory.Inject(pageFaultDataS);

    PageFaultAssembler assembler(PROCESSOR_NAME_PAGE_FAULT, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));

    auto files = File::GetOriginData(RESULT_PATH, {PAGE_FAULT_NAME}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(3ul, res.size());
    EXPECT_EQ("Device Id,Stream Id,Task Id,Page Fault Num,Op Name", res[0]);
    EXPECT_EQ("0,15,31,3,Conv2D", res[1]);
    EXPECT_EQ("0,17,63,1,Add", res[2]);
}

TEST_F(PageFaultAssemblerUTest, ShouldReturnFalseWhenPageFaultDataEmpty)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<PageFaultData>> pageFaultDataS;
    std::vector<PageFaultData> pageFaultData;
    MAKE_SHARED_NO_OPERATION(pageFaultDataS, std::vector<PageFaultData>, pageFaultData);
    dataInventory.Inject(pageFaultDataS);

    PageFaultAssembler assembler(PROCESSOR_NAME_PAGE_FAULT, PROF_PATH);
    EXPECT_FALSE(assembler.Run(dataInventory));
}

TEST_F(PageFaultAssemblerUTest, ShouldGetAssemblerAndProcessListForPageFault)
{
    auto assembler = SummaryFactory::GetAssemblerByName(PROCESSOR_NAME_PAGE_FAULT, PROF_PATH);
    EXPECT_NE(nullptr, assembler);

    const auto& processList = SummaryManager::GetProcessList();
    EXPECT_TRUE(processList.find(PROCESSOR_NAME_PAGE_FAULT) != processList.end());
}
