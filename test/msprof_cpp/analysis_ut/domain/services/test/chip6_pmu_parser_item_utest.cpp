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
#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/chip6_pmu_parser_item.h"

namespace Analysis {
    using namespace testing;
    using namespace Analysis::Domain;

    class Chip6PmuParserItemUtest : public Test {
    protected:
        void SetUp() override
        {
        }
    };

    namespace {
    Chip6Pmu BuildChip6Pmu()
    {
        Chip6Pmu pmu;
        pmu.cnt = 9;
        pmu.taskId = 0x12345678;
        pmu.totalCycle = 10000;
        pmu.flags = 0;  // mst=0 mix=0 ov_flag=0
        pmu.coreType = 0;
        pmu.coreId = 3;
        pmu.subBlockId = 0x1111;
        pmu.blockId = 0x2222;
        for (int i = 0; i < V6_PMU_LENGTH; ++i) {
            pmu.pmuList[i] = static_cast<uint64_t>(i + 1);
        }
        pmu.startTime = 1000;
        pmu.endTime = 2000;
        return pmu;
    }
    }  // namespace

    TEST_F(Chip6PmuParserItemUtest, ShouldReturnSizeMismatchWhenInvalidSize)
    {
        Chip6Pmu pmu;
        HalPmuData halData;
        ASSERT_EQ(Chip6PmuParseItem(reinterpret_cast<uint8_t *>(&pmu), sizeof(pmu) - 1,
            reinterpret_cast<uint8_t *>(&halData), 0), Analysis::PARSER_ERROR_SIZE_MISMATCH);
        ASSERT_EQ(Chip6BlockPmuParseItem(reinterpret_cast<uint8_t *>(&pmu), sizeof(pmu) - 1,
            reinterpret_cast<uint8_t *>(&halData), 0), Analysis::PARSER_ERROR_SIZE_MISMATCH);
    }

    TEST_F(Chip6PmuParserItemUtest, ShouldParseContextPmuSuccessfully)
    {
        Chip6Pmu pmu = BuildChip6Pmu();
        HalPmuData halData;
        int ret = Chip6PmuParseItem(reinterpret_cast<uint8_t *>(&pmu), sizeof(pmu),
            reinterpret_cast<uint8_t *>(&halData), 0);

        ASSERT_EQ(ret, pmu.cnt);
        ASSERT_EQ(halData.type, PMU);
        // V6 PMU未携带streamId，解析时填默认值UINT16_MAX，后续由PmuAssociation用host task表替换
        ASSERT_EQ(halData.hd.taskId.streamId, 65535u);
        ASSERT_EQ(halData.hd.taskId.batchId, INVALID_BATCH_ID);
        ASSERT_EQ(halData.hd.taskId.taskId, 0x12345678u);
        ASSERT_EQ(halData.hd.taskId.contextId, INVALID_CONTEXT_ID);
        ASSERT_EQ(halData.hd.timestamp, 2000u);
        ASSERT_EQ(halData.pmu.acceleratorType, AIC);
        ASSERT_EQ(halData.pmu.ovFlag, 0);
        ASSERT_EQ(halData.pmu.totalCycle, 10000u);
        ASSERT_EQ(halData.pmu.subBlockId, 0x1111);
        ASSERT_EQ(halData.pmu.blockId, 0x2222);
        ASSERT_EQ(halData.pmu.coreType, 0);
        ASSERT_EQ(halData.pmu.coreId, 3);
        ASSERT_EQ(halData.pmu.pmuList.size(), static_cast<size_t>(V6_PMU_LENGTH));
        for (int i = 0; i < V6_PMU_LENGTH; ++i) {
            ASSERT_EQ(halData.pmu.pmuList[i], static_cast<uint64_t>(i + 1));
        }
        ASSERT_EQ(halData.pmu.timeList[0], 1000u);
        ASSERT_EQ(halData.pmu.timeList[1], 2000u);
    }

    TEST_F(Chip6PmuParserItemUtest, ShouldParseBlockPmuWithBlockType)
    {
        Chip6Pmu pmu = BuildChip6Pmu();
        HalPmuData halData;
        int ret = Chip6BlockPmuParseItem(reinterpret_cast<uint8_t *>(&pmu), sizeof(pmu),
            reinterpret_cast<uint8_t *>(&halData), 0);

        ASSERT_EQ(ret, pmu.cnt);
        ASSERT_EQ(halData.type, BLOCK_PMU);
    }

    TEST_F(Chip6PmuParserItemUtest, ShouldDetermineAcceleratorTypeByMixAndCoreType)
    {
        auto ParseWith = [](uint8_t flags, uint8_t coreType) {
            Chip6Pmu pmu;
            pmu.flags = flags;
            pmu.coreType = coreType;
            HalPmuData halData;
            Chip6PmuParseItem(reinterpret_cast<uint8_t *>(&pmu), sizeof(pmu),
                reinterpret_cast<uint8_t *>(&halData), 0);
            return halData.pmu.acceleratorType;
        };

        EXPECT_EQ(ParseWith(0b000, 0), AIC);
        EXPECT_EQ(ParseWith(0b000, 1), AIV);
        EXPECT_EQ(ParseWith(0b010, 0), MIX_AIC);
        EXPECT_EQ(ParseWith(0b010, 1), MIX_AIV);
    }

    TEST_F(Chip6PmuParserItemUtest, ShouldSetOvFlagWhenOverflowBitSet)
    {
        Chip6Pmu pmu = BuildChip6Pmu();
        pmu.flags = 0b100;  // ov_flag=1
        HalPmuData halData;
        Chip6PmuParseItem(reinterpret_cast<uint8_t *>(&pmu), sizeof(pmu),
            reinterpret_cast<uint8_t *>(&halData), 0);

        ASSERT_EQ(halData.pmu.ovFlag, 1);
    }
}