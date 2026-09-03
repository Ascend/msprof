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

#include "analysis/csrc/domain/entities/tree/include/tree.h"

#include <queue>
#include <string>

#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{

namespace
{
// EventType类型对应字符串的映射关系
// EventType和EventTypeString一一对应
std::vector<std::string> EventTypeString{"Api",           "Event",        "NodeBasicInfo", "NodeAttrInfo",
                                         "TensorInfo",    "HcclInfo",     "ContextId",     "GraphIdMap",
                                         "FusionOpInfo",  "TaskTrack",    "HcclOpInfo",    "MemoryCopy",
                                         "RuntimeOpInfo", "DpuTaskTrack", "Dummy",         "Invalid"};
}  // namespace

std::shared_ptr<TreeNode> Tree::GetRoot() const { return root_; }

std::string Tree::GetTreeLevelStr(const std::shared_ptr<TreeNode> &node) const
{
    std::string lstr;
    for (const auto &r : node->records)
    {
        lstr += "[" + EventTypeString[static_cast<unsigned long>(r->info.type)] +
                Utils::Join("_", r->info.start, r->info.end) + "] ";
    }
    return lstr;
}

std::vector<std::string> Tree::Show()
{
    if (!root_)
    {
        return {};
    }
    std::vector<std::string> levelStr;
    std::queue<std::shared_ptr<TreeNode>> q;
    q.push(root_);
    while (!q.empty())
    {
        auto levelSize = q.size();
        std::string lstr{};
        for (size_t i = 0; i < levelSize; ++i)
        {
            std::shared_ptr<TreeNode> node = q.front();
            q.pop();
            lstr += EventTypeString[static_cast<unsigned long>(node->event->info.type)] +
                    Utils::Join("_", node->event->info.start, node->event->info.end) + " ";
            lstr += GetTreeLevelStr(node);
            for (const auto &child : node->children)
            {
                q.push(child);
            }
        }
        levelStr.emplace_back(lstr);
    }
    return levelStr;
}

}  // namespace Domain
}  // namespace Analysis
