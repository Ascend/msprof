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
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_DEVICE_UB_PERSISTENCE_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_DEVICE_UB_PERSISTENCE_H

#include <cstdint>
#include <tuple>

#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis
{
namespace Domain
{
using UbBwRow = std::tuple<uint32_t, uint16_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                           uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>;

class UbPersistence : public Infra::Process
{
   private:
    uint32_t ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context) override;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_DEVICE_UB_PERSISTENCE_H
