/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_DOMAIN_HOST_PLATFORM_PROCESSOR_H
#define ANALYSIS_DOMAIN_HOST_PLATFORM_PROCESSOR_H

#include <string>

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/host_platform_data.h"

namespace Analysis
{
namespace Domain
{
class HostPlatformProcessor final : public DataProcessor
{
   public:
    explicit HostPlatformProcessor(const std::string& profPath) : DataProcessor(profPath) {}

   private:
    bool Process(Infra::DataInventory& dataInventory) override;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_HOST_PLATFORM_PROCESSOR_H
