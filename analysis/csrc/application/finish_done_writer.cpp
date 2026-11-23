/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include "analysis/csrc/application/include/finish_done_writer.h"

#include <fstream>
#include <ios>
#include <mutex>

#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis
{
namespace Application
{
namespace
{
const std::string FINISH_DONE_NAME = "finish.txt";
// Last line of a finished export.
const std::string DONE_MARKER = "done";
std::mutex gStateMu;
std::string gOutputDir;
std::string gResetDir;
}  // namespace

std::string FinishDoneWriter::GetOutputDir(const std::string &profPath)
{
    if (profPath.empty())
    {
        return "";
    }
    return Utils::File::PathJoin({profPath, Common::OUTPUT_PATH});
}

bool FinishDoneWriter::Create(const std::string &profPath)
{
    const std::string dir = GetOutputDir(profPath);
    if (dir.empty())
    {
        ERROR("Create finish.txt failed: the profile path is empty.");
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(gStateMu);
        gOutputDir = dir;
        if (gResetDir == dir)
        {
            return true;
        }
        gResetDir = dir;
    }
    // Environment.Load may run before the export init node creates the output directory.
    if (!Utils::File::CreateDir(dir))
    {
        ERROR("Create output directory failed, so finish.txt is skipped: %.", dir);
        return false;
    }
    const std::string report = Utils::File::PathJoin({dir, FINISH_DONE_NAME});
    // Drop the report of a previous run instead of relying on the open mode, so no entry of it can
    // survive into this export.
    if (Utils::File::Exist(report) && !Utils::File::DeleteFile(report))
    {
        ERROR("Remove the previous finish.txt failed: %.", report);
        return false;
    }
    Utils::FileWriter file;
    file.Open(report, std::ios::out | std::ios::trunc);
    if (!file.IsOpen())
    {
        ERROR("Open finish.txt failed: %.", report);
        return false;
    }
    return true;
}

bool FinishDoneWriter::Append(const std::string &filePath)
{
    std::string dir;
    {
        std::lock_guard<std::mutex> lock(gStateMu);
        dir = gOutputDir;
    }
    if (filePath.empty() || dir.empty() || !Utils::File::Exist(filePath))
    {
        WARN("Skip reporting a deliverable that is not on disk: %.", filePath);
        return false;
    }
    Utils::FileWriter file;
    file.Open(Utils::File::PathJoin({dir, FINISH_DONE_NAME}), std::ios::out | std::ios::app);
    if (!file.IsOpen())
    {
        ERROR("Open finish.txt failed: %.", dir);
        return false;
    }
    file.WriteText(Utils::File::BaseName(filePath) + "\n");
    if (!file.IsGood())
    {
        ERROR("Write finish.txt failed: %.", dir);
        return false;
    }
    return true;
}

bool FinishDoneWriter::MarkDone()
{
    std::string dir;
    {
        std::lock_guard<std::mutex> lock(gStateMu);
        dir = gOutputDir;
    }
    if (dir.empty())
    {
        WARN("Skip the done marker: finish.txt was not created in this run.");
        return false;
    }
    Utils::FileWriter file;
    file.Open(Utils::File::PathJoin({dir, FINISH_DONE_NAME}), std::ios::out | std::ios::app);
    if (!file.IsOpen())
    {
        ERROR("Open finish.txt failed: %.", dir);
        return false;
    }
    file.WriteText(DONE_MARKER + "\n");
    if (!file.IsGood())
    {
        ERROR("Write the done marker failed: %.", dir);
        return false;
    }
    return true;
}
}  // namespace Application
}  // namespace Analysis
