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

#include <cstdint>
#include <initializer_list>

#include "gtest/gtest.h"

#include "analysis/csrc/domain/services/parser/host/cann/tensor_desc_formatter.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"
#include "analysis/csrc/infrastructure/utils/prof_struct.h"

using namespace Analysis::Common;
using namespace Analysis::Domain;
using namespace Analysis::Domain::Host::Cann;

namespace
{
ParserTensorData MakeParserTensor(uint32_t tensorType, uint32_t format, uint32_t dataType,
                                  std::initializer_list<uint32_t> dims)
{
    ParserTensorData tensor{};
    tensor.tensorType = tensorType;
    tensor.format = format;
    tensor.dataType = dataType;
    uint32_t i = 0;
    for (auto dim : dims)
    {
        if (i >= MSPROF_GE_TENSOR_DATA_SHAPE_LEN)
        {
            break;
        }
        tensor.shape[i++] = dim;
    }
    return tensor;
}
}  // namespace

class TensorDescFormatterUTest : public testing::Test
{
};

TEST_F(TensorDescFormatterUTest, TestGetFormatShouldReturnUndefinedWhenFormatIsUint32Max)
{
    EXPECT_EQ("UNDEFINED", TensorDescFormatter::GetFormat(UINT32_MAX));
}

TEST_F(TensorDescFormatterUTest, TestGetFormatShouldReturnNchwWhenFormatIsZero)
{
    EXPECT_EQ("NCHW", TensorDescFormatter::GetFormat(0));
}

TEST_F(TensorDescFormatterUTest, TestGetFormatShouldReturnNdWhenFormatIsTwo)
{
    EXPECT_EQ("ND", TensorDescFormatter::GetFormat(2));
}

TEST_F(TensorDescFormatterUTest, TestGetFormatShouldAppendSubFormatWhenSubFormatGreaterThanZero)
{
    EXPECT_EQ("NCHW:1", TensorDescFormatter::GetFormat(0x000100));
}

TEST_F(TensorDescFormatterUTest, TestToParserTensorShouldCopyFieldsFromMsrofTensorData)
{
    MsrofTensorData src{};
    src.tensorType = 0;
    src.format = 2;
    src.dataType = 0;
    src.shape[0] = 1;
    src.shape[1] = 2;
    src.shape[2] = 3;
    auto dst = TensorDescFormatter::ToParserTensor(src);
    EXPECT_EQ(src.tensorType, dst.tensorType);
    EXPECT_EQ(src.format, dst.format);
    EXPECT_EQ(src.dataType, dst.dataType);
    EXPECT_EQ(1u, dst.shape[0]);
    EXPECT_EQ(2u, dst.shape[1]);
    EXPECT_EQ(3u, dst.shape[2]);
}

TEST_F(TensorDescFormatterUTest, TestToParserTensorShouldCopyFieldsFromRuntimeOpTensor)
{
    MsprofRuntimeOpTensor src{};
    src.tensorType = 1;
    src.format = 1;
    src.dataType = 12;
    src.shape[0] = 4;
    src.shape[1] = 9;
    auto dst = TensorDescFormatter::ToParserTensor(src);
    EXPECT_EQ(src.tensorType, dst.tensorType);
    EXPECT_EQ(src.format, dst.format);
    EXPECT_EQ(src.dataType, dst.dataType);
    EXPECT_EQ(4u, dst.shape[0]);
    EXPECT_EQ(9u, dst.shape[1]);
}

TEST_F(TensorDescFormatterUTest, TestFormatShouldReturnNaWhenTensorsIsNullOrEmpty)
{
    auto emptyPtr = TensorDescFormatter::Format(static_cast<const ParserTensorData *>(nullptr), 1);
    EXPECT_EQ(NA, emptyPtr.inputFormats);
    EXPECT_EQ(NA, emptyPtr.inputDataTypes);
    EXPECT_EQ(NA, emptyPtr.inputShapes);
    EXPECT_EQ(NA, emptyPtr.outputFormats);
    EXPECT_EQ(NA, emptyPtr.outputDataTypes);
    EXPECT_EQ(NA, emptyPtr.outputShapes);

    auto zeroNum = TensorDescFormatter::Format(std::vector<ParserTensorData>{MakeParserTensor(0, 0, 0, {1})}, 0);
    EXPECT_EQ(NA, zeroNum.inputFormats);

    auto emptyVec = TensorDescFormatter::Format(std::vector<ParserTensorData>{}, 3);
    EXPECT_EQ(NA, emptyVec.outputShapes);
}

TEST_F(TensorDescFormatterUTest, TestFormatShouldJoinInputAndOutputAndQuoteShapes)
{
    std::vector<ParserTensorData> tensors = {
        MakeParserTensor(0, 2, 9, {4, 9}),     // input ND INT64
        MakeParserTensor(0, 0, 0, {1, 2, 3}),  // input NCHW FLOAT
        MakeParserTensor(1, 1, 12, {9}),       // output NHWC BOOL
    };
    auto fields = TensorDescFormatter::Format(tensors, static_cast<uint32_t>(tensors.size()));
    EXPECT_EQ("ND;NCHW", fields.inputFormats);
    EXPECT_EQ("INT64;FLOAT", fields.inputDataTypes);
    EXPECT_EQ("\"4,9;1,2,3\"", fields.inputShapes);
    EXPECT_EQ("NHWC", fields.outputFormats);
    EXPECT_EQ("BOOL", fields.outputDataTypes);
    EXPECT_EQ("\"9\"", fields.outputShapes);
}

TEST_F(TensorDescFormatterUTest, TestFormatShouldStopShapeAtFirstZero)
{
    auto tensor = MakeParserTensor(0, 2, 0, {9, 0, 5});
    auto fields = TensorDescFormatter::Format(&tensor, 1);
    EXPECT_EQ("\"9\"", fields.inputShapes);
}

TEST_F(TensorDescFormatterUTest, TestFormatShouldCapCountByVectorSize)
{
    std::vector<ParserTensorData> tensors = {
        MakeParserTensor(0, 2, 0, {1}),
        MakeParserTensor(1, 2, 0, {2}),
    };
    auto fields = TensorDescFormatter::Format(tensors, 8);
    EXPECT_EQ("ND", fields.inputFormats);
    EXPECT_EQ("ND", fields.outputFormats);
    EXPECT_EQ("\"1\"", fields.inputShapes);
    EXPECT_EQ("\"2\"", fields.outputShapes);
}

TEST_F(TensorDescFormatterUTest, TestFormatShouldIgnoreUnknownTensorType)
{
    auto tensor = MakeParserTensor(3, 2, 0, {1, 2});
    auto fields = TensorDescFormatter::Format(&tensor, 1);
    EXPECT_EQ(NA, fields.inputFormats);
    EXPECT_EQ(NA, fields.outputFormats);
}

TEST_F(TensorDescFormatterUTest, TestFormatShouldKeepOutputNaWhenOnlyInput)
{
    auto tensor = MakeParserTensor(0, 2, 1, {8, 8});
    auto fields = TensorDescFormatter::Format(&tensor, 1);
    EXPECT_EQ("ND", fields.inputFormats);
    EXPECT_EQ("FLOAT16", fields.inputDataTypes);
    EXPECT_EQ("\"8,8\"", fields.inputShapes);
    EXPECT_EQ(NA, fields.outputFormats);
    EXPECT_EQ(NA, fields.outputDataTypes);
    EXPECT_EQ(NA, fields.outputShapes);
}
