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

#include "analysis/csrc/application/timeline/cann_assembler.h"
#include <algorithm>
#include <unordered_set>
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/application/timeline/connection_id_pool.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Domain::Environment;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
namespace {
const std::string CANN_ASSEMBLER = "CANN";
const std::string CANN_LABEL = "CPU";
const std::string PROCESS_API = "Api";
const std::unordered_map<uint16_t, std::string> LEVEL_MAP{
    {MSPROF_REPORT_ACL_LEVEL, "AscendCL"},
    {MSPROF_REPORT_RUNTIME_LEVEL, "Runtime"},
    {MSPROF_REPORT_MODEL_LEVEL, "Model"},
    {MSPROF_REPORT_NODE_LEVEL, "Node"},
    {MSPROF_REPORT_HCCL_NODE_LEVEL, "Communication"}
};
}

CannAssembler::CannAssembler() : JsonAssembler(PROCESS_API, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateMetaData(std::vector<ApiData> &apiData, uint32_t pid, std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::set<uint32_t> tidSet;
    std::transform(apiData.begin(), apiData.end(), std::inserter(tidSet, tidSet.end()),
                   [](ApiData &data) { return data.threadId; });
    std::shared_ptr<MetaDataNameEvent> processName;
    MAKE_SHARED_RETURN_VOID(processName, MetaDataNameEvent, pid, DEFAULT_TID, META_DATA_PROCESS_NAME, CANN_ASSEMBLER);
    std::shared_ptr<MetaDataLabelEvent> processLabel;
    MAKE_SHARED_RETURN_VOID(processLabel, MetaDataLabelEvent, pid, DEFAULT_TID, META_DATA_PROCESS_LABEL, CANN_LABEL);
    std::shared_ptr<MetaDataIndexEvent> processIndex;
    MAKE_SHARED_RETURN_VOID(processIndex, MetaDataIndexEvent, pid, DEFAULT_TID, META_DATA_PROCESS_INDEX, LAYER_CANN_SORT);
    res.push_back(processName);
    res.push_back(processLabel);
    res.push_back(processIndex);
    for (const auto &id : tidSet) {
        int tid = static_cast<int>(id);
        std::string argName{"Thread " + std::to_string(tid)};
        std::shared_ptr<MetaDataNameEvent> threadName;
        MAKE_SHARED_RETURN_VOID(threadName, MetaDataNameEvent, pid, tid, META_DATA_THREAD_NAME, argName);
        res.push_back(threadName);
        std::shared_ptr<MetaDataIndexEvent> threadIndex;
        MAKE_SHARED_RETURN_VOID(threadIndex, MetaDataIndexEvent, pid, tid, META_DATA_THREAD_INDEX, tid);
        res.push_back(threadIndex);
    }
}

std::string GetLevel(uint16_t level)
{
    for (const auto &node : API_LEVEL_TABLE) {
        if (node.second == level) {
            return node.first;
        }
    }
    return "";
}

std::string GetTraceName(uint16_t level, std::string &name)
{
    auto it = LEVEL_MAP.find(level);
    if (it != LEVEL_MAP.end()) {
        return it->second + "@" + name;
    }
    return PROCESS_API + "@" + name;
}

std::unordered_set<uint64_t> GetMemcpyAsyncConnectionIds(DataInventory& dataInventory)
{
    std::unordered_set<uint64_t> connectionIds;
    auto taskData = dataInventory.GetPtr<std::vector<AscendTaskData>>();
    if (taskData == nullptr) {
        WARN("Can't get task data from dataInventory");
        return connectionIds;
    }
    for (const auto& task: *taskData) {
        if (task.hostType == MEMCPY_ASYNC) {
            connectionIds.emplace(task.connectionId);
        }
    }
    return connectionIds;
}

void GenerateApiTrace(std::vector<ApiData> &apiData, std::vector<std::shared_ptr<TraceEvent>> &res, uint32_t pid)
{
    std::string levelStr;
    std::string traceName;
    double dur;
    for (auto &data : apiData) {
        levelStr = GetLevel(data.level);
        traceName = GetTraceName(data.level, data.apiName);
        dur = static_cast<double>(data.end - data.timestamp) / NS_TO_US;
        std::shared_ptr<ApiTraceEvent> event;
        MAKE_SHARED_RETURN_VOID(event, ApiTraceEvent, pid, data.threadId, dur,
                                DivideByPowersOfTenWithPrecision(data.timestamp), traceName, data.threadId,
                                data.connectionId, data.structType, levelStr, data.id, data.itemId);
        res.push_back(event);
    }
}

void GenerateConnectionTrace(std::vector<ApiData> &apiData, uint32_t pid, std::vector<std::shared_ptr<TraceEvent>> &res,
                             std::unordered_set<uint64_t> memcpyAsyncConnectionIds)
{
    std::string connId;
    std::string name;
    for (auto &data : apiData) {
        if (MSPROF_REPORT_NODE_LEVEL == data.level || RECORD_EVENT == data.id || WAIT_EVENT == data.id ||
            memcpyAsyncConnectionIds.find(data.connectionId) != memcpyAsyncConnectionIds.end()) {
            connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::GENERAL);
            name = HOST_TO_DEVICE + connId;
            std::shared_ptr<FlowEvent> start;
            MAKE_SHARED_RETURN_VOID(start, FlowEvent, pid, data.threadId,
                                    DivideByPowersOfTenWithPrecision(data.timestamp),
                                    HOST_TO_DEVICE, connId, name, FLOW_START);
            res.push_back(start);
        }
    }
}

uint8_t CannAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto apiData = dataInventory.GetPtr<std::vector<ApiData>>();
    if (apiData == nullptr) {
        WARN("Can't get apiData from dataInventory");
        return DATA_NOT_EXIST;
    }
    auto pid = Context::GetInstance().GetPidFromInfoJson(HOST_ID, profPath);
    auto formatPid = JsonAssembler::GetFormatPid(pid, LAYER_CANN_SORT);
    GenerateMetaData(*apiData, formatPid, res_);
    GenerateApiTrace(*apiData, res_, formatPid);
    auto memcpyAsyncConnectionIds = GetMemcpyAsyncConnectionIds(dataInventory);
    GenerateConnectionTrace(*apiData, formatPid, res_, memcpyAsyncConnectionIds);
    if (res_.empty()) {
        ERROR("Can't Generate any CANN process data");
        return ASSEMBLE_FAILED;
    }
    for (const auto &node : res_) {
        node->DumpJson(ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    return ASSEMBLE_SUCCESS;
}
}
}
