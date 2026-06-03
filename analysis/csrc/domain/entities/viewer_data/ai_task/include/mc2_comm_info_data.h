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

#ifndef ANALYSIS_DOMAIN_MC2_COMM_INFO_H
#define ANALYSIS_DOMAIN_MC2_COMM_INFO_H

#include <stdint.h>

namespace Analysis {
namespace Domain {
struct MC2CommInfoData {
    uint16_t aiCpuKfcStreamId = UINT16_MAX;   // 通信大算子所在stream
    std::string commStreamIds;    // 通信小算子所在stream
};
}
}

#endif // ANALYSIS_DOMAIN_MC2_COMM_INFO_H
