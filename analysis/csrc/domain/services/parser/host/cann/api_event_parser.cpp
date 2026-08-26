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

#include "analysis/csrc/domain/services/parser/host/cann/api_event_parser.h"

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
std::shared_ptr<MsprofApi> CreateMsprofApi(const MsprofEvent *startEvent, const MsprofEvent *endEvent)
{
    if (!startEvent || !endEvent)
    {
        ERROR("Event data is null.");
        return nullptr;
    }
    std::shared_ptr<MsprofApi> apiData;
    MAKE_SHARED0_RETURN_VALUE(apiData, MsprofApi, nullptr);

    apiData->level = startEvent->level;
    apiData->type = startEvent->type;
    apiData->threadId = startEvent->threadId;
    apiData->reserve = startEvent->requestId;
    apiData->beginTime = startEvent->timeStamp;
    apiData->endTime = endEvent->timeStamp;
    apiData->itemId = startEvent->itemId;
    return apiData;
}

void ClearStartEventMap(
    std::map<std::tuple<uint16_t, uint32_t, uint32_t, uint32_t, uint64_t>, MsprofEvent *> &startEventMap)
{
    if (startEventMap.empty())
    {
        return;
    }
    ERROR("There is remaining start event.");
    for (auto &startEvent : startEventMap)
    {
        delete startEvent.second;
        startEvent.second = nullptr;
    }
    startEventMap.clear();
}

}  // namespace

ApiEventParser::ApiEventParser(const std::string &path) : BaseParser(path, "ApiEventParser")
{
    MAKE_SHARED_NO_OPERATION(chunkProducer_, ChunkGenerator, sizeof(MsprofApi), path_, filePrefix_);
}

template <>
std::vector<std::shared_ptr<ParserApi>> ApiEventParser::GetData()
{
    return apiData_;
}

template <>
std::vector<std::shared_ptr<ParserEvent>> ApiEventParser::GetData()
{
    return eventData_;
}

int ApiEventParser::ProduceData()
{
    if (chunkProducer_->Empty())
    {
        return ANALYSIS_OK;
    }
    std::map<std::tuple<uint16_t, uint32_t, uint32_t, uint32_t, uint64_t>, MsprofEvent *> startEventMap;
    if (!Reserve(apiData_, chunkProducer_->Size()))
    {
        ERROR("%: Reserve data failed", parserName_);
        return ANALYSIS_ERROR;
    }
    while (!chunkProducer_->Empty())
    {
        auto event = ReinterpretConvert<MsprofEvent *>(chunkProducer_->Pop());
        if (!event)
        {
            ERROR("%: Pop chunk failed.", parserName_);
            ClearStartEventMap(startEventMap);
            return ANALYSIS_ERROR;
        }
        if (event->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM)
        {
            ERROR("%: The last %th data check failed.", parserName_, chunkProducer_->Size());
            delete event;
            continue;
        }
        if (event->reserve != MSPROF_EVENT_FLAG)
        {
            // api data
            auto parser = std::make_shared<ParserApi>();
            if (!ParserApiAdapter::AdapterApi(ReinterpretConvert<MsprofApi *>(event), parser.get()))
            {
                ERROR("%: copy api data data failed.", parserName_);
                delete event;
                return ANALYSIS_ERROR;
            }
            apiData_.emplace_back(parser);
            delete event;
            continue;
        }
        // event data，根据level, type, threadId, requestId, itemId合并start event和end event
        auto key = std::make_tuple(event->level, event->type, event->threadId, event->requestId, event->itemId);
        auto iter = startEventMap.find(key);
        if (iter == startEventMap.end())
        {
            startEventMap[key] = event;
            continue;
        }
        auto startEvent = iter->second;
        auto apiData = CreateMsprofApi(startEvent, event);
        startEventMap.erase(iter);
        delete startEvent;
        delete event;
        if (!apiData)
        {
            ERROR("Api data is null.");
            ClearStartEventMap(startEventMap);
            return ANALYSIS_ERROR;
        }
        auto parser = std::make_shared<ParserApi>();
        if (!ParserApiAdapter::AdapterApi(apiData.get(), parser.get()))
        {
            ERROR("%: copy api data data failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        apiData_.emplace_back(parser);
    }
    ClearStartEventMap(startEventMap);
    return ANALYSIS_OK;
}
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis
