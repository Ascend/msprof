/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */
#include "analysis/csrc/domain/services/device_context/device_context.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "nlohmann/json.hpp"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;

const auto DEVICE_DIR = "./device_0";
const std::string INFO_JSON = "info.json";


class DeviceInfoUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(DEVICE_DIR));
        CreateInfoJson(DEVICE_DIR, 0);
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(DEVICE_DIR, 0));
    }

    static void CreateInfoJson(const std::string &filePath, uint16_t deviceId)
    {
        // info.json
        nlohmann::json info = {
            {"platform_version", "5"},
            {"devices", "0"},
            {"pid", "2376271"},
            {"DeviceInfo", {{{"hwts_frequency", "49.000000"},
                             {"aic_frequency", "1850"},
                             {"aiv_frequency", "1850aa"},
                             {"ai_core_num", 25},
                             {"aiv_num", 25}}}},
            {"hostname", "localhost"}
        };
        FileWriter infoWriter(File::PathJoin({filePath, INFO_JSON}));
        infoWriter.WriteText(info.dump());
    }
};

TEST_F(DeviceInfoUTest, TestGetInfoJsonShouldReturnFalseWhenInfoJsonInvalid)
{
    DeviceContext deviceContext;
    DeviceContextInfo deviceContextInfo;
    deviceContextInfo.deviceFilePath = DEVICE_DIR;
    deviceContext.deviceContextInfo = deviceContextInfo;
    EXPECT_FALSE(deviceContext.GetInfoJson());
}
}  // Domain
}  // Analysis
