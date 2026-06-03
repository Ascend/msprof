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

#ifndef ANALYSIS_VIEWER_DATABASE_DRAFTS_NUMBER_MAPPING_H
#define ANALYSIS_VIEWER_DATABASE_DRAFTS_NUMBER_MAPPING_H

#include <string>
#include <stdint.h>

namespace Analysis {
namespace Domain {
class NumberMapping {
public:
    // 对象类型
    enum class MappingType {
        GE_DATA_TYPE = 0,
        GE_FORMAT,
        GE_TASK_TYPE,
        HCCL_DATA_TYPE,
        HCCL_LINK_TYPE,
        HCCL_TRANSPORT_TYPE,
        HCCL_RDMA_TYPE,
        HCCL_OP_TYPE,
        LEVEL,
        ACL_API_TAG,
        HCCL_ALG_TYPE
    };
    static std::string Get(MappingType type, uint32_t key);
};

} // Domain
} // Analysis
#endif // ANALYSIS_VIEWER_DATABASE_DRAFTS_NUMBER_MAPPING_H
