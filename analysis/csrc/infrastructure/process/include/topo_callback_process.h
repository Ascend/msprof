/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_INFRASTRUCTURE_PROCESS_TOPO_CALLBACK_PROCESS_H
#define ANALYSIS_INFRASTRUCTURE_PROCESS_TOPO_CALLBACK_PROCESS_H

#include <functional>
#include <utility>

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/process/include/topo_graph.h"

namespace Analysis
{
namespace Application
{
using TopoProcessCallback = std::function<bool(Infra::DataInventory&)>;

class TopoCallbackProcess final : public Infra::Process
{
   public:
    explicit TopoCallbackProcess(TopoProcessCallback callback) : callback_(std::move(callback)) {}

   private:
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context&) override
    {
        return callback_ != nullptr && callback_(dataInventory) ? ANALYSIS_OK : ANALYSIS_ERROR;
    }

    TopoProcessCallback callback_;
};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_INFRASTRUCTURE_PROCESS_TOPO_CALLBACK_PROCESS_H
