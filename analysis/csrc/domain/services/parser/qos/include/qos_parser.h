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

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_QOS_QOS_PARSER_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_QOS_QOS_PARSER_H

#include <functional>

#include "analysis/csrc/domain/entities/hal/include/hal_qos.h"
#include "analysis/csrc/domain/services/parser/parser.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis
{
namespace Domain
{
class QosParserBase : public Parser
{
   private:
    uint32_t GetTrunkSize() override;
    uint32_t ParseData(Infra::DataInventory &dataInventory, const Infra::Context &context) override;
    bool ParseDataItem(uint8_t *binaryData,
                       const std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> &parserItem,
                       const Utils::SyscntConversionParams &timeParams, HalQosBwData &data) const;

    virtual uint32_t GetParserItemType() const = 0;
    virtual bool IsDataItemValid(const uint8_t *binaryData) const { return true; }
};

class QosParser final : public QosParserBase
{
   private:
    std::vector<std::string> GetFilePattern() override;
    uint32_t GetParserItemType() const override;

    std::vector<std::string> filePrefix_{"qos.data."};
};

class StarsQosParser final : public QosParserBase
{
   private:
    std::vector<std::string> GetFilePattern() override;
    uint32_t GetParserItemType() const override;
    bool IsDataItemValid(const uint8_t *binaryData) const override;

    std::vector<std::string> filePrefix_{"stars_soc_profile.data."};
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PARSER_QOS_QOS_PARSER_H
