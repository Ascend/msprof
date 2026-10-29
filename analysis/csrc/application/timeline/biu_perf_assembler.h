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
#ifndef ANALYSIS_BIU_PERF_ASSEMBLER_H
#define ANALYSIS_BIU_PERF_ASSEMBLER_H

#include <utility>

#include "analysis/csrc/application/timeline/json_assembler.h"

namespace Analysis
{
namespace Application
{

class BiuPerfEvent : public DurationEvent
{
   public:
    BiuPerfEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name,
                 std::string coreType, uint16_t blockId)
        : DurationEvent(pid, tid, dur, ts, name), coreType(std::move(coreType)), blockId(blockId)
    {
    }

   private:
    std::string coreType;
    uint16_t blockId;
    void ProcessArgs(JsonWriter &ostream) override
    {
        ostream["Core Type"] << coreType;
        ostream["Block Id"] << blockId;
    }
};

class BiuPerfAssembler : public JsonAssembler
{
   public:
    BiuPerfAssembler();

   private:
    uint8_t AssembleData(DataInventory &dataInventory, const std::string &profPath) override;

   private:
};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_BIU_PERF_ASSEMBLER_H
