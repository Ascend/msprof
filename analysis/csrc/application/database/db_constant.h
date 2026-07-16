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

#ifndef ANALYSIS_APPLICATION_DB_CONSTANT_H
#define ANALYSIS_APPLICATION_DB_CONSTANT_H

#include <cstdint>
#include <string>

#include "analysis/csrc/infrastructure/utils/utils.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Viewer::Database;
const std::unordered_map<std::string, std::unordered_map<std::string, uint16_t>> ENUM_TABLE = {
    {TABLE_NAME_ENUM_API_TYPE, API_LEVEL_TABLE},
    {TABLE_NAME_ENUM_MODULE, MODULE_NAME_TABLE},
    {TABLE_NAME_ENUM_HCCL_DATA_TYPE, HCCL_DATA_TYPE_TABLE},
    {TABLE_NAME_ENUM_HCCL_LINK_TYPE, HCCL_LINK_TYPE_TABLE},
    {TABLE_NAME_ENUM_HCCL_TRANSPORT_TYPE, HCCL_TRANSPORT_TYPE_TABLE},
    {TABLE_NAME_ENUM_HCCL_RDMA_TYPE, HCCL_RDMA_TYPE_TABLE},
    {TABLE_NAME_MSTX_EVENT_TYPE, MSTX_EVENT_TYPE_TABLE},
    {TABLE_NAME_ENUM_MEMCPY_OPERATION, MEMCPY_OPERATION_TABLE},
    {TABLE_NAME_ENUM_OVERLAP_ANALYSIS_TYPE, OVERLAP_TYPE_TABLE}};

const std::string SCHEMA_VERSION_MAJOR = "1";
const std::string SCHEMA_VERSION_MINOR = "2";
const std::string SCHEMA_VERSION_MICRO = "0";
const std::string SCHEMA_VERSION = Utils::Join(".", SCHEMA_VERSION_MAJOR, SCHEMA_VERSION_MINOR, SCHEMA_VERSION_MICRO);
// 版本号相关 SCHEMA_VERSION
// SCHEMA_VERSION_MAJOR，对于数据库的整体格式，只有在整体db格式发生重大重写或重构时，进行更改
// SCHEMA_VERSION_MINOR，表示存在非兼容性修改，需要做适配。例如，当表名或字段类型发生变动时。
// SCHEMA_VERSION_MICRO，表示版本变动，每次变动代表支持兼容性新特性，对原有功能不存在修改，不存在影响和中断原有脚本和数据读取
const std::unordered_map<std::string, std::string> META_DATA = {{NAME_STR(SCHEMA_VERSION), SCHEMA_VERSION},
                                                                {NAME_STR(SCHEMA_VERSION_MAJOR), SCHEMA_VERSION_MAJOR},
                                                                {NAME_STR(SCHEMA_VERSION_MINOR), SCHEMA_VERSION_MINOR},
                                                                {NAME_STR(SCHEMA_VERSION_MICRO), SCHEMA_VERSION_MICRO}};

const std::unordered_map<uint16_t, std::string> CHIP_TABLE = {
    {0, "Ascend310"}, {1, "Ascend910A"}, {4, "Ascend310P"}, {5, "Ascend910B"}, {7, "Ascend310B"}, {15, "Ascend950"},
};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_DB_CONSTANT_H
