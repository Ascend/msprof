/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_NPU_MODULE_MEM_PERSISTENCE_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_NPU_MODULE_MEM_PERSISTENCE_H

#include <string>
#include <tuple>

#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis
{
namespace Domain
{
using NpuModuleMemRow = std::tuple<uint32_t, uint64_t, int64_t, std::string>;

class NpuModuleMemPersistence final : public Infra::Process
{
   private:
    uint32_t ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context) override;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_NPU_MODULE_MEM_PERSISTENCE_H
