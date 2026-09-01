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

#ifndef ANALYSIS_APPLICATION_HOST_USAGE_ASSEMBLER_H
#define ANALYSIS_APPLICATION_HOST_USAGE_ASSEMBLER_H

#include <vector>

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/host_usage_data.h"

namespace Analysis
{
namespace Application
{
class HostUsageAssembler : public JsonAssembler
{
   public:
    HostUsageAssembler(const std::string &assembleName);

   protected:
    std::vector<std::shared_ptr<TraceEvent>> res_;

   private:
    uint8_t AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    virtual uint8_t GenerateDataTrace(DataInventory &dataInventory, uint32_t pidMap) = 0;
};

// 处理host cpu usage数据
class CpuUsageAssembler : public HostUsageAssembler
{
   public:
    CpuUsageAssembler();

   private:
    uint8_t GenerateDataTrace(DataInventory &dataInventory, uint32_t pidMap) override;
};

// 处理host cpu freq数据
class CpuFreqAssembler : public HostUsageAssembler
{
   public:
    CpuFreqAssembler();

   private:
    uint8_t GenerateDataTrace(DataInventory &dataInventory, uint32_t pidMap) override;
};

// 处理host memory usage数据
class MemUsageAssembler : public HostUsageAssembler
{
   public:
    MemUsageAssembler();

   private:
    uint8_t GenerateDataTrace(DataInventory &dataInventory, uint32_t pidMap) override;
};

// 处理host disk usage数据
class DiskUsageAssembler : public HostUsageAssembler
{
   public:
    DiskUsageAssembler();

   private:
    uint8_t GenerateDataTrace(DataInventory &dataInventory, uint32_t pidMap) override;
};

// 处理host network usage数据
class NetworkUsageAssembler : public HostUsageAssembler
{
   public:
    NetworkUsageAssembler();

   private:
    uint8_t GenerateDataTrace(DataInventory &dataInventory, uint32_t pidMap) override;
};

// 处理os runtime api数据
class OSRuntimeApiAssembler : public HostUsageAssembler
{
   public:
    OSRuntimeApiAssembler();

   private:
    uint8_t GenerateDataTrace(DataInventory &dataInventory, uint32_t pidMap) override;
};

}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_HOST_USAGE_ASSEMBLER_H
