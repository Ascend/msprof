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

#include "analysis/csrc/domain/services/persistence/host/runtime_op_info_dumper.h"

#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
RuntimeOpInfoDumper::RuntimeOpInfoDumper(const std::string &hostFilePath)
    : BaseDumper<RuntimeOpInfoDumper>(hostFilePath, "RuntimeOpInfo")
{
    MAKE_SHARED0_NO_OPERATION(database_, RtsTrackDB);
}

RuntimeOpInfoDumpData RuntimeOpInfoDumper::GenerateData(const std::vector<RuntimeOpInfo> &opInfoList)
{
    RuntimeOpInfoDumpData data;
    if (!Utils::Reserve(data, opInfoList.size()))
    {
        ERROR("Can't reserve RuntimeOpInfo dump vector");
        return data;
    }
    for (const auto &info : opInfoList)
    {
        data.emplace_back(info.level, info.structType, info.threadId, info.timeStamp, info.deviceId, info.modelId,
                          info.streamId, info.taskId, info.opName, info.taskType, info.opType, info.hashId,
                          info.blockNum, info.mixBlockNum, info.opFlag, info.isDynamic, info.tensorNum,
                          info.inputFormats, info.inputDataTypes, info.inputShapes, info.outputFormats,
                          info.outputDataTypes, info.outputShapes);
    }
    return data;
}
}  // namespace Domain
}  // namespace Analysis
