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
#include "pah_d.h"
#include "infrastructure/process/include/process_register.h"
#include "pstart_a.h"
#include "pstart_h.h"
#include "process_spy.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PahD::ProcessEntry(DataInventory& dataInventory, const Infra::Context&)
{
    PROCESS_TRACE(PahD);
    auto a = dataInventory.GetPtr<Ps::StartA>();
    auto h = dataInventory.GetPtr<Ps::StartH>();  // H流程在chipv2时不生成，因此下面不判断h
    if (!a) {
        return 1;
    }
    if (a->i != TEST_MAGIC_NUMBER) {
        return 1;
    }
    auto d = std::make_shared<StructD>();
    dataInventory.Inject(d);

    return ProcessSpy::GetResult("PahD");
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PahD, true, Ps::PstartA, Ps::PstartH);
REGISTER_PROCESS_DEPENDENT_DATA(Ps::PahD, Ps::StartA, Ps::StartH);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PahD, CHIP_V1_1_0, CHIP_V2_1_0);

}
