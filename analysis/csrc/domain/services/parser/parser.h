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

#ifndef MSPROF_ANALYSIS_PARSER_H
#define MSPROF_ANALYSIS_PARSER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis
{
namespace Domain
{

class Parser : public Infra::Process
{
   protected:
    std::unique_ptr<uint8_t[]> binaryData;
    uint64_t binaryDataSize;
    virtual uint32_t Prepare(const DeviceContext &) { return ANALYSIS_OK; }

   private:
    uint32_t ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context) override;

    uint32_t GetFileSize(const char *filePath) const;

    uint64_t GetFilesSize(const std::vector<std::string> &paths);

    uint32_t ReadData(const std::vector<std::string> &files, size_t firstFileOffset);

    uint32_t ReadDataEntry(const DeviceContext &deviceContext);

    // 获取文件路径
    std::string GetFilePath(const DeviceContext &deviceContext);

    virtual std::vector<std::string> GetFilePattern() = 0;

    virtual uint32_t GetTrunkSize() = 0;

    virtual uint32_t ParseData(Infra::DataInventory &dataInventory, const Infra::Context &context) = 0;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // MSPROF_ANALYSIS_PARSER_H
