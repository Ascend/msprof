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
#include "pstart_a.h"
#include "infrastructure/process/include/process_register.h"
#include "process_spy.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PstartA::ProcessEntry(DataInventory& dataInventory, const Infra::Context&)
{
    PROCESS_TRACE(PstartA);
    auto data = std::make_shared<Ps::StartA>();
    data->i = TEST_MAGIC_NUMBER;
    dataInventory.Inject(data);

    return ProcessSpy::GetResult("PstartA");
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PstartA, true);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PstartA, CHIP_V1_1_0, CHIP_V2_1_0);

}
