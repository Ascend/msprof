/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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
 * MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef ANALYSIS_DOMAIN_FTRACE_GENERIC_EVENT_PROCESSOR_H
#define ANALYSIS_DOMAIN_FTRACE_GENERIC_EVENT_PROCESSOR_H

#include <string>
#include <vector>

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ftrace_generic_event_data.h"

namespace Analysis
{
namespace Domain
{
// Converts the ftrace collector artifacts into GENERIC_EVENTS / GENERIC_EVENT_DATA rows:
// epoch ns conversion, globalTid synthesis and the field value dispatch.
class FtraceGenericEventProcessor : public DataProcessor
{
   public:
    FtraceGenericEventProcessor() = default;
    explicit FtraceGenericEventProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_FTRACE_GENERIC_EVENT_PROCESSOR_H
