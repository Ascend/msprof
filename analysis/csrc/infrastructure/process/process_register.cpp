/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include "analysis/csrc/infrastructure/process/include/process_register.h"

namespace Analysis
{

namespace Infra
{

ProcessRegister::ProcessRegister(std::type_index selfType, ProcessCreator creator, bool mandatory,
                                 const char* processName, std::vector<std::type_index>&& preProcessType)
{
    GetContainer()[selfType].creator = creator;
    GetContainer()[selfType].mandatory = mandatory;
    GetContainer()[selfType].processDependence = preProcessType;
    GetContainer()[selfType].processName = processName;
}

ProcessRegister::ProcessRegister(std::type_index selfType, std::vector<std::type_index>&& paramTypes)
{
    GetContainer()[selfType].paramTypes = paramTypes;
}

ProcessRegister::ProcessRegister(std::type_index selfType, std::initializer_list<uint32_t> chipIds)
{
    GetContainer()[selfType].chipIds = chipIds;
}

ProcessCollection& ProcessRegister::GetContainer()
{
    static ProcessCollection container;
    return container;
}

ProcessCollection ProcessRegister::CopyProcessInfo()
{
    // 注册信息数据量小，这里COPY一份
    return GetContainer();
}

}  // namespace Infra

}  // namespace Analysis
