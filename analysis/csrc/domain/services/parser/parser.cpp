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

#include "parser.h"

#include <algorithm>
#include <iostream>

#include "analysis/csrc/domain/services/parser/parser_error_code.h"

namespace Analysis
{
namespace Domain
{
using namespace Infra;
namespace
{
const long INVALID_FILE_SIZE = -1;
const uint64_t MAX_FILE_SIZE = 10737418240;  // 10 * 1024 *1024 *1024, means 10GB
}  // namespace

uint32_t Parser::GetFileSize(const char *filePath) const
{
    FILE *fp = fopen(filePath, "r");
    if (fp == nullptr)
    {
        ERROR("fopen error! path: %, error: %", filePath, PARSER_FOPEN_ERROR);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    auto fileSize = ftell(fp);
    fclose(fp);
    if (fileSize == INVALID_FILE_SIZE)
    {
        ERROR("Get File size failed! filePath is %, error: %", filePath, PARSER_FREAD_ERROR);
        return 0;
    }
    return static_cast<uint32_t>(fileSize);
}

uint64_t Parser::GetFilesSize(const std::vector<std::string> &paths)
{
    uint64_t fileSize = 0;

    uint32_t size;
    for (const auto &loop : paths)
    {
        size = this->GetFileSize(loop.c_str());
        INFO("loop: %, file size: %", loop, size);
        fileSize += size;
    }
    INFO("total size: %", fileSize);
    return fileSize;
}

uint32_t Parser::ReadData(const std::vector<std::string> &files, size_t firstFileOffset)
{
    uint64_t binaryDataOffset = 0;

    for (const auto &loop : files)
    {
        uint32_t fileSize = this->GetFileSize(loop.c_str());

        auto file = fopen(loop.c_str(), "r");
        if (file == nullptr)
        {
            ERROR("fopen error! path: %, error: %", loop, PARSER_FOPEN_ERROR);
            return PARSER_FOPEN_ERROR;
        }

        if (loop == *files.begin())
        {
            fseek(file, firstFileOffset, SEEK_SET);
            fileSize -= firstFileOffset;
        }

        auto code = fread(this->binaryData.get() + binaryDataOffset, sizeof(uint8_t), fileSize, file);
        if (code != fileSize)
        {
            ERROR("fread error! code = %, fileSize = %, error: %", code, fileSize, ferror(file));
            fclose(file);
            return Analysis::PARSER_FREAD_ERROR;
        }

        fclose(file);

        binaryDataOffset += fileSize;
    }
    INFO("Data has been read to the memory successfully!");
    return Analysis::ANALYSIS_OK;
}

std::string Parser::GetFilePath(const DeviceContext &deviceContext)
{
    std::string deviceFilePath = deviceContext.GetDeviceFilePath();
    return Analysis::Utils::File::PathJoin({deviceFilePath, "data"});
}

// 提取字符串尾部的数字部分
int ExtractNumber(const std::string &str)
{
    size_t pos = str.find_last_of('_');
    if (pos != std::string::npos)
    {
        std::string numStr = str.substr(pos + 1);
        uint32_t extraction;
        if (Utils::StrToU32(extraction, numStr) == ANALYSIS_OK)
        {
            return extraction;
        }
        ERROR("Failed to parse the slice number in the binary file: %", str);
    }
    return 0;  // 默认返回0
}

uint32_t Parser::ReadDataEntry(const DeviceContext &deviceContext)
{
    auto filePrefix = this->GetFilePattern();
    // 读取解析文件
    auto files =
        Analysis::Utils::File::GetOriginData(this->GetFilePath(deviceContext), filePrefix, {"done", "complete"});
    if (files.empty())
    {
        INFO("notify: no files pattern, don't need to parse");
        this->binaryData = nullptr;
        this->binaryDataSize = 0;
        return ANALYSIS_OK;
    }

    // 自然排序文件名称
    std::sort(files.begin(), files.end(),
              [](const std::string &a, const std::string &b)
              {
                  // 提取数字部分并转换为整数进行比较
                  int num_a = ExtractNumber(a);
                  int num_b = ExtractNumber(b);
                  return num_a < num_b;
              });

    // 读取每个处理块大小
    auto trunkSize = this->GetTrunkSize();

    // 读取所有文件大小
    auto fileSize = this->GetFilesSize(files);
    // 同一类二进制文件总大小不能超过10GB
    if (fileSize > MAX_FILE_SIZE)
    {
        ERROR("FileSize is too large, more than 10 GB");
        return ANALYSIS_ERROR;
    }
    // 计算处理块个数
    size_t structCount = fileSize / trunkSize;
    size_t firstFileOffset = fileSize % trunkSize;
    if (firstFileOffset != 0)
    {
        // 如果有余，从开始offset偏移获取！
        INFO("offset: %", firstFileOffset);
    }

    this->binaryData.reset(new (std::nothrow) uint8_t[structCount * trunkSize]);
    if (this->binaryData == nullptr)
    {
        ERROR("new binary data error!");
        return Analysis::PARSER_NEW_BINARY_DATA_ERROR;
    }
    this->binaryDataSize = structCount * trunkSize;
    INFO("Parse filePrefix is: %, the number of files is: %, and the total size of all files is: %",
         Analysis::Utils::Join(filePrefix, ","), files.size(), structCount * trunkSize);
    return this->ReadData(files, firstFileOffset);
}

uint32_t Parser::ProcessEntry(DataInventory &dataInventory, const Infra::Context &context)
{
    const DeviceContext &deviceContext = static_cast<const DeviceContext &>(context);
    uint32_t code = this->ReadDataEntry(deviceContext);
    if (code != Analysis::ANALYSIS_OK)
    {
        ERROR("ReadData error: %", code);
        return Analysis::PARSER_READ_DATA_ERROR;
    }

    code = this->ParseData(dataInventory, context);
    if (code != Analysis::ANALYSIS_OK)
    {
        ERROR("ParseData error: %", code);
        return Analysis::PARSER_PARSE_DATA_ERROR;
    }
    INFO("Parser is completed!");
    return Analysis::ANALYSIS_OK;
}

}  // namespace Domain
}  // namespace Analysis
