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
#include "analysis/csrc/domain/data_process/system/npu_module_mem_processor.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;
namespace {
const int DEPTH = 0;
const std::string NPU_MODULE_MEM_PATH = "./npu_module_mem_path";
const std::string DB_PATH = File::PathJoin({NPU_MODULE_MEM_PATH, "msprof.db"});
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "npu_module_mem.db";
const std::string PROF_PATH = File::PathJoin({NPU_MODULE_MEM_PATH,
                                                 "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string TABLE_NAME = "NpuModuleMem";

using OriDataFormat = std::vector<std::tuple<uint32_t, double, uint64_t, std::string>>;

using ProcessedDataFormat = std::vector<std::tuple<uint32_t, uint64_t, uint64_t, uint16_t>>;

const OriDataFormat DATA{{0, 48673240, 0,        "NPU:1"},
                         {7, 48673240, 20025344, "NPU:1"}};

}


class NpuModuleMemProcessorUTest : public testing::Test {
protected:
    void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(NPU_MODULE_MEM_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, DEVICE_SUFFIX, SQLITE})));
        CreateNpuModuleMem(File::PathJoin({PROF_PATH, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA);
    }
    void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(NPU_MODULE_MEM_PATH, DEPTH));
    }
    static void CreateNpuModuleMem(const std::string& dbPath, OriDataFormat data)
    {
        std::shared_ptr<NpuModuleMemDB> database;
        MAKE_SHARED0_RETURN_VOID(database, NpuModuleMemDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(NpuModuleMemProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetSyscntConversionParams)
        .stubs()
        .will(returnValue(true));
    auto processor = NpuModuleMemProcessor(PROF_PATH);

    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MODULE_MEM));

    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetSyscntConversionParams).reset();
}

TEST_F(NpuModuleMemProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = NpuModuleMemProcessor(PROF_PATH);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MODULE_MEM));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    // Mock ProfTimeRecord and SyscntConversionParams
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    // Construct DBRunner Failed
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MODULE_MEM));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    // CheckPathAndTable Failed
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).stubs().will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MODULE_MEM));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();
    // CheckPathAndTable not exist
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).stubs().will(returnValue(NOT_EXIST));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MODULE_MEM));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetSyscntConversionParams).reset();
    // GetSyscntConversionParams Failed
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetSyscntConversionParams).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MODULE_MEM));
    // Reset Mock ProfTimeRecord and SyscntConversionParams
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetSyscntConversionParams).reset();
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<NpuModuleMemData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MODULE_MEM));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<NpuModuleMemData>).reset();
}

TEST_F(NpuModuleMemProcessorUTest, TestRunShouldReturnFalseWhenFormatDataFail)
{
    auto processor = NpuModuleMemProcessor(PROF_PATH);
    DataInventory dataInventory;
    Analysis::Test::StubReserveFailureForVector<std::vector<NpuModuleMemData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MODULE_MEM));
    Analysis::Test::ResetReserveFailureForVector<std::vector<NpuModuleMemData>>();
}

TEST_F(NpuModuleMemProcessorUTest, TestRunShouldReturnFalseWhenReserveException)
{
    auto processor = NpuModuleMemProcessor(PROF_PATH);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    Analysis::Test::StubReserveFailureForVector<std::vector<NpuModuleMemData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_ACC_PMU));
    Analysis::Test::ResetReserveFailureForVector<std::vector<NpuModuleMemData>>();
}
