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
#include <iostream>
#include <vector>
#include <unordered_map>
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/domain/services/device_context/load_host_data.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/entities/hal/include/ascend_obj.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"

namespace Analysis {
using namespace Analysis;
using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;

namespace {
using DeviceTaskSummary = std::map<TaskId, std::vector<DeviceTask>>;
using runTimeData = std::vector<
        std::tuple<
            uint32_t,
            int32_t,
            uint32_t,
            uint32_t,
            uint32_t,
            uint32_t,
            std::string,
            std::string,
            uint32_t,
            uint32_t,
            uint32_t,
            uint32_t>>;
using OriDataFormat = std::vector<
        std::tuple<
            uint32_t,
            std::string,
            uint32_t,
            uint32_t,
            uint32_t,
            uint32_t,
            uint32_t,
            std::string,
            std::string,
            uint32_t,
            uint32_t,
            uint32_t,
            uint32_t,
            uint32_t,
            std::string,
            std::string,
            std::string,
            std::string,
            std::string,
            std::string,
            uint32_t,
            uint32_t,
            std::string,
            std::string,
            std::string,
            std::string>>;
using HcclOpOriDataFormat = std::vector<
        std::tuple<
            uint16_t,
            uint64_t,
            int32_t,
            uint32_t,
            std::string,
            std::string,
            std::string,
            uint64_t,
            uint64_t,
            std::string,
            int64_t,
            int64_t,
            int32_t,
            int32_t,
            std::string,
            std::string,
            uint64_t,
            std::string
        >>;
using HcclTaskOriDataFormat = std::vector<
        std::tuple<
            uint32_t,
            uint32_t,
            std::string,
            std::string,
            uint32_t,
            uint64_t,
            double,
            uint32_t,
            uint32_t,
            uint32_t,
            uint32_t,
            uint32_t,
            uint32_t,
            uint32_t,
            uint32_t,
            std::string,
            double,
            std::string,
            std::string,
            std::string,
            std::string,
            uint32_t,
            uint32_t
        >>;

const uint64_t START_TIME = 100;
const uint64_t END_TIME = 200;
const std::string HOST_PATH = "./host";
const std::string GEDB_PATH = File::PathJoin({HOST_PATH, "sqlite", "ge_info.db"});
const std::string RUNDB_PATH = File::PathJoin({HOST_PATH, "sqlite", "runtime.db"});
const std::string HCCLDB_PATH = File::PathJoin({HOST_PATH, "sqlite", "hccl.db"});
const OriDataFormat DATA_A{
    {0, "Default/network/network/bert/bert/bert_embedding_postprocessor/StridedSliceD-op6673",
    1, 1, 32, 0, 0, "AI_CORE", "StridedSliceD", 0, 120040, 458597374830, 1, 2, "DEFAULT_",
    "INT32_", "1,512", "DEFAULT_", "INT32_", "1,512", 0, 1, "1", "123", "N/A", "N/A"},
    {0, "Default/network/network/bert/bert/bert_embedding_postprocessor/StridedSliceD-op6673",
     1, 2, 32, 0, 0, "AI_CORE", "StridedSliceD", 0, 120040, 458597374830, 1, 2, "DEFAULT_",
     "INT32_", "1,512", "DEFAULT_", "INT32_", "1,512", 0, 1, "1", "123", "N/A", "N/A"}};
const runTimeData DATA_B{{4294967295, -1, 65535, 0, 4294967295, 0, "PROFILING_ENABLE", "aclnn", 0, 397936887714, -1, 1},
                   {4294967295, -1, 65535, 0, 4294967295, 0, "PROFILING_ENABLE", "aclnn", 0, 397936887714, -1, 1}};
const HcclOpOriDataFormat DATA_HCCL_OP{{0, 3, 0, 121639, "Default/network/AllReduce-op0", "HCCL", "HcomAllReduce",
                                        9897897351791, 9897916800504, "", 33558, -1, -1, -1, "", "", 1, ""}};
const HcclTaskOriDataFormat DATA_HCCL_TASK{{3, 0, "Memcpy", "7415574198778220483", 3, 9897976501291, 307.45316062176164,
                                            29, 386, 4294967295, 1, 0, 0, 0, 4, "SDMA", 5904896, "INVALID_TYPE", "PCIE",
                                            "18446744073709551615", "INVALID_TYPE", 123, 64}};
}

class LoadHostDataUtest : public testing::Test {
protected:
    void SetUp() override
    {
        auto ptr = std::make_shared<std::map<TaskId, std::vector<DeviceTask>>>();
        dataInventory_.Inject(ptr);
        EXPECT_TRUE(File::CreateDir(HOST_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({HOST_PATH, "sqlite"})));
    }

    void CreateHostData()
    {
        GEInfoDB geInfoDb;
        std::string geInfoDbName = "TaskInfo";
        CreateDB(geInfoDb, geInfoDbName, GEDB_PATH, DATA_A);

        RuntimeDB runtimeDb;
        std::string runtimeDbName = "HostTask";
        CreateDB(runtimeDb, runtimeDbName, RUNDB_PATH, DATA_B);

        HCCLDB hcclDb;
        std::string hcclDbName = "HCCLTask";
        CreateDB(hcclDb, hcclDbName, HCCLDB_PATH, DATA_HCCL_TASK);

        std::string hcclOpName = "HCCLOP";
        CreateDB(hcclDb, hcclOpName, HCCLDB_PATH, DATA_HCCL_OP);
    }

    void TearDown() override
    {
        File::RemoveDir(HOST_PATH, 0);
    }

    template<typename T>
    void CreateDB(Database db, const std::string &tableName, const std::string& dpPath, T data)
    {
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dpPath);
        auto cols = db.GetTableCols(tableName);
        EXPECT_TRUE(dbRunner->CreateTable(tableName, cols));
        EXPECT_TRUE(dbRunner->InsertData(tableName, data));
    }

    void DropTable(const std::string& dpPath, const std::string &tableName) const
    {
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dpPath);
        EXPECT_TRUE(dbRunner->DropTable(tableName));
    }

    static std::map<TaskId, std::vector<DeviceTask>> GenerateDeviceTask()
    {
        std::map<TaskId, std::vector<DeviceTask>> deviceTask;
        auto& res1 = deviceTask[{1, 1, 1, 1}];
        res1.emplace_back();
        res1.back().taskStart = START_TIME;
        auto& res2 = deviceTask[{1, 1, 2, 1}];
        res2.emplace_back();
        res2.back().taskStart = END_TIME;
        return deviceTask;
    }
protected:
    DataInventory dataInventory_;
};

TEST_F(LoadHostDataUtest, InjectDataReturnSuccess)
{
    auto ptr = std::make_shared<std::map<TaskId, std::vector<DeviceTask>>>();
    dataInventory_.Inject(ptr);
    CreateHostData();
    std::vector<uint32_t> expectBlockNum{32, 32};
    auto deviceTask = GenerateDeviceTask();
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceFilePath = HOST_PATH;
    LoadHostData loadHostData;
    ASSERT_EQ(ANALYSIS_OK, loadHostData.Run(dataInventory_, deviceContext));
    ASSERT_EQ(deviceTaskS->at({1, 1, 1, 1})[0].blockNum, expectBlockNum[0]);
    ASSERT_EQ(deviceTaskS->at({1, 1, 2, 1})[0].blockNum, expectBlockNum[1]);
}

TEST_F(LoadHostDataUtest, ReturnOKWhenWithOutDeviceTask)
{
    CreateHostData();
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceFilePath = HOST_PATH;
    LoadHostData loadHostData;
    ASSERT_EQ(ANALYSIS_OK, loadHostData.Run(dataInventory_, deviceContext));
}

TEST_F(LoadHostDataUtest, ReturnOKWhenTaskInfo)
{
    GEInfoDB geInfoDb;
    std::string geInfoDbName = "TaskInfo";
    CreateDB(geInfoDb, geInfoDbName, GEDB_PATH, DATA_A);
    auto ptr = std::make_shared<std::map<TaskId, std::vector<DeviceTask>>>();
    dataInventory_.Inject(ptr);

    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceFilePath = HOST_PATH;
    LoadHostData loadHostData;

    ASSERT_EQ(ANALYSIS_OK, loadHostData.Run(dataInventory_, deviceContext));
}

TEST_F(LoadHostDataUtest, ReturnOKWhenHcclOpAndHcclTask)
{
    HCCLDB hcclDb;
    std::string hcclOpDbName = "HCCLOP";
    std::string hcclTaskDbName = "HCCLTask";
    CreateDB(hcclDb, hcclOpDbName, RUNDB_PATH, DATA_HCCL_OP);
    CreateDB(hcclDb, hcclTaskDbName, RUNDB_PATH, DATA_HCCL_TASK);
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceFilePath = HOST_PATH;
    LoadHostData loadHostData;

    ASSERT_EQ(ANALYSIS_OK, loadHostData.Run(dataInventory_, deviceContext));
}

TEST_F(LoadHostDataUtest, ReturnOKWhenWithoutHostTask)
{
    RuntimeDB runtimeDb;
    std::string runtimeDbName = "HostTask";
    CreateDB(runtimeDb, runtimeDbName, RUNDB_PATH, DATA_B);
    auto ptr = std::make_shared<std::map<TaskId, std::vector<DeviceTask>>>();
    dataInventory_.Inject(ptr);

    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceFilePath = HOST_PATH;
    LoadHostData loadHostData;

    ASSERT_EQ(ANALYSIS_OK, loadHostData.Run(dataInventory_, deviceContext));
}

TEST_F(LoadHostDataUtest, ReturnOKWhenNoTable)
{
    RuntimeDB runtimeDb;
    std::string runtimeDbName = "HostTask";
    CreateDB(runtimeDb, runtimeDbName, RUNDB_PATH, DATA_B);
    DropTable(RUNDB_PATH, runtimeDbName);
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceFilePath = HOST_PATH;
    LoadHostData loadHostData;

    ASSERT_EQ(ANALYSIS_OK, loadHostData.Run(dataInventory_, deviceContext));
}

TEST_F(LoadHostDataUtest, TestProcessEntryReadHostRuntimeSuccess)
{
    CreateHostData();
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceInfo.chipID = 15;
    deviceContext.deviceContextInfo.deviceFilePath = HOST_PATH;
    LoadHostData loadHostData;
    loadHostData.ProcessEntry(dataInventory_, deviceContext);
    auto streamInfo_ = dataInventory_.GetPtr<StreamIdInfo>();
    ASSERT_EQ(1u, streamInfo_->streamIdMap.size());
}
}