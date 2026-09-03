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

#include "analysis/csrc/domain/services/parser/host/cann/variable_info_parser.h"

#include <memory>
#include <vector>

#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/domain/services/parser/host/cann/tensor_desc_formatter.h"
#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"
#include "analysis/csrc/domain/services/parser/host/chunk_generator.h"
#include "analysis/csrc/domain/services/persistence/host/number_mapping.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
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
using MappingType = Analysis::Domain::NumberMapping::MappingType;

namespace
{
static_assert(sizeof(MsprofRuntimeOpInfoPayload) == 64, "runtime op payload size");
static_assert(sizeof(MsprofRuntimeOpTensor) == 44, "runtime op tensor size");
constexpr uint16_t kDynamicOp = 1;
constexpr uint16_t kStaticOp = 0;
constexpr uint16_t kBlockNumMask = 0xFFFF;
constexpr uint32_t kBlockNumShift = 16;
// 采集侧 runtimeOpInfo 单算子固定格式上报：tensor 数远小于 1000、单条 dataLen 远小于 50000（实际取值再低一个量级）。
// 此处设硬上限，防止异常或被篡改的二进制数据令 tensorNum 过大，使下方 tensorBytes 的 uint32 乘法溢出，
// 绕过 dataLen 边界校验造成越界读
constexpr uint32_t kMaxTensorNum = 1000;
constexpr uint32_t kMaxDataLen = 50000;

void FillTensorFields(const MsprofRuntimeOpTensor *tensors, uint32_t tensorNum, RuntimeOpInfo &info)
{
    std::vector<ParserTensorData> parsed;
    parsed.reserve(tensorNum);
    for (uint32_t i = 0; i < tensorNum; ++i)
    {
        parsed.emplace_back(TensorDescFormatter::ToParserTensor(tensors[i]));
    }
    auto fields = TensorDescFormatter::Format(parsed, tensorNum);
    info.inputFormats = fields.inputFormats;
    info.inputDataTypes = fields.inputDataTypes;
    info.inputShapes = fields.inputShapes;
    info.outputFormats = fields.outputFormats;
    info.outputDataTypes = fields.outputDataTypes;
    info.outputShapes = fields.outputShapes;
}

bool AssembleRuntimeOpInfo(uint16_t level, uint32_t type, uint32_t threadId, uint64_t timeStamp, uint32_t dataLen,
                           const uint8_t *payload, uint16_t isDynamic, RuntimeOpInfo &info)
{
    if (payload == nullptr || dataLen < sizeof(MsprofRuntimeOpInfoPayload))
    {
        ERROR("RuntimeOpInfo payload is invalid, dataLen=%", dataLen);
        return false;
    }
    auto *body = ReinterpretConvert<const MsprofRuntimeOpInfoPayload *>(payload);
    // 见 kMaxTensorNum / kMaxDataLen 注释：采集格式固定且规模很小，超限视为异常数据直接拒绝
    if (body->tensorNum > kMaxTensorNum)
    {
        ERROR("RuntimeOpInfo tensor num is invalid, dataLen=%, tensorNum=%", dataLen, body->tensorNum);
        return false;
    }
    if (dataLen > kMaxDataLen)
    {
        ERROR("RuntimeOpInfo dataLen is invalid, dataLen=%, tensorNum=%", dataLen, body->tensorNum);
        return false;
    }
    const uint32_t tensorBytes = body->tensorNum * static_cast<uint32_t>(sizeof(MsprofRuntimeOpTensor));
    const uint32_t neededAfterHeader = static_cast<uint32_t>(sizeof(MsprofRuntimeOpInfoPayload)) + tensorBytes;
    if (dataLen < neededAfterHeader)
    {
        ERROR("data_len error: data_len is %, tensor num is %", dataLen, body->tensorNum);
        return false;
    }
    uint16_t blockNum = static_cast<uint16_t>(body->blockNum & kBlockNumMask);
    uint16_t mixBlockNum = static_cast<uint16_t>(blockNum * (body->blockNum >> kBlockNumShift));
    std::string taskType = NumberMapping::Get(MappingType::GE_TASK_TYPE, body->taskType);
    std::string opName = HashData::GetInstance().Get(body->nodeId);
    std::string opType = HashData::GetInstance().Get(body->opType);
    std::string hashId = body->hashId == 0 ? NA : std::to_string(body->hashId);
    info = RuntimeOpInfo{static_cast<uint16_t>(body->deviceId),
                         body->taskId,
                         blockNum,
                         mixBlockNum,
                         static_cast<uint16_t>(body->opFlag),
                         static_cast<uint16_t>(body->tensorNum),
                         body->streamId,
                         body->modelId,
                         taskType,
                         opType,
                         opName,
                         hashId,
                         std::to_string(isDynamic),
                         NA,
                         NA,
                         NA,
                         NA,
                         NA,
                         NA};
    info.level = NumberMapping::Get(MappingType::LEVEL, level);
    info.structType = TypeData::GetInstance().Get(level, type);
    info.threadId = threadId;
    info.timeStamp = timeStamp;
    if (body->tensorNum > 0)
    {
        auto *tensors = ReinterpretConvert<const MsprofRuntimeOpTensor *>(payload + sizeof(MsprofRuntimeOpInfoPayload));
        FillTensorFields(tensors, body->tensorNum, info);
    }
    return true;
}

int ParseAdditionalRecords(ChunkGenerator *producer, const std::string &parserName, uint16_t isDynamic,
                           std::vector<RuntimeOpInfo> &out)
{
    if (producer == nullptr)
    {
        ERROR("%: The chunk producer is null.", parserName);
        return ANALYSIS_ERROR;
    }
    while (!producer->Empty())
    {
        // Pop 内部按 new char[chunkSize] 分配，用 unique_ptr<char[]> 接管以保证按数组规则释放
        std::unique_ptr<char[]> chunk(producer->Pop());
        auto additionalInfo = ReinterpretConvert<MsprofAdditionalInfo *>(chunk.get());
        if (!additionalInfo)
        {
            ERROR("%: Pop additional capture_op_info failed.", parserName);
            return ANALYSIS_ERROR;
        }
        if (additionalInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM)
        {
            ERROR("%: additional capture_op_info magic check failed.", parserName);
            continue;
        }
        RuntimeOpInfo info;
        bool ok = AssembleRuntimeOpInfo(additionalInfo->level, additionalInfo->type, additionalInfo->threadId,
                                        additionalInfo->timeStamp, additionalInfo->dataLen, additionalInfo->data,
                                        isDynamic, info);
        if (!ok)
        {
            continue;
        }
        out.emplace_back(std::move(info));
    }
    return ANALYSIS_OK;
}
}  // namespace

