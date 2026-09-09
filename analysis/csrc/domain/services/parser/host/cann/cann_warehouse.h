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

#ifndef ANALYSIS_PARSER_HOST_CANN_CANN_WAREHOUSE_H
#define ANALYSIS_PARSER_HOST_CANN_CANN_WAREHOUSE_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/domain/entities/tree/include/event.h"
#include "analysis/csrc/domain/entities/tree/include/event_queue.h"
#include "analysis/csrc/domain/services/parser/host/cann/ccu_add_info_parser.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{

using EventQueue = Analysis::Domain::EventQueue;

// CANN数据仓，用于存储各个Type的Events
struct CANNWarehouse
{
    // API Type只在Model、Node、HCCL Level的Event
    std::shared_ptr<EventQueue> kernelEvents = nullptr;
    std::shared_ptr<EventQueue> graphIdMapEvents = nullptr;
    std::shared_ptr<EventQueue> fusionOpInfoEvents = nullptr;
    std::shared_ptr<EventQueue> nodeBasicInfoEvents = nullptr;
    std::shared_ptr<EventQueue> nodeAttrInfoEvents = nullptr;
    std::shared_ptr<EventQueue> tensorInfoEvents = nullptr;
    std::shared_ptr<EventQueue> contextIdEvents = nullptr;
    std::shared_ptr<EventQueue> hcclInfoEvents = nullptr;
    std::shared_ptr<EventQueue> taskTrackEvents = nullptr;
    std::shared_ptr<EventQueue> hcclOpInfoEvents = nullptr;
};

// 不建树、只解析后落盘的数据仓
struct CANNDumpWarehouse
{
    std::vector<std::shared_ptr<ParserCompactInfo>> dpuTrackData;
    std::vector<std::shared_ptr<ParserCompactInfo>> captureStreamInfoData;
    std::vector<std::shared_ptr<ParserCompactInfo>> memcpyInfoData;
    std::vector<std::shared_ptr<ParserCompactInfo>> streamExpandSpecData;
    std::vector<std::shared_ptr<ParserAdditionalInfo>> mc2CommInfoData;
    std::vector<std::shared_ptr<ParserAdditionalInfo>> graphIdMapData;
    std::vector<std::shared_ptr<ParserAdditionalInfo>> staticOpMemData;
    std::vector<std::shared_ptr<CcuInfoData>> ccuInfoData;
};

}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_PARSER_HOST_CANN_CANN_WAREHOUSE_H
