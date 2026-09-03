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

#include "analysis/csrc/domain/services/parser/host/cann/addition_info_parser.h"

#include <memory>
#include <unordered_map>

#include "analysis/csrc/domain/services/adapter/parser_struct_adapter.h"
#include "analysis/csrc/domain/services/parser/host/cann/tensor_desc_formatter.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{
using namespace Analysis::Utils;
using namespace Analysis::Domain::Adapter;
namespace
{
ParserTensorData ConvertTensorData(const MsrofTensorData &src) { return TensorDescFormatter::ToParserTensor(src); }

std::shared_ptr<ParserConcatTensorInfo> CreateConcatTensorInfo(MsprofAdditionalInfo *additionalInfo)
{
    if (!additionalInfo)
    {
        ERROR("Additional info is null.");
        return nullptr;
    }
    std::shared_ptr<ParserConcatTensorInfo> concatTensorInfo;
    MAKE_SHARED0_RETURN_VALUE(concatTensorInfo, ParserConcatTensorInfo, nullptr);

    concatTensorInfo->level = additionalInfo->level;
    concatTensorInfo->type = additionalInfo->type;
    concatTensorInfo->threadId = additionalInfo->threadId;
    concatTensorInfo->dataLen = additionalInfo->dataLen;
    concatTensorInfo->timeStamp = additionalInfo->timeStamp;
    auto tensorInfo = ReinterpretConvert<MsprofTensorInfo *>(additionalInfo->data);
    concatTensorInfo->opName = tensorInfo->opName;
    concatTensorInfo->tensorNum = tensorInfo->tensorNum;
    for (uint32_t i = 0; i < tensorInfo->tensorNum; ++i)
    {
        concatTensorInfo->tensorData[i] = ConvertTensorData(tensorInfo->tensorData[i]);
    }
    return concatTensorInfo;
}
}  // namespace

void AdditionInfoParser::Init(const std::vector<std::string> &filePrefix)
{
    MAKE_SHARED_RETURN_VOID(chunkProducer_, ChunkGenerator, sizeof(MsprofAdditionalInfo), path_, filePrefix);
}

template <>
std::vector<std::shared_ptr<ParserAdditionalInfo>> AdditionInfoParser::GetData()
{
    return additionalData_;
}

template <>
std::vector<std::shared_ptr<ParserConcatTensorInfo>> AdditionInfoParser::GetData()
{
    return concatTensorData_;
}

int AdditionInfoParser::ProduceData()
{
    if (chunkProducer_->Empty())
    {
        return ANALYSIS_OK;
    }
    if (!Reserve(additionalData_, chunkProducer_->Size()))
    {
        ERROR("%: Reserve data failed", parserName_);
        return ANALYSIS_ERROR;
    }
    while (!chunkProducer_->Empty())
    {
        std::unique_ptr<char[]> chunk(chunkProducer_->Pop());
        auto additionalInfo = ReinterpretConvert<MsprofAdditionalInfo *>(chunk.get());
        if (!additionalInfo)
        {
            ERROR("%: Pop chunk failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (additionalInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM)
        {
            ERROR("%: Invalid magic number, record discarded, % records remaining.", parserName_,
                  chunkProducer_->Size());
            continue;
        }
        if (!IsDataValid(*additionalInfo))
        {
            ERROR("%: Invalid payload length %, record discarded, % records remaining.", parserName_,
                  additionalInfo->dataLen, chunkProducer_->Size());
            continue;
        }
        auto parser = std::make_shared<ParserAdditionalInfo>();
        if (!ParserAdditionalInfoAdapter::AdapterAdditionalInfo(additionalInfo, parser.get(), parserType_))
        {
            ERROR("%: copy addition info data failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        additionalData_.emplace_back(std::move(parser));
    }
    return ANALYSIS_OK;
}

bool AdditionInfoParser::IsDataValid(const MsprofAdditionalInfo &) const { return true; }

bool TaskMemoryParser::IsDataValid(const MsprofAdditionalInfo &additionalInfo) const
{
    return additionalInfo.dataLen >= sizeof(MsprofMemoryInfo);
}

int TensorInfoParser::ProduceData()
{
    if (chunkProducer_->Empty())
    {
        return ANALYSIS_OK;
    }
    if (!Reserve(concatTensorData_, chunkProducer_->Size()))
    {
        ERROR("%: Reserve data failed", parserName_);
        return ANALYSIS_ERROR;
    }
    std::unordered_map<std::string, std::shared_ptr<ParserConcatTensorInfo>> concatTensorMap;
    while (!chunkProducer_->Empty())
    {
        std::unique_ptr<char[]> chunk(chunkProducer_->Pop());
        auto currTensorInfo = ReinterpretConvert<MsprofAdditionalInfo *>(chunk.get());
        if (!currTensorInfo)
        {
            ERROR("%: Pop chunk failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (currTensorInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM)
        {
            ERROR("%: Invalid magic number, record discarded, % records remaining.", parserName_,
                  chunkProducer_->Size());
            continue;
        }
        auto currTensor = ReinterpretConvert<MsprofTensorInfo *>(currTensorInfo->data);
        std::string key = Utils::Join("_", currTensor->opName, currTensorInfo->timeStamp, currTensorInfo->threadId);
        if (concatTensorMap.find(key) == concatTensorMap.end())
        {
            auto concatTensor = CreateConcatTensorInfo(currTensorInfo);
            if (!concatTensor)
            {
                ERROR("%: Create concat tensor failed.");
                return ANALYSIS_ERROR;
            }
            concatTensorMap.insert({key, concatTensor});
            continue;
        }
        auto concatTensor = concatTensorMap[key];
        // tensor info拼接
        for (uint32_t i = 0; i < currTensor->tensorNum; ++i)
        {
            if (concatTensor->tensorNum >= MSPROF_GE_TENSOR_DATA_NUM)
            {
                concatTensor->tensorData.emplace_back(ConvertTensorData(currTensor->tensorData[i]));
            }
            else
            {
                concatTensor->tensorData[concatTensor->tensorNum] = ConvertTensorData(currTensor->tensorData[i]);
            }
            concatTensor->tensorNum += 1;
        }
    }
    for (const auto &kv : concatTensorMap)
    {
        concatTensorData_.emplace_back(kv.second);
    }
    return ANALYSIS_OK;
}
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis
