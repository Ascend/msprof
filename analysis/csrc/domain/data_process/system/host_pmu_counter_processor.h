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
#ifndef ANALYSIS_DOMAIN_HOST_PMU_COUNTER_PROCESSOR_H
#define ANALYSIS_DOMAIN_HOST_PMU_COUNTER_PROCESSOR_H

#include <string>

#include "analysis/csrc/domain/data_process/data_processor.h"

namespace Analysis
{
namespace Domain
{
class HostPmuCounterProcessor : public DataProcessor
{
   public:
    HostPmuCounterProcessor() = default;
    explicit HostPmuCounterProcessor(const std::string &profPath);

   private:
    bool Process(DataInventory &dataInventory) override;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_HOST_PMU_COUNTER_PROCESSOR_H
