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
#include "analysis/csrc/domain/entities/metric/include/metric.h"

using namespace testing;
namespace Analysis {
namespace Domain {
class MetricUTest : public Test {
protected:
    void SetUp() override
    {}

    void TearDown() override
    {}
};

enum class InvalidType {
    TEST_INVALID = 0
};

TEST_F(MetricUTest, ShouldReturnRightHeaderStringWhenInputEnumType)
{
    ASSERT_EQ("mac_fp16_ratio", Metric::GetMetricHeaderString(ArithMetricIndex::MacFp16Ratio));
    ASSERT_EQ("INVALID", Metric::GetMetricHeaderString(InvalidType::TEST_INVALID));
}

TEST_F(MetricUTest, ShouldReturnChip6HeaderString)
{
    ASSERT_EQ("read_local_l2_hit", Metric::GetMetricHeaderString(L2CacheIndex::ReadLocalL2Hit));
    ASSERT_EQ("write_local_l2_victim", Metric::GetMetricHeaderString(L2CacheIndex::WriteLocalL2Victim));
    ASSERT_EQ("fixp2ub_write_bw", Metric::GetMetricHeaderString(MemoryUBIndex::Fixp2UbWriteBw));
}
}
}