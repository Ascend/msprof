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

#ifndef ANALYSIS_UTILS_FILE_H
#define ANALYSIS_UTILS_FILE_H

#include <string>
#include <fstream>
#include <vector>
#include <stdint.h>

#include "opensource/json/include/nlohmann/json.hpp"

namespace Analysis {
namespace Utils {
// 该类主要用于文件相关的处理接口
// 主要包括以下特性：
// 1. 空路径、软连接和路径长度的校验
// 2. Chmod修改文件和目录的权限
// 3. 文件和目录的rwx权限校验
// 4. 创建目录、路径拼接、获取子文件名等操作
class File {
public:
    File() = default;
    virtual ~File() = default;
    // 文件的打开默认大小设置为64MB，允许使用时设置自己需要的大小
    static bool Check(const std::string &path, uint64_t maxReadFileBytes = 64 * 1024 * 1024);
    static bool CreateDir(const std::string &path, const mode_t &mode = 0750);
    static bool RemoveDir(const std::string &path, int depth);
    static bool CheckDir(const std::string &path);
    static bool Chmod(const std::string &path, const mode_t &mode);
    static std::string PathJoin(const std::vector<std::string> &paths);
    static std::string BaseName(const std::string &path);
    static std::vector<std::string> GetFilesWithPrefix(const std::string &path, const std::string &prefix);
    static std::vector<std::string> FilterFileWithSuffix(const std::vector<std::string> &files,
                                                         const std::string &suffix);
    /*!
     * 将文件排序，排序依据为: 第一优先级：unaging>aging, 第二优先级：slice num递增
     */
    static std::vector<std::string> SortFilesByAgingAndSliceNum(std::vector<std::string> &files);
    /*!
     * 从data目录下筛选文件
     * @param path data目录
     * @param prePatterns 用于匹配的前缀列表
     * @param sufFilters 用于筛选的后缀列表
     * @return 筛选后的文件列表
     */
    static std::vector<std::string> GetOriginData(const std::string &path, const std::vector<std::string> &prePatterns,
                                                  const std::vector<std::string> &sufFilters);
    static bool Access(const std::string &path, const int &mode);
    static bool Exist(const std::string &path);
    static bool IsSoftLink(const std::string &path);
    static bool IsFile(const std::string &path);
    static bool DeleteFile(const std::string &filePath);
    static uint64_t Size(const std::string &filePath);
};  // class File

// 该类主要用于文件安全读取
// 主要包括以下特性：
// 1. 文本文件、二进制文件和json文件的安全读取
// 2. 安全校验：空路径和软连接的校验，文件校验，文件大小校验，文件读权限校验
class FileReader {
public:
    FileReader() = default;
    explicit FileReader(const std::string &path, const std::ios_base::openmode &mode = std::ios::in) : path_(path)
    {
        Open(path, mode);
    }
    virtual ~FileReader()
    {
        Close();
    }
    void Open(const std::string &path, const std::ios_base::openmode &mode = std::ios::in);
    void Close();
    bool IsOpen() const;
    int ReadBinary(std::stringstream &ss);
    int ReadText(std::vector<std::string> &text);
    int ReadJson(nlohmann::json &content);
    static bool Check(const std::string &path, uint64_t maxReadFileBytes = 64 * 1024 * 1024);

private:
    std::string path_;
    std::ifstream inStream_;
};  // class FileReader

// 该类主要用于文件安全写入
// 主要包括以下特性：
// 1. 字符串写入文件
// 2. 安全校验：空路径和软连接的校验，文件校验，文件写权限校验
// 3. 设置写入文件的rwx权限，默认0640
class FileWriter {
public:
    FileWriter() = default;
    explicit FileWriter(const std::string &path,
                        const std::ios_base::openmode &mode = std::ios::out, const mode_t &permission = 0640)
        : path_(path), permission_(permission)
    {
        Open(path, mode);
    }
    virtual ~FileWriter()
    {
        Close();
    }
    void Open(const std::string &path, const std::ios_base::openmode &mode = std::ios::out);
    void Close();
    bool IsOpen() const;
    void WriteText(const std::string &content);
    void WriteText(const char *content, std::size_t len);
    void WriteTextBack(const std::string &content, int back);
    static bool Check(const std::string &path, uint64_t maxReadFileBytes = 64 * 1024 * 1024);

private:
    std::string path_;
    std::ofstream outStream_;
    mode_t permission_ = 0640;
};  // class FileWriter
}  // namespace Utils
}  // namespace Analysis

#endif // ANALYSIS_UTILS_FILE_H
