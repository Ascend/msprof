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

#ifndef ANALYSIS_INFRA_RESOURCE_BINARY_STRUCT_INFO_H
#define ANALYSIS_INFRA_RESOURCE_BINARY_STRUCT_INFO_H

namespace Analysis
{
const int STARS_SOC_STRUCT_SIZE = 64;
const int STARS_SOC_STRUCT_SIZE_V6 = 32;
const int STARS_SOC_PROFILE_STRUCT_SIZE = 64;
const int FFTS_PROFILE_STRUCT_SIZE = 128;
const int TS_TRACK_STRUCT_SIZE = 40;
const int FREQ_STRUCT_SIZE = 888;
const int QOS_STRUCT_SIZE = 64;
const int UB_STRUCT_SIZE = 128;
const int NPU_MEM_STRUCT_SIZE = 32;
const int NPU_MODULE_MEM_STRUCT_SIZE = 24;
const int HBM_STRUCT_SIZE_V1 = 24;
const int HBM_STRUCT_SIZE_V2 = 32;
}  // namespace Analysis
#endif  // ANALYSIS_INFRA_RESOURCE_BINARY_STRUCT_INFO_H
