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

#include "gtest/gtest.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"

using namespace Analysis::Utils;
using namespace Analysis::Domain::Host::Cann;

const std::string BASE_PATH = "./type_data";
const std::string TEST_FILE = "./type_data/unaging.additional.type_info_dic.slice_0";
const int DEPTH = 0;

class TypeDataUTest : public testing::Test {
protected:
    // 所有测试用例之前执行
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        // 创建test_file文件
        FileWriter fw(TEST_FILE);
        // 有效输入
        fw.WriteText("20000_655361:isGraphNeedRebuild\n");
        fw.WriteText("5000_1125:CpuKernelLaunchExWithArgs_Big\n");
        fw.WriteText("5000_1077:MemCopy2DAsync\n");
        // 无效输入
        fw.WriteText("20000_655361:isGraphNeedRebuild:t1\n");
        fw.WriteText("5000_1125_t2:CpuKernelLaunchExWithArgs_Big\n");
        fw.WriteText("5000t3_1077:MemCopy2DAsync\n");
        fw.WriteText("5000_1077_t4:MemCopy2DAsync\n");
        fw.Close();
    }
    // 所有测试用例之后执行
    static void TearDownTestCase()
    {
        File::RemoveDir(BASE_PATH, DEPTH);
    }
};

TEST_F(TypeDataUTest, LoadShouldReturnFalseWhenFileNotExist)
{
    auto status = TypeData::GetInstance().Load("test");
    EXPECT_FALSE(status);
    TypeData::GetInstance().Clear();
}

TEST_F(TypeDataUTest, LoadShouldReturnTrueWhenFileIsOK)
{
    auto status = TypeData::GetInstance().Load(BASE_PATH);
    EXPECT_TRUE(status);
    TypeData::GetInstance().Clear();
}

TEST_F(TypeDataUTest, GetShouldReturnNumWhenNoTypeInDict)
{
    TypeData::GetInstance().Load(BASE_PATH);
    std::string expectRes = "10";
    auto str = TypeData::GetInstance().Get(100, 10);
    auto str2 = TypeData::GetInstance().Get(5000, 10);
    EXPECT_STREQ(str.c_str(), expectRes.c_str());
    EXPECT_STREQ(str2.c_str(), expectRes.c_str());
    TypeData::GetInstance().Clear();
}

TEST_F(TypeDataUTest, GetShouldReturnStrWhenTypeInDict)
{
    TypeData::GetInstance().Load(BASE_PATH);
    std::string expectRes = "CpuKernelLaunchExWithArgs_Big";
    auto str = TypeData::GetInstance().Get(5000, 1125);
    EXPECT_STREQ(str.c_str(), expectRes.c_str());
    TypeData::GetInstance().Clear();
}

TEST_F(TypeDataUTest, GetAllShouldReturn3ValidResInFile)
{
    TypeData::GetInstance().Load(BASE_PATH);
    int expectResSize = 2;
    int expect5000ResSize = 2;
    int expect20000ResSize = 1;
    uint16_t hcclLevel = 5000;
    uint16_t aclLevel = 20000;
    EXPECT_EQ(expectResSize, TypeData::GetInstance().GetAll().size());
    EXPECT_EQ(expect5000ResSize, TypeData::GetInstance().GetAll()[hcclLevel].size());
    EXPECT_EQ(expect20000ResSize, TypeData::GetInstance().GetAll()[aclLevel].size());
    TypeData::GetInstance().Clear();
}