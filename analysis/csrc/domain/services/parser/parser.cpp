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

uint32_t Parser::ReadData(const std::vector<std::string> &files, uint64_t firstFileOffset)
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

        uint64_t bytesToRead = fileSize;
        if (loop == *files.begin())
        {
            if (firstFileOffset > fileSize)
            {
                ERROR("first file offset % exceeds first file size %", firstFileOffset, fileSize);
                fclose(file);
                return ANALYSIS_ERROR;
            }
            if (fseek(file, static_cast<long>(firstFileOffset), SEEK_SET) != 0)
            {
                ERROR("fseek error! path: %, offset: %", loop, firstFileOffset);
                fclose(file);
                return ANALYSIS_ERROR;
            }
            bytesToRead = static_cast<uint64_t>(fileSize) - firstFileOffset;
        }

        if (binaryDataOffset > this->binaryDataSize || bytesToRead > this->binaryDataSize - binaryDataOffset)
        {
            ERROR("read length exceeds remaining binary capacity, offset: %, length: %, capacity: %", binaryDataOffset,
                  bytesToRead, this->binaryDataSize);
            fclose(file);
            return ANALYSIS_ERROR;
        }

        auto code =
            fread(this->binaryData.get() + binaryDataOffset, sizeof(uint8_t), static_cast<size_t>(bytesToRead), file);
        if (code != bytesToRead)
        {
            ERROR("fread error! code = %, fileSize = %, error: %", code, bytesToRead, ferror(file));
            fclose(file);
            return Analysis::PARSER_FREAD_ERROR;
        }

        fclose(file);

        binaryDataOffset += bytesToRead;
    }
    if (binaryDataOffset != this->binaryDataSize)
    {
        ERROR("read size % does not match binary data size %", binaryDataOffset, this->binaryDataSize);
        return ANALYSIS_ERROR;
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
    if (trunkSize == 0)
    {
        ERROR("trunk size is 0, cannot read records");
        return ANALYSIS_ERROR;
    }

    // 读取所有文件大小
    auto fileSize = this->GetFilesSize(files);
    const uint64_t firstFileOffset = this->GetReadOffset(fileSize, trunkSize);
    if (firstFileOffset > fileSize)
    {
        ERROR("read offset % exceeds file size %", firstFileOffset, fileSize);
        return ANALYSIS_ERROR;
    }
    if (firstFileOffset != 0)
    {
        // 如果有余，从开始offset偏移获取！
        INFO("offset: %", firstFileOffset);
    }

    const uint64_t payloadSize = fileSize - firstFileOffset;
    this->binaryData.reset(new (std::nothrow) uint8_t[payloadSize]);
    if (this->binaryData == nullptr)
    {
        ERROR("new binary data error!");
        return Analysis::PARSER_NEW_BINARY_DATA_ERROR;
    }
    this->binaryDataSize = payloadSize;
    INFO("Parse filePrefix is: %, the number of files is: %, and the total size of all files is: %",
         Analysis::Utils::Join(filePrefix, ","), files.size(), payloadSize);
    return this->ReadData(files, firstFileOffset);
}

uint64_t Parser::GetReadOffset(uint64_t fileSize, uint32_t trunkSize) const
{
    if (trunkSize == 0)
    {
        return 0;
    }
    return fileSize % trunkSize;
}

uint32_t Parser::ProcessEntry(DataInventory &dataInventory, const Infra::Context &context)
{
    const DeviceContext &deviceContext = static_cast<const DeviceContext &>(context);
    uint32_t code = this->Prepare(deviceContext);
    if (code != Analysis::ANALYSIS_OK)
    {
        ERROR("Prepare parser error: %", code);
        return Analysis::PARSER_GET_FILE_ERROR;
    }
    code = this->ReadDataEntry(deviceContext);
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
