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
#include "analysis/csrc/application/include/export_manager.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/timeline/chip_trans_v6_assembler.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/chip_trans_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/topo_callback_process.h"
#include "analysis/csrc/infrastructure/process/include/topo_graph.h"
 	 
using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Application;
using namespace Analysis::Domain::Environment;
using EnvContext = Analysis::Domain::Environment::Context;
 	 
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./chip_trans_v6_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE0_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string DEVICE1_PATH = File::PathJoin({PROF_PATH, "device_1"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
const std::string REPORTS_JSON = "reports.json";

bool InjectV6PcieData(Analysis::Infra::DataInventory& dataInventory)
{
    auto pcieV6Data = std::make_shared<std::vector<PcieInfoV6Data>>();
    PcieInfoV6Data data;
    data.deviceId = 0;
    data.dieId = 1;
    data.pcieReadBandwidth = 3;
    data.pcieWriteBandwidth = 4;
    data.timestamp = 1000;
    pcieV6Data->emplace_back(data);
    return dataInventory.Inject(pcieV6Data);
}
}
 	 
class ChipTransV6AssemblerUTest : public testing::Test {
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
 	    dataInventory_.RemoveRestData({});
 	    GlobalMockObject::verify();
 	}
 	 
protected:
 	DataInventory dataInventory_;
};

static std::vector<PcieInfoV6Data> GeneratePcieInfoV6Data()
{
 	std::vector<PcieInfoV6Data> res;
 	PcieInfoV6Data data;
 	data.deviceId = 0;
 	data.dieId = 1;
 	data.pcieReadBandwidth = 3;
 	data.pcieWriteBandwidth = 4;
 	data.timestamp = 1724405892226599429;
 	res.push_back(data);

 	data.deviceId = 1;
 	data.dieId = 2;
 	data.pcieReadBandwidth = 5;
 	data.pcieWriteBandwidth = 6;
 	data.timestamp = 1724405892226699429;
 	res.push_back(data);

 	data.deviceId = 3;
 	data.dieId = 7;
 	data.pcieReadBandwidth = 8;
 	data.pcieWriteBandwidth = 9;
 	data.timestamp = 1724405892226799429;
 	res.push_back(data);
 	return res;
}

TEST_F(ChipTransV6AssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
 	ChipTransV6Assembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(ChipTransV6AssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
 	EXPECT_TRUE(File::CreateDir(DEVICE0_PATH));
 	EXPECT_TRUE(File::CreateDir(DEVICE1_PATH));

 	ChipTransV6Assembler assembler;
 	std::shared_ptr<std::vector<PcieInfoV6Data>> dataS;
 	auto data = GeneratePcieInfoV6Data();
 	MAKE_SHARED_NO_OPERATION(dataS, std::vector<PcieInfoV6Data>, data);
 	dataInventory_.Inject(dataS);

    MOCKER_CPP(&EnvContext::GetPidFromInfoJson).stubs().will(returnValue(2328086));
 	EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

 	auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
 	ASSERT_EQ(1ul, files.size());
 	FileReader reader(files.back());
 	std::vector<std::string> res;
 	EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
 	ASSERT_FALSE(res.empty());

 	const std::string& content = res.back();
 	EXPECT_NE(std::string::npos, content.find("\"args\":{\"name\":\"Stars Chip Trans\"}"));
 	EXPECT_NE(std::string::npos, content.find("\"args\":{\"labels\":\"NPU 0\"}"));
 	EXPECT_NE(std::string::npos, content.find("\"args\":{\"labels\":\"NPU 1\"}"));
 	EXPECT_NE(std::string::npos, content.find("\"name\":\"PCIE Read Bandwidth\""));
 	EXPECT_NE(std::string::npos, content.find("\"name\":\"PCIE Write Bandwidth\""));
 	EXPECT_NE(std::string::npos, content.find("\"U-Die1\":3"));
 	EXPECT_NE(std::string::npos, content.find("\"U-Die1\":4"));
 	EXPECT_NE(std::string::npos, content.find("\"U-Die2\":5"));
 	EXPECT_NE(std::string::npos, content.find("\"U-Die2\":6"));
	EXPECT_EQ(std::string::npos, content.find("\"U-Die7\":8"));
}

TEST_F(ChipTransV6AssemblerUTest, ShouldExportTimelineFromV6PcieData)
{
    ASSERT_TRUE(File::CreateDir(DEVICE0_PATH));
    {
        FileWriter reportsWriter(File::PathJoin({BASE_PATH, REPORTS_JSON}));
        reportsWriter.WriteText(R"({"json_process":{"stars_chip_trans":true}})");
    }
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::IsChipV6).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetPidFromInfoJson).stubs().will(returnValue(2328086));

    const TopoNodeId chipTransNode{TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_CHIP_TRAINS};
    const TopoNodeId pcieNode{TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_PCIE};
    auto& definitions = TopoNodeRegistry::MutableDefinitions();
    const TopoNodeDefinition chipTransDefinition = definitions.at(chipTransNode);
    const TopoNodeDefinition pcieDefinition = definitions.at(pcieNode);
    definitions.at(chipTransNode).creatorFactory = [](const TopoBuildContext&) {
        return []() -> std::unique_ptr<Analysis::Infra::Process> {
            return std::unique_ptr<Analysis::Infra::Process>(
                new TopoCallbackProcess([](Analysis::Infra::DataInventory& dataInventory) {
                    return InjectV6PcieData(dataInventory);
                }));
        };
    };
    definitions.at(pcieNode).creatorFactory = [](const TopoBuildContext&) {
        return []() -> std::unique_ptr<Analysis::Infra::Process> {
            return std::unique_ptr<Analysis::Infra::Process>(
                new TopoCallbackProcess([](Analysis::Infra::DataInventory&) { return true; }));
        };
    };

    const bool exportSuccess = manager.Run({ExportMode::TIMELINE});
    definitions.at(chipTransNode) = chipTransDefinition;
    definitions.at(pcieNode) = pcieDefinition;
    ASSERT_TRUE(exportSuccess);

    const std::vector<std::string> files = File::GetOriginData(RESULT_PATH, {MSPROF_JSON_FILE}, {});
    ASSERT_EQ(1UL, files.size());
    FileReader reader(files.front());
    std::vector<std::string> contents;
    ASSERT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(contents));
    ASSERT_FALSE(contents.empty());
    EXPECT_NE(std::string::npos, contents.back().find("\"U-Die1\":3"));
    EXPECT_NE(std::string::npos, contents.back().find("\"U-Die1\":4"));
}
 	 
TEST_F(ChipTransV6AssemblerUTest, ShouldReturnFalseWhenNoDeviceDirectoryExists)
{
 	ChipTransV6Assembler assembler;
 	std::shared_ptr<std::vector<PcieInfoV6Data>> dataS;
 	auto data = GeneratePcieInfoV6Data();
 	MAKE_SHARED_NO_OPERATION(dataS, std::vector<PcieInfoV6Data>, data);
 	dataInventory_.Inject(dataS);

 	EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
 	auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
 	EXPECT_TRUE(files.empty());
}
