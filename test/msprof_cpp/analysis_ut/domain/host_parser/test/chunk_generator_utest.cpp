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

#include <memory>
#include <string>
#include <vector>

#include "analysis/csrc/domain/services/parser/host/chunk_generator.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "gtest/gtest.h"

using namespace Analysis::Domain;
using namespace Analysis::Utils;

namespace
{
const std::string CG_DATA_DIR = "./chunk_generator_utest";
const uint32_t CG_CHUNK_SIZE = 256;
const std::vector<std::string> CG_FILE_PREFIX = {"unaging.additional.cg_utest.slice",
                                                 "aging.additional.cg_utest.slice"};

std::string MakeRecord(char value) { return std::string(CG_CHUNK_SIZE, value); }

void WriteFile(const std::string &dir, const std::string &fileName, const std::string &content)
{
    FileWriter writer(File::PathJoin({dir, fileName}), std::ios::out | std::ios::binary);
    writer.WriteText(content.data(), content.size());
}

std::vector<std::string> PopAll(ChunkGenerator &generator)
{
    std::vector<std::string> chunks;
    while (!generator.Empty())
    {
        std::unique_ptr<char[]> chunk(generator.Pop());
        EXPECT_NE(nullptr, chunk.get());
        if (chunk == nullptr)
        {
            break;
        }
        chunks.emplace_back(chunk.get(), CG_CHUNK_SIZE);
    }
    return chunks;
}
}  // namespace

class ChunkGeneratorUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        File::RemoveDir(CG_DATA_DIR, 0);
        ASSERT_TRUE(File::CreateDir(CG_DATA_DIR));
    }

    void TearDown() override { File::RemoveDir(CG_DATA_DIR, 0); }

    std::string CaseDir(const std::string &name)
    {
        auto dir = File::PathJoin({CG_DATA_DIR, name});
        EXPECT_TRUE(File::CreateDir(dir));
        return dir;
    }
};

TEST_F(ChunkGeneratorUTest, ThreeParamApiReadsAlignedSlicesInAgingOrder)
{
    auto dir = CaseDir("aligned");
    WriteFile(dir, "unaging.additional.cg_utest.slice_0", MakeRecord('A'));
    WriteFile(dir, "aging.additional.cg_utest.slice_1", MakeRecord('B'));

    ChunkGenerator generator(CG_CHUNK_SIZE, dir, CG_FILE_PREFIX);
    ASSERT_EQ(Analysis::ANALYSIS_OK, generator.ReadChunk());
    ASSERT_EQ(2U, generator.Size());
    const std::vector<std::string> expected = {MakeRecord('A'), MakeRecord('B')};
    EXPECT_EQ(expected, PopAll(generator));
}

TEST_F(ChunkGeneratorUTest, UnalignedFileSizeReturnsError)
{
    auto badDir = CaseDir("unaligned");
    WriteFile(badDir, "unaging.additional.cg_utest.slice_0", std::string(10, 'x') + MakeRecord('A'));
    ChunkGenerator badGenerator(CG_CHUNK_SIZE, badDir, CG_FILE_PREFIX);
    EXPECT_EQ(Analysis::ANALYSIS_ERROR, badGenerator.ReadChunk());
}

TEST_F(ChunkGeneratorUTest, EmptyDirAndZeroByteFileAreOk)
{
    auto emptyDir = CaseDir("empty");
    auto zeroDir = CaseDir("zero");
    WriteFile(zeroDir, "unaging.additional.cg_utest.slice_0", "");

    ChunkGenerator emptyGenerator(CG_CHUNK_SIZE, emptyDir, CG_FILE_PREFIX);
    EXPECT_EQ(Analysis::ANALYSIS_OK, emptyGenerator.ReadChunk());
    EXPECT_TRUE(emptyGenerator.Empty());

    ChunkGenerator zeroGenerator(CG_CHUNK_SIZE, zeroDir, CG_FILE_PREFIX);
    EXPECT_EQ(Analysis::ANALYSIS_OK, zeroGenerator.ReadChunk());
    EXPECT_TRUE(zeroGenerator.Empty());
}

TEST_F(ChunkGeneratorUTest, MultipleAlignedRecordsFromOneFile)
{
    auto dir = CaseDir("multi");
    WriteFile(dir, "unaging.additional.cg_utest.slice_0", MakeRecord('A') + MakeRecord('B'));
    ChunkGenerator generator(CG_CHUNK_SIZE, dir, CG_FILE_PREFIX);
    ASSERT_EQ(Analysis::ANALYSIS_OK, generator.ReadChunk());
    const std::vector<std::string> expected = {MakeRecord('A'), MakeRecord('B')};
    EXPECT_EQ(expected, PopAll(generator));
}