void VariableInfoParser::Init(const std::vector<std::string> &filePrefix)
{
    MAKE_SHARED_NO_OPERATION(variableProducer_, VariableChunkGenerator, path_, filePrefix);
}

int VariableInfoParser::Parse()
{
    if (!variableProducer_)
    {
        ERROR("%: The chunk producer is null.", parserName_);
        return ANALYSIS_ERROR;
    }
    if (variableProducer_->ReadChunk() != ANALYSIS_OK)
    {
        ERROR("%: Read Chunk failed.", parserName_);
        return ANALYSIS_ERROR;
    }
    return ProduceData();
}

template <>
std::vector<std::shared_ptr<ParserVariableInfo>> VariableInfoParser::GetData()
{
    return variableData_;
}

int VariableInfoParser::ProduceData()
{
    if (!variableProducer_ || variableProducer_->Empty())
    {
        return ANALYSIS_OK;
    }
    while (!variableProducer_->Empty())
    {
        auto record = ReinterpretConvert<MsprofVariableInfo *>(variableProducer_->Pop());
        if (!record)
        {
            ERROR("%: Pop chunk failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (record->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM)
        {
            ERROR("%: The last %th data check failed.", parserName_, variableProducer_->Size());
            delete[] ReinterpretConvert<char *>(record);
            continue;
        }
        auto parser = std::make_shared<ParserVariableInfo>();
        parser->magicNumber = record->magicNumber;
        parser->level = record->level;
        parser->type = record->type;
        parser->threadId = record->threadId;
        parser->dataLen = record->dataLen;
        parser->timeStamp = record->timeStamp;
        if (record->dataLen > 0)
        {
            parser->data.assign(record->data, record->data + record->dataLen);
        }
        delete[] ReinterpretConvert<char *>(record);
        variableData_.emplace_back(std::move(parser));
    }
    return ANALYSIS_OK;
}

int RuntimeOpInfoParser::Parse() { return ProduceData(); }

const std::vector<RuntimeOpInfo> &RuntimeOpInfoParser::GetOpInfo() const { return runtimeOpInfoData_; }

int RuntimeOpInfoParser::ProduceData()
{
    if (ParseAdditional(unagingAdditionalPrefix_, kStaticOp) != ANALYSIS_OK)
    {
        return ANALYSIS_ERROR;
    }
    if (ParseAdditional(agingAdditionalPrefix_, kDynamicOp) != ANALYSIS_OK)
    {
        return ANALYSIS_ERROR;
    }
    return ParseVariable();
}

int RuntimeOpInfoParser::ParseAdditional(const std::vector<std::string> &filePrefix, uint16_t isDynamic)
{
    std::shared_ptr<ChunkGenerator> producer;
    MAKE_SHARED_RETURN_VALUE(producer, ChunkGenerator, ANALYSIS_ERROR, sizeof(MsprofAdditionalInfo), path_, filePrefix);
    if (producer->ReadChunk() != ANALYSIS_OK)
    {
        ERROR("%: Read additional capture_op_info failed.", parserName_);
        return ANALYSIS_ERROR;
    }
    if (producer->Empty())
    {
        return ANALYSIS_OK;
    }
    return ParseAdditionalRecords(producer.get(), parserName_, isDynamic, runtimeOpInfoData_);
}

int RuntimeOpInfoParser::ParseVariable()
{
    if (!variableProducer_)
    {
        return ANALYSIS_OK;
    }
    if (variableProducer_->ReadChunk() != ANALYSIS_OK)
    {
        ERROR("%: Read variable capture_op_info failed.", parserName_);
        return ANALYSIS_ERROR;
    }
    if (variableProducer_->Empty())
    {
        return ANALYSIS_OK;
    }
    while (!variableProducer_->Empty())
    {
        auto record = ReinterpretConvert<MsprofVariableInfo *>(variableProducer_->Pop());
        if (!record)
        {
            ERROR("%: Pop variable capture_op_info failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (record->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM)
        {
            ERROR("%: variable capture_op_info magic check failed.", parserName_);
            delete[] ReinterpretConvert<char *>(record);
            continue;
        }
        RuntimeOpInfo info;
        bool ok = AssembleRuntimeOpInfo(record->level, record->type, record->threadId, record->timeStamp,
                                        record->dataLen, record->data, kStaticOp, info);
        delete[] ReinterpretConvert<char *>(record);
        if (!ok)
        {
            continue;
        }
        runtimeOpInfoData_.emplace_back(std::move(info));
    }
    return ANALYSIS_OK;
}
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis
