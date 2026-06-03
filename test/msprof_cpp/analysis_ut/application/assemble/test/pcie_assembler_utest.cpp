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
#include "analysis/csrc/application/timeline/pcie_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/pcie_data.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain::Environment;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./pcie_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class PCIeAssemblerUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        dataInventory_.RemoveRestData({});
        GlobalMockObject::verify();
    }
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
protected:
    static DataInventory dataInventory_;
};
DataInventory PCIeAssemblerUTest::dataInventory_;

static std::vector<PCIeData> GeneratePCIeData()
{
    std::vector<PCIeData> res;
    PCIeData data;
    data.deviceId = 0; // device 0
    data.timestamp = 1719621074669030430; // timestamp 1719621074669030430
    data.txPost.avg = 62 * MICRO_SECOND; // txPost.avg 62 * MICRO_SECOND
    data.rxPost.avg = 0 * MICRO_SECOND; // rxPost.avg 0 * MICRO_SECOND
    data.txNonpost.avg = 0 * MICRO_SECOND; // txNonpost.avg 0 * MICRO_SECOND
    data.rxNonpost.avg = 0 * MICRO_SECOND; // rxNonpost.avg 0 * MICRO_SECOND
    data.txCpl.avg = 0 * MICRO_SECOND; // txCpl.avg 0 * MICRO_SECOND
    data.rxCpl.avg = 0 * MICRO_SECOND; // rxCpl.avg 0 * MICRO_SECOND
    data.txNonpostLatency.avg = 257; // txNonpostLatency.avg 257
    res.push_back(data);

    data.deviceId = 0; // device 0
    data.timestamp = 1719621074688865380; // timestamp 1719621074688865380
    data.txPost.avg = 77 * MICRO_SECOND; // txPost.avg 77 * MICRO_SECOND
    data.rxPost.avg = 0 * MICRO_SECOND; // rxPost.avg 0 * MICRO_SECOND
    data.txNonpost.avg = 0 * MICRO_SECOND; // txNonpost.avg 0 * MICRO_SECOND
    data.rxNonpost.avg = 0 * MICRO_SECOND; // rxNonpost.avg 0 * MICRO_SECOND
    data.txCpl.avg = 0 * MICRO_SECOND; // txCpl.avg 0 * MICRO_SECOND
    data.rxCpl.avg = 0 * MICRO_SECOND; // rxCpl.avg 0 * MICRO_SECOND
    data.txNonpostLatency.avg = 307; // txNonpostLatency.avg 307
    res.push_back(data);
    return res;
}

TEST_F(PCIeAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    PCIeAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(PCIeAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    PCIeAssembler assembler;
    std::shared_ptr<std::vector<PCIeData>> dataS;
    auto data = GeneratePCIeData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<PCIeData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"PCIe_cpl\",\"pid\":2383960800,\"tid\":0,\"ts\":\"1719621074669030.430\",\""
                            "ph\":\"C\",\"args\":{\"Rx\":0.0,\"Tx\":0.0}},{\"name\":\"PCIe_nonpost\",\"pid\":"
                            "2383960800,\"tid\":0,\"ts\":\"1719621074669030.430\",\"ph\":\"C\",\"args\":{\"Rx\":0.0"
                            ",\"Tx\":0.0}},{\"name\":\"PCIe_nonpost_latency\",\"pid\":2383960800,\"tid\":0,\"ts\":\""
                            "1719621074669030.430\",\"ph\":\"C\",\"args\":{\"Rx\":0.0,\"Tx\":0.257}},{\"name\":\""
                            "PCIe_post\",\"pid\":2383960800,\"tid\":0,\"ts\":\"1719621074669030.430\",\"ph\":\"C\","
                            "\"args\":{\"Rx\":0.0,\"Tx\":59.1278076171875}},{\"name\":\"PCIe_cpl\",\"pid\":2383960800,"
                            "\"tid\":0,\"ts\":\"1719621074688865.380\",\"ph\":\"C\",\"args\":{\"Rx\":0.0,\"Tx\":0.0"
                            "}},{\"name\":\"PCIe_nonpost\",\"pid\":2383960800,\"tid\":0,\"ts\":\""
                            "1719621074688865.380\",\"ph\":\"C\",\"args\":{\"Rx\":0.0,\"Tx\":0.0}},{\"name\":\""
                            "PCIe_nonpost_latency\",\"pid\":2383960800,\"tid\":0,\"ts\":\"1719621074688865.380\",\""
                            "ph\":\"C\",\"args\":{\"Rx\":0.0,\"Tx\":0.307}},{\"name\":\"PCIe_post\",\"pid\":2383960800"
                            ",\"tid\":0,\"ts\":\"1719621074688865.380\",\"ph\":\"C\",\"args\":{\"Rx\":0.0,\"Tx\":"
                            "73.43292236328125}},{\"name\":\"process_name\",\"pid\":2383960800,\"tid\":0,\"ph\":\"M\","
                            "\"args\":{\"name\":\"PCIe\"}},{\"name\":\"process_labels\",\"pid\":2383960800,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":"
                            "2383960800,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":23}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(PCIeAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    PCIeAssembler assembler;
    std::shared_ptr<std::vector<PCIeData>> dataS;
    auto data = GeneratePCIeData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<PCIeData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
