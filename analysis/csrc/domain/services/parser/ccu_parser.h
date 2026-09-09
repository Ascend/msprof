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

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_CCU_PARSER_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_CCU_PARSER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis
{
namespace Domain
{

struct CcuMissionRecord
{
    uint32_t streamId;
    uint32_t taskId;
    uint16_t lpInstrId;
    uint64_t lpStartTime;
    uint64_t lpEndTime;
    uint16_t setckeBitInstrId;
    uint64_t setckeBitStartTime;
    uint32_t relId;
    uint64_t relEndTime;
};

struct CcuChannelRecord
{
    uint32_t channelId;
    uint64_t timestamp;
    uint32_t maxBw;
    uint32_t minBw;
    uint32_t avgBw;
};

struct CcuMissionDataSet
{
    std::vector<CcuMissionRecord> records;
    std::vector<std::string> completedFiles;
    bool hasCompletedFiles = false;
};

struct CcuChannelDataSet
{
    std::vector<CcuChannelRecord> records;
    std::vector<std::string> completedFiles;
    bool hasCompletedFiles = false;
};

class CcuMissionParser final : public Infra::Process
{
   private:
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context) override;
    static bool DecodeV6_1(const uint8_t* data, size_t size, std::vector<CcuMissionRecord>& records);
};

class CcuChannelParser final : public Infra::Process
{
   private:
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context) override;
    static bool DecodeV6_1(const uint8_t* data, size_t size, std::vector<CcuChannelRecord>& records);
};

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PARSER_CCU_PARSER_H
