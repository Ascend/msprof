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

#ifndef ANALYSIS_APPLICATION_DATABASE_MSPROF_DB_H
#define ANALYSIS_APPLICATION_DATABASE_MSPROF_DB_H

#include "analysis/csrc/infrastructure/db/include/database.h"

namespace Analysis
{
namespace Application
{
using namespace Infra;

class MsprofDB : public Infra::Database
{
   public:
    MsprofDB();
};

}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_DATABASE_MSPROF_DB_H
