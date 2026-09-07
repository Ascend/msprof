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

#include "analysis/csrc/domain/services/persistence/host/static_op_mem_db_dumper.h"

#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"

namespace Analysis
{
namespace Domain
{
namespace
{
constexpr uint64_t NODE_INDEX_END_MAX = 4294967294ULL;
constexpr uint64_t CORRECT_NODE_INDEX_END_MAX = 4294967295ULL;
constexpr double BYTES_TO_KB = 1024.0;
const std::string TOTAL = "TOTAL";
const std::string INVALID_DYN_OP_NAME = "0";
}  // namespace

using HashData = Analysis::Domain::Host::Cann::HashData;

StaticOpMemDBDumper::StaticOpMemDBDumper(const std::string &hostFilePath)
    : BaseDumper<StaticOpMemDBDumper>(hostFilePath, "StaticOpMem")
{
    MAKE_SHARED0_NO_OPERATION(database_, Infra::StaticOpMemDB);
}

StaticOpMemDumpData StaticOpMemDBDumper::GenerateData(const StaticOpMemInfos &staticOpMemInfos)
{
    StaticOpMemDumpData data;
    if (!Utils::Reserve(data, staticOpMemInfos.size()))
    {
        return data;
    }
    for (const auto &info : staticOpMemInfos)
    {
        if (!info)
        {
            continue;
        }
        const auto &staticOpMem = info->staticOpMem;
        auto opName = staticOpMem.opName == 0 ? TOTAL : HashData::GetInstance().Get(staticOpMem.opName);
        auto modelName =
            staticOpMem.dynOpName == 0 ? INVALID_DYN_OP_NAME : HashData::GetInstance().Get(staticOpMem.dynOpName);
        auto lifeEnd = staticOpMem.lifeEnd == NODE_INDEX_END_MAX ? CORRECT_NODE_INDEX_END_MAX : staticOpMem.lifeEnd;
        auto opMemSize = static_cast<double>(staticOpMem.size) / BYTES_TO_KB;
        data.emplace_back(opName, modelName, staticOpMem.graphId, staticOpMem.lifeStart, lifeEnd, opMemSize);
    }
    return data;
}
}  // namespace Domain
}  // namespace Analysis
