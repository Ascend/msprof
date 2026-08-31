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

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_HOST_NPU_OP_MEM_DB_DUMPER_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_HOST_NPU_OP_MEM_DB_DUMPER_H

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/entities/host/include/npu_op_mem.h"
#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"

namespace Analysis
{
namespace Domain
{

using NpuOpMemRawDBData = std::vector<std::tuple<std::string, std::string, int64_t, uint64_t, uint32_t, uint64_t,
                                                 uint64_t, uint32_t, uint32_t, std::string>>;
using NpuOpMemRecordDBData = std::vector<std::tuple<std::string, uint64_t, uint64_t, uint64_t, std::string>>;
using NpuOpMemLifecycleDBData = std::vector<std::tuple<std::string, int64_t, uint64_t, uint64_t, int64_t, uint64_t,
                                                       uint64_t, uint64_t, uint64_t, std::string, std::string>>;

class NpuOpMemRawDBDumper final : public BaseDumper<NpuOpMemRawDBDumper>
{
   public:
    explicit NpuOpMemRawDBDumper(const std::string &hostFilePath);
    NpuOpMemRawDBData GenerateData(const std::vector<std::shared_ptr<ParserAdditionalInfo>> &rawData);
};

class NpuOpMemRecordDBDumper final : public BaseDumper<NpuOpMemRecordDBDumper>
{
   public:
    explicit NpuOpMemRecordDBDumper(const std::string &hostFilePath);
    NpuOpMemRecordDBData GenerateData(const std::vector<Host::NpuOpMemRecordData> &recordData);
};

class NpuOpMemLifecycleDBDumper final : public BaseDumper<NpuOpMemLifecycleDBDumper>
{
   public:
    explicit NpuOpMemLifecycleDBDumper(const std::string &hostFilePath);
    NpuOpMemLifecycleDBData GenerateData(const std::vector<Host::NpuOpMemLifecycleData> &lifecycleData);
};

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_HOST_NPU_OP_MEM_DB_DUMPER_H
