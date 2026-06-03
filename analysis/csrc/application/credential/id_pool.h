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

#ifndef ANALYSIS_ASSOCIATION_CREDENTIAL_ID_POOL_H
#define ANALYSIS_ASSOCIATION_CREDENTIAL_ID_POOL_H

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include "analysis/csrc/infrastructure/utils/singleton.h"

namespace Analysis
{
namespace Application
{
namespace Credential
{
using CorrelationTuple = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>;
class IdPool : public Utils::Singleton<IdPool>
{
   public:
    void Clear();
    uint64_t GetId(const CorrelationTuple& key);
    uint64_t GetUint64Id(const std::string& key);
    uint32_t GetUint32Id(const std::string& key);
    std::unordered_map<std::string, uint64_t>& GetAllUint64Ids();

   private:
    uint64_t correlationIndex_ = 0;
    uint64_t uint64Index_ = 0;
    uint32_t uint32Index_ = 0;
    std::mutex correlationMutex_;
    std::mutex uint64Mutex_;
    std::mutex uint32Mutex_;
    std::unordered_map<std::string, uint64_t> correlationIds_;
    std::unordered_map<std::string, uint64_t> uint64Ids_;
    std::unordered_map<std::string, uint32_t> uint32Ids_;
};

}  // namespace Credential
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_ASSOCIATION_CREDENTIAL_ID_POOL_H
