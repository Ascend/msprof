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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "analysis/csrc/domain/services/persistence/device/freq_persistence.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/entities/hal/include/hal_freq.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace testing;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace Analysis {
namespace Domain {
namespace {
    const std::string DEVICE_PATH = "./device_0";
}
class FreqPersistenceUTest : public Test {
protected:
    void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({DEVICE_PATH, "sqlite"})));
        auto data = std::make_shared<std::vector<HalFreqLpmData>>();
        dataInventory_.Inject(data);
    }

    void TearDown()
    {
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(DEVICE_PATH, 0));
    }

protected:
    DataInventory dataInventory_;
};

static std::vector<HalFreqLpmData> CreateFreqData(uint64_t sysCnt, uint32_t freq)
{
    std::vector<HalFreqLpmData> res;
    HalFreqLpmData freqData{};
    freqData.sysCnt = sysCnt;
    freqData.freq = freq;
    res.emplace_back(freqData);
    res.emplace_back(freqData);
    return res;
}

TEST_F(FreqPersistenceUTest, ShouldSaveFreqDataSuccess)
{
    FreqPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    auto freqDataS = dataInventory_.GetPtr<std::vector<HalFreqLpmData>>();
    auto freqData = CreateFreqData(1000, 1800);
    freqDataS->swap(freqData);
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context));
}

TEST_F(FreqPersistenceUTest, ShouldReturnErrorWhenDataIsNullPtr)
{
    FreqPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    dataInventory_.RemoveRestData({});
    ASSERT_EQ(ANALYSIS_ERROR, persistence.Run(dataInventory_, context));
}

TEST_F(FreqPersistenceUTest, ShouldReturnOKWhenDataIsEmpty)
{
    FreqPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context));
}
}
}