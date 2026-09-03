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
 * -------------------------------------------------------------------------
 */

#include "analysis/csrc/domain/services/parser/host/cann/rt_add_info_center.h"

#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{

using namespace Analysis::Utils;
using namespace Analysis::Infra;

void Cann::RTAddInfoCenter::Load(const std::string& path)
{
    if (runtimeOpInfoData_.empty())
    {
        LoadDB(path);
    }
    LoadCaptureInfoDB(path);
    BuildCaptureInfoTimeRange();
}

void RTAddInfoCenter::Add(const RuntimeOpInfo& info)
{
    // 仅在 EventGrouper::Group() 内单线程调用（见头文件使用场景注释），与后续读取互不重叠
    std::string key = Utils::Join("_", info.deviceId, info.streamId, info.taskId);
    runtimeOpInfoData_[key] = info;
    dumpList_.push_back(info);
    loadedFromBinary_ = true;
}

const std::unordered_map<std::string, RuntimeOpInfo>& RTAddInfoCenter::GetAll() const { return runtimeOpInfoData_; }

const std::vector<RuntimeOpInfo>& RTAddInfoCenter::GetDumpList() const { return dumpList_; }

bool RTAddInfoCenter::Empty() const { return runtimeOpInfoData_.empty(); }

bool RTAddInfoCenter::LoadedFromBinary() const { return loadedFromBinary_; }

RuntimeOpInfo RTAddInfoCenter::Get(uint16_t deviceId, uint32_t streamId, uint32_t taskId)
{
    std::string key = Utils::Join("_", deviceId, streamId, taskId);
    auto it = runtimeOpInfoData_.find(key);
    if (it != runtimeOpInfoData_.end())
    {
        return it->second;
    }
    return RuntimeOpInfo();
}

uint64_t RTAddInfoCenter::GetModelId(uint16_t deviceId, uint32_t streamId, uint32_t batchId, uint64_t timestamp)
{
    constexpr uint32_t kDefaultModelId = DEFAULT_MODEL_ID;

    CaptureKey key{deviceId, streamId, batchId};
    auto it = captureInfoTimeRangeDict_.find(key);
    if (it == captureInfoTimeRangeDict_.end()) return kDefaultModelId;

    const auto& timeRange = it->second;
    uint64_t start = std::get<0>(timeRange);
    uint64_t end = std::get<1>(timeRange);
    uint32_t model = std::get<2>(timeRange);

    return (timestamp >= start && timestamp <= end) ? model : kDefaultModelId;
}

void RTAddInfoCenter::BuildCaptureInfoTimeRange()
{
    const uint64_t kInf = std::numeric_limits<uint64_t>::max();

    for (const auto& info : captureStreamInfoData_)
    {
        CaptureKey key{info.deviceId, info.streamId, info.batchId};

        auto it = captureInfoTimeRangeDict_.find(key);
        if (it == captureInfoTimeRangeDict_.end())
        {
            it = captureInfoTimeRangeDict_.emplace(key, TimeRangeInfo{0, kInf, info.modelId}).first;
        }
        auto& timeRange = it->second;

        if (info.captureStatus == CAPTURE_STATUS_START)
        {
            std::get<0>(timeRange) = info.timeStamp;  // startTime
            std::get<2>(timeRange) = info.modelId;    // modelId
        }
        else if (info.captureStatus == CAPTURE_STATUS_END)
        {
            std::get<1>(timeRange) = info.timeStamp;  // endTime
            std::get<2>(timeRange) = info.modelId;
        }
    }
}

void RTAddInfoCenter::LoadCaptureInfoDB(const std::string& path)
{
    StreamInfoDB streamInfoDB;
    std::string hostDbDirectory = Utils::File::PathJoin({path, streamInfoDB.GetDBName()});
    DBRunner dbRunner(hostDbDirectory);
    if (!File::Exist(hostDbDirectory) || !dbRunner.CheckTableExists("CaptureStreamInfo"))
    {
        return;
    }
    std::string sql{
        "SELECT model_id, timestamp, stream_id, original_stream_id, device_id, batch_id, capture_status "
        "FROM CaptureStreamInfo"};
    using DataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint32_t, uint32_t, uint16_t, uint32_t, uint16_t>>;
    DataFormat result;
    if (!dbRunner.QueryData(sql, result))
    {
        ERROR("Query capture stream info data failed, db path is %.", hostDbDirectory);
        return;
    }

    for (const auto& row : result)
    {
        uint16_t deviceId;
        uint32_t originalStreamId;
        uint32_t batchId;
        uint16_t captureStatus;
        uint32_t streamId;
        uint64_t modelId;
        uint64_t timeStamp;
        std::tie(modelId, timeStamp, streamId, originalStreamId, deviceId, batchId, captureStatus) = row;
        CaptureStreamInfo info{modelId, timeStamp, streamId, originalStreamId, deviceId, batchId, captureStatus};
        captureStreamInfoData_.push_back(info);
    }
}

void RTAddInfoCenter::LoadDB(const std::string& path)
{
    RtsTrackDB rtsTrackDb;
    std::string hostDbDirectory = Utils::File::PathJoin({path, rtsTrackDb.GetDBName()});
    DBRunner dbRunner(hostDbDirectory);
    if (!File::Exist(hostDbDirectory) || !dbRunner.CheckTableExists("RuntimeOpInfo"))
    {
        return;
    }

    std::string sql{
        "SELECT device_id, model_id, stream_id, task_id, op_name, task_type, op_type, hash_id, "
        "block_num, mix_block_num, op_flag, is_dynamic, tensor_num, input_formats, "
        "input_data_types, input_shapes, output_formats, output_data_types, output_shapes "
        "FROM RuntimeOpInfo"};
    using dataFormat =
        std::vector<std::tuple<uint16_t, uint64_t, uint32_t, uint32_t, std::string, std::string, std::string,
                               std::string, uint16_t, uint16_t, uint16_t, std::string, uint16_t, std::string,
                               std::string, std::string, std::string, std::string, std::string>>;
    dataFormat result;
    if (!dbRunner.QueryData(sql, result))
    {
        ERROR("Query runtime op info data failed, db path is %.", hostDbDirectory);
        return;
    }

    for (auto row : result)
    {
        uint16_t deviceId, blockNum, mixBlockNum, opFlag, tensorNum;
        uint32_t streamId, taskId;
        uint64_t modelId, opNameU64, opTypeU64;
        std::string opName, taskType, opType, hashId, isDynamic, inputFormats, inputDataTypes, inputShapes,
            outputFormats, outputDataTypes, outputShapes;
        std::tie(deviceId, modelId, streamId, taskId, opName, taskType, opType, hashId, blockNum, mixBlockNum, opFlag,
                 isDynamic, tensorNum, inputFormats, inputDataTypes, inputShapes, outputFormats, outputDataTypes,
                 outputShapes) = row;
        std::string key = Utils::Join("_", deviceId, streamId, taskId);
        if (Utils::StrToU64(opNameU64, opName) != ANALYSIS_OK || Utils::StrToU64(opTypeU64, opType) != ANALYSIS_OK)
        {
            ERROR("opName hash is %, opType hash is %.", opName, opType);
            continue;
        }
        opName = HashData::GetInstance().Get(opNameU64);
        opType = HashData::GetInstance().Get(opTypeU64);
        RuntimeOpInfo info{deviceId,    taskId,        blockNum,        mixBlockNum,  opFlag,
                           tensorNum,   streamId,      modelId,         taskType,     opType,
                           opName,      hashId,        isDynamic,       inputFormats, inputDataTypes,
                           inputShapes, outputFormats, outputDataTypes, outputShapes};
        runtimeOpInfoData_[key] = info;
    }
}

}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis
