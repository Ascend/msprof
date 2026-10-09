/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */
#include "pg_f.h"
#include "infrastructure/process/include/process_register.h"
#include "pab_g.h"
#include "pab_c.h"
#include "process_spy.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PgF::ProcessEntry(DataInventory& dataInventory, const Infra::Context&)
{
    PROCESS_TRACE(PgF);
    auto a = dataInventory.GetPtr<Ps::StructG>();
    auto c = dataInventory.GetPtr<Ps::StructC>();
    if (!a) { // 这里不能强判断c，c是可选流程，在chipv2场景不运行
        return 1;
    }
    auto f = std::make_shared<StructF>();
    dataInventory.Inject(f);

    return ProcessSpy::GetResult("PgF");
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PgF, true, Ps::PabG, Ps::PabC);
REGISTER_PROCESS_DEPENDENT_DATA(Ps::PgF, Ps::StructG, Ps::StructC);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PgF, CHIP_ID_ALL);

}
