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
#include "pab_c.h"
#include "infrastructure/process/include/process_register.h"
#include "pstart_a.h"
#include "pd_b.h"
#include "process_spy.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PabC::ProcessEntry(DataInventory& dataInventory, const Infra::Context& context)
{
    PROCESS_TRACE(PabC);
    CommonFun();
    auto a = dataInventory.GetPtr<Ps::StartA>();
    if (!a || a->i != TEST_MAGIC_NUMBER) {
        return 1;
    }
    auto b = dataInventory.GetPtr<Ps::StructB4c>();
    if (!b) {
        return 1;
    }

    auto c = std::make_shared<StructC>();
    dataInventory.Inject(c);

    return ProcessSpy::GetResult("PabC");
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PabC, true, Ps::PstartA, Ps::PdB);
REGISTER_PROCESS_DEPENDENT_DATA(Ps::PabC, Ps::StartA, Ps::StructB4c);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PabC, CHIP_V1_1_0);

}