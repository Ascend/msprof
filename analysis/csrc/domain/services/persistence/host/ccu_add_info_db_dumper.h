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

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_HOST_CCU_ADD_INFO_DB_DUMPER_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_HOST_CCU_ADD_INFO_DB_DUMPER_H

#include <string>
#include <utility>

#include "analysis/csrc/domain/services/parser/host/cann/ccu_add_info_parser.h"

namespace Analysis
{
namespace Domain
{

class CcuAddInfoDBDumper final
{
   public:
    explicit CcuAddInfoDBDumper(std::string hostPath) : hostPath_(std::move(hostPath)) {}

    bool DumpData(const Host::Cann::CcuInfoData& data) const;

   private:
    std::string hostPath_;
};

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_HOST_CCU_ADD_INFO_DB_DUMPER_H
