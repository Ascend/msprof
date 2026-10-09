# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

# pylint: disable=duplicate-code

import os
import json


class FileManager:
    MAX_PATH_LENGTH = 4096
    MAX_FILE_SIZE = 1024 * 1024 * 1024 * 10
    DATA_FILE_AUTHORITY = 0o640
    DATA_DIR_AUTHORITY = 0o750

    @classmethod
    def make_dir_safety(cls, path: str):
        msg = f"Failed to make directory: {path}"
        if os.path.islink(path):
            raise RuntimeError(msg)
        if os.path.exists(path):
            return
        try:
            os.makedirs(path, mode=cls.DATA_DIR_AUTHORITY, exist_ok=True)
        except Exception as err:
            raise RuntimeError(msg) from err

    @classmethod
    def create_file_safety(cls, path: str):
        msg = f"Failed to create file: {path}"
        if os.path.islink(path):
            raise RuntimeError(msg)
        if os.path.exists(path):
            return
        try:
            os.close(os.open(path, os.O_WRONLY | os.O_CREAT, cls.DATA_FILE_AUTHORITY))
        except Exception as err:
            raise RuntimeError(msg) from err

    @classmethod
    def check_directory_path_writeable(cls, path):
        cls.check_path_owner_consistent(path)
        if os.path.islink(path):
            msg = f"Invalid path is a soft link: {path}"
            raise RuntimeError(msg)
        if not os.access(path, os.W_OK):
            msg = f"The path permission check failed: {path}"
            raise RuntimeError(msg)

    @classmethod
    def check_directory_path_readable(cls, path):
        cls.check_path_owner_consistent(path)
        if os.path.islink(path):
            msg = f"Invalid path is a soft link: {path}"
            raise RuntimeError(msg)
        if not os.access(path, os.R_OK):
            msg = f"The path permission check failed: {path}"
            raise RuntimeError(msg)

    @classmethod
    def check_path_owner_consistent(cls, path: str):
        if not os.path.exists(path):
            msg = f"The path does not exist: {path}"
            raise RuntimeError(msg)
        if os.getuid() == 0:
            return
        if os.stat(path).st_uid != os.getuid():
            msg = f"Permission mismatch: The owner of {path} does not match."
            raise RuntimeError(msg)

    @classmethod
    def read_json_file(cls, file_path: str):
        if not os.path.isfile(file_path):
            return None
        file_size = os.path.getsize(file_path)
        if file_size <= 0:
            return None
        if file_size > cls.MAX_FILE_SIZE:
            msg = f"The file size exceeds the preset value, please check the file: {file_path}"
            raise RuntimeError(msg)
        cls.check_directory_path_readable(file_path)
        try:
            with open(file_path, 'r', encoding='utf-8') as json_file:
                data = json.load(json_file)
                return data
        except Exception as err:
            raise RuntimeError(f"Failed to read the file: {file_path}") from err

    @classmethod
    def create_json_file(cls, data: list, file_path: str):
        if not data:
            return
        file_path = os.path.abspath(os.path.realpath(file_path))
        if len(file_path) > cls.MAX_PATH_LENGTH:
            raise RuntimeError("Length of input path exceeds the limit.")
        dir_name = os.path.dirname(file_path)
        cls.make_dir_safety(dir_name)
        cls.create_file_safety(file_path)
        cls.check_directory_path_writeable(file_path)
        try:
            with open(file_path, "w", encoding="utf-8") as file:
                json.dump(data, file, ensure_ascii=False)
        except Exception as err:
            raise RuntimeError(f"Can't create file: {file_path}") from err
