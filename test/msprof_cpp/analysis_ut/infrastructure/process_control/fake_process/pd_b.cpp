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
#include "pd_b.h"
#include "infrastructure/process/include/process_register.h"
#include "pah_d.h"
#include "pstart_a.h"  // 包含这个头文件是为了测试，实际写代码的时候，请不要包含自己不需要的头文件
#include "process_spy.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PdB::ProcessEntry(DataInventory& dataInventory, const Infra::Context&)
{
    PROCESS_TRACE(PdB);
    auto a = dataInventory.GetPtr<Ps::StartA>();
    if (!a) { // 这里是测试，框架应该在进入本流程时，不应该释放这个内存
        return 1;
    }
    auto d4c = std::make_shared<StructB4c>();
    d4c->testCh = "testCh";
    dataInventory.Inject(d4c);
    auto d4g = std::make_shared<StructB4g>();
    d4g->testStr = "testStr";
    dataInventory.Inject(d4g);

    return ProcessSpy::GetResult("PdB");
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PdB, true, Ps::PahD);
REGISTER_PROCESS_DEPENDENT_DATA(Ps::PdB, Ps::StructD);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PdB, CHIP_ID_ALL);

}
