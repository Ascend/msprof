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
#include "analysis/csrc/domain/services/persistence/host/model_name_db_dumper.h"

using namespace Analysis::Domain;

const std::string HOST_FILE_PATH = "host";

class ModelNameDBDumperUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(ModelNameDBDumperUtest, TestGenerateDataSuccess)
{
    ModelNameDBDumper dumper(HOST_FILE_PATH);
    ModelNameDBDumper::GraphIdMaps maps = {std::make_shared<ParserAdditionalInfo>()};
    dumper.GenerateData(maps);
    EXPECT_EQ(maps.size(), 1ul);
}