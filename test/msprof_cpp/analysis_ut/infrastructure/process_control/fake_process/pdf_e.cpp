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
#include "pdf_e.h"
#include "infrastructure/process/include/process_register.h"
#include "pah_d.h"
#include "pg_f.h"
#include "process_spy.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PdfE::ProcessEntry(DataInventory& dataInventory, const Infra::Context&)
{
    PROCESS_TRACE(PdfE);
    auto d = dataInventory.GetPtr<Ps::StructD>();
    auto f = dataInventory.GetPtr<Ps::StructF>();
    if (!d || !f) { // 这里是测试，框架应该在进入本流程时，不应该释放这个内存
        return 1;
    }

    return ProcessSpy::GetResult("PdfE");
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PdfE, true, Ps::PahD, Ps::PgF);
REGISTER_PROCESS_DEPENDENT_DATA(Ps::PdfE, Ps::StructD, Ps::StructF);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PdfE, CHIP_ID_ALL);

}
