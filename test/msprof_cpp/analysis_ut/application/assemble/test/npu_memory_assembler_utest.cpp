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
#include <iostream>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/summary/npu_memory_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/summary/summary_constant.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Application;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./npu_mem_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
}

class NpuMemoryAssemblerUTest : public testing::Test {
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        GlobalMockObject::verify();
    }
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }
};

static std::vector<NpuMemData> GenerateTaskData()
{
    std::vector<NpuMemData> res;
    NpuMemData data;
    data.deviceId = 0; // deviceId 0
    data.event = "0"; // event 0
    data.ddr = 0; // ddr 0
    data.hbm = 19061620736; // hbm 19061620736
    data.memory = 19061620736; // memory 19061620736
    data.timestamp = 1730343450651306550; // timestamp 1730343450651306550
    res.push_back(data); // 有taskInfo

    data.deviceId = 0; // deviceId 0
    data.event = "0"; // event 0
    data.ddr = 0; // ddr 0
    data.hbm = 19061620736; // hbm 19061620736
    data.memory = 19061620736; // memory 19061620736
    data.timestamp = 1730343450651306550; // timestamp 1730343450651306550
    res.push_back(data); // 有taskInfo
    return res;
}

/**
 * test successful
 */
TEST_F(NpuMemoryAssemblerUTest, ShouldReturnTrueSuccessful)
{
    NpuMemoryAssembler assembler(PROCESSOR_NAME_NPU_MEM, PROF_PATH);
    DataInventory dataInventory;
    std::vector<NpuMemData> data = GenerateTaskData();
    std::shared_ptr<std::vector<NpuMemData>> dataS;
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<NpuMemData>, data);
    dataInventory.Inject(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {NPU_MEMORY_NAME}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(0ul, reader.ReadText(res));
    EXPECT_EQ(3ul, res.size());
    std::string expectHeader{"Device_id,event,ddr(KB),hbm(KB),memory(KB),timestamp(us)"};
    EXPECT_EQ(expectHeader, res[0]);
}

/**
 * test no data
 */
TEST_F(NpuMemoryAssemblerUTest, ShouldReturnTrueWhenDataNotExist)
{
    NpuMemoryAssembler assembler(PROCESSOR_NAME_NPU_MEM, PROF_PATH);
    DataInventory dataInventory;
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {NPU_MEMORY_NAME}, {});
    EXPECT_EQ(0ul, files.size());
}

/**
 * test empty data
 */
TEST_F(NpuMemoryAssemblerUTest, ShouldReturnFalseWhenDataEmpty)
{
    NpuMemoryAssembler assembler(PROCESSOR_NAME_NPU_MEM, PROF_PATH);
    DataInventory dataInventory;
    std::vector<NpuMemData> emptyData;
    std::shared_ptr<std::vector<NpuMemData>> dataS;
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<NpuMemData>, emptyData);
    dataInventory.Inject(dataS);
    EXPECT_FALSE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {NPU_MEMORY_NAME}, {});
    EXPECT_EQ(0ul, files.size());
}