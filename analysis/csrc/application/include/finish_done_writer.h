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

#ifndef ANALYSIS_APPLICATION_INCLUDE_FINISH_DONE_WRITER_H
#define ANALYSIS_APPLICATION_INCLUDE_FINISH_DONE_WRITER_H

#include <string>

namespace Analysis
{
namespace Application
{
// finish.txt lists the deliverables that one export finished, one real file name per line, and ends
// with a done marker written by the last node of the graph. Environment.Load truncates it when the
// export graph starts, so a run never shows entries of the previous one.
class FinishDoneWriter
{
   public:
    static std::string GetOutputDir(const std::string &profPath);
    // Create or truncate finish.txt in the output directory of profPath.
    static bool Create(const std::string &profPath);
    // Append the file name of a finished deliverable, whatever directory it lives in. Nothing is
    // appended when the deliverable itself is missing.
    static bool Append(const std::string &filePath);
    // Append the done marker. A reader can then tell a finished export from an interrupted one.
    static bool MarkDone();
};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_INCLUDE_FINISH_DONE_WRITER_H
