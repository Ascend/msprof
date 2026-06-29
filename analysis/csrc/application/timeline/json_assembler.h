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

#ifndef ANALYSIS_APPLICATION_JSON_ASSEMBLER_H
#define ANALYSIS_APPLICATION_JSON_ASSEMBLER_H

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/domain/entities/json_trace/include/meta_data_event.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/dump_tools/include/dump_tool.h"
#include "analysis/csrc/infrastructure/dump_tools/json_tool/include/json_writer.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain;
using namespace Analysis::Infra;

class JsonAssembler
{
   public:
    JsonAssembler() = default;
    JsonAssembler(const std::string &name, std::unordered_map<std::string, FileCategory> &&files);
    virtual ~JsonAssembler() = default;
    bool Run(DataInventory &dataInventory, const std::string &profPath);

   protected:
    uint32_t GetDevicePid(std::unordered_map<uint16_t, uint32_t> &pidMap, uint16_t deviceId,
                          const std::string &profPath, uint32_t index);
    void GenerateHWMetaData(const std::unordered_map<uint16_t, uint32_t> &pidMap, const struct LayerInfo &layerInfo,
                            std::vector<std::shared_ptr<TraceEvent>> &res);
    void GenerateTaskMetaData(const std::unordered_map<uint16_t, uint32_t> &pidMap, const struct LayerInfo &layer,
                              std::vector<std::shared_ptr<TraceEvent>> &res,
                              std::set<std::pair<uint32_t, int>> &pidTidSet);
    static uint32_t GetFormatPid(uint32_t pid, uint32_t index, uint32_t deviceId = HOST_PID);
    static uint32_t GetDeviceIdFromPid(uint32_t pid);
    static std::string GetLayerInfoLabelWithDeviceId(const std::string &label, uint32_t pid);

   protected:
    std::string processorName_;
    std::unordered_map<std::string, FileCategory> fileMap_;

   private:
    bool FlushToFile(JsonWriter &ostream, const std::string &profPath);
    virtual uint8_t AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath) = 0;
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_JSON_ASSEMBLER_H
