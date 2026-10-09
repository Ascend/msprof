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
#include "pab_g.h"
#include "infrastructure/process/include/process_register.h"
#include "pstart_a.h"
#include "pd_b.h"
#include "process_spy.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PabG::ProcessEntry(DataInventory& dataInventory, const Infra::Context&)
{
    PROCESS_TRACE(PabG);
    CommonFun();
    auto a = dataInventory.GetPtr<Ps::StartA>();
    if (!a || a->i != TEST_MAGIC_NUMBER) {
        return 1;
    }
    auto b = dataInventory.GetPtr<Ps::StructB4g>();
    if (!b || b->testStr != "testStr") {
        return 1;
    }

    auto g = std::make_shared<StructG>();
    dataInventory.Inject(g);

    return ProcessSpy::GetResult("PabG");
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PabG, true, Ps::PstartA, Ps::PdB);
REGISTER_PROCESS_DEPENDENT_DATA(Ps::PabG, Ps::StartA, Ps::StructB4g);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PabG, CHIP_ID_ALL);

}
