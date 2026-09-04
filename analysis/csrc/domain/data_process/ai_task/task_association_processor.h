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

#ifndef ANALYSIS_DOMAIN_TASK_ASSOCIATION_PROCESSOR_H
#define ANALYSIS_DOMAIN_TASK_ASSOCIATION_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"

namespace Analysis
{
namespace Domain
{
class TaskAssociationProcessor : public DataProcessor
{
   public:
    TaskAssociationProcessor() = default;
    explicit TaskAssociationProcessor(const std::string &profPaths);

   private:
    bool Process(DataInventory &dataInventory) override;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_TASK_ASSOCIATION_PROCESSOR_H
