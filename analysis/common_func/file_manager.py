# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
# pylint: skip-file
import json
import logging
import os
import re

from common_func.common import CommonConstant, print_info
from common_func.common import error
from common_func.common import is_linux
from common_func.constant import Constant
from common_func.empty_class import EmptyClass
from common_func.file_name_manager import FileNameManagerConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from common_func.return_code_checker import ReturnCodeCheck


class FileManager:
    """
    class to manage files
    """

    FILE_AUTHORITY = 0o640

    @staticmethod
    def remove_file(file_path: str) -> bool:
        """
        remove file
        """
        try:
            if os.path.isfile(file_path):
                os.remove(file_path)
                return True
            return False
        except OSError as err:
            error(os.path.basename(__file__), str(err))
            return False

    @staticmethod
    def match_process(pattern: str, file_name: str) -> any:
        """
        match files
        :param pattern: check pattern
        :param file_name: check files
        :return:
        """
        matched_name = re.match(pattern, file_name)
        if matched_name:
            return matched_name
        return EmptyClass("empty match name")

    @classmethod
    def is_info_json_file(cls: any, file_name: str) -> any:
        """
        check whether file match info.json pattern
        :param file_name: check files
        :return: matched groups
        """
        return cls.match_process(CommonConstant.INFO_JSON_PATTERN, file_name)

    @classmethod
    def is_valid_jobid(cls: any, check_string: str) -> any:
        """
        check whether job id matches JOB_ID_PATTERN
        :param check_string: check strings
        :return: matched groups
        """
        return cls.match_process(CommonConstant.JOB_ID_PATTERN, check_string)

    @classmethod
    def is_analyzed_data(cls: any, project_path: str) -> bool:
        """
        return if data dir contains all parse complete file
        """
        data_dir = PathManager.get_data_dir(project_path)
        sqlite_dir = PathManager.get_sql_dir(project_path)
        is_analyse = False
        sqlite_list = os.listdir(sqlite_dir) if os.path.exists(sqlite_dir) else []
        if os.path.exists(data_dir) and sqlite_list:
            all_file_name = os.path.join(data_dir, FileNameManagerConstant.ALL_FILE_TAG)
            if os.path.exists(all_file_name):
                is_analyse = True
                logging.warning(
                    "Data has been analyzed, path is %s, there are %s in sqlite.", project_path, sqlite_list
                )
        return is_analyse

    @classmethod
    def add_complete_file(cls: any, project_path: str, file_name: str) -> None:
        """
        add complete file
        """
        file_dir = PathManager.get_data_dir(project_path)
        try:
            if os.path.exists(os.path.join(file_dir, file_name)) and not file_name.endswith(Constant.COMPLETE_TAG):
                file_path = os.path.join(file_dir, file_name + Constant.COMPLETE_TAG)
                with FdOpen(file_path):
                    os.chmod(file_path, cls.FILE_AUTHORITY)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            error(os.path.basename(__file__), err)

    @classmethod
    def storage_query_result_json_file(cls: any, collection_path: str, result_data: dict, file_name: str) -> None:
        if not result_data:
            return
        query_path = os.path.join(collection_path, PathManager.QUERY_CLUSTER)
        if not os.path.exists(query_path):
            try:
                os.makedirs(query_path, mode=NumberConstant.DIR_AUTHORITY)
            except OSError:
                error(
                    os.path.basename(__file__),
                    "Storing data failed, you may not have the permission to write files in the current path.",
                )
                return
        output_file_path = PathManager.get_query_result_path(collection_path, file_name)
        check_path_valid(output_file_path, True)
        try:
            with FdOpen(output_file_path) as file:
                json.dump(result_data, file)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            error(os.path.basename(__file__), err)
        else:
            print_info(
                os.path.basename(__file__), "The data has stored successfully, file path: {}".format(output_file_path)
            )


class FileOpen:
    """
    open and read file
    """

    def __init__(self: any, file_path: str, mode: str = "r", max_size: int = Constant.MAX_READ_FILE_BYTES) -> None:
        self.file_path = file_path
        self.file_reader = None
        self.mode = mode
        self.max_size = max_size

    def __enter__(self: any) -> any:
        check_path_valid(self.file_path, True, max_size=self.max_size)
        self.file_reader = open(self.file_path, self.mode)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.file_reader:
            self.file_reader.close()


class FdOpen:
    """
    creat and write file
    """

    def __init__(
        self: any,
        file_path: str,
        flags: int = Constant.WRITE_FLAGS,
        mode: int = Constant.WRITE_MODES,
        operate: str = "w",
        newline: str = None,
    ) -> None:
        self.file_path = file_path
        self.flags = flags
        self.newline = newline
        self.mode = mode
        self.operate = operate
        self.fd = None
        self.file_open = None

    def __enter__(self: any) -> any:
        file_dir = os.path.dirname(self.file_path)
        check_dir_writable(file_dir)
        self.fd = os.open(self.file_path, self.flags, self.mode)
        if self.newline is None:
            self.file_open = os.fdopen(self.fd, self.operate)
        else:
            self.file_open = os.fdopen(self.fd, self.operate, newline=self.newline)
        return self.file_open

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.file_open:
            self.file_open.close()
        elif self.fd:
            os.close(self.fd)


def is_link(path: str) -> bool:
    """
    软链接校验漏洞，在最后加一个/或者./可以获取到绝对路径进行校验
    """
    if not path:
        return False
    path = os.path.abspath(path)
    return os.path.islink(path)


def check_path_valid(path: str, is_file: bool, max_size: int = Constant.MAX_READ_FILE_BYTES) -> None:
    """
    check the path is valid or not
    :param path: file path
    :param is_file: file or not
    :param max_size: file's max size
    :return: None
    """
    if path == "":
        raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, "The path is empty. Please enter a valid path.")
    if not os.path.exists(path):
        raise ProfException(
            ProfException.PROF_INVALID_PATH_ERROR,
            f"The path \"{path}\" does not exist. Please check that the path exists.",
        )
    if is_link(path):
        raise ProfException(
            ProfException.PROF_INVALID_PATH_ERROR, f"The path \"{path}\" is link. Please check the path."
        )
    if len(path) > NumberConstant.PROF_PATH_MAX_LEN:
        raise ProfException(
            ProfException.PROF_INVALID_PATH_ERROR,
            f"Please ensure the length of input path \"{path}\" less than {NumberConstant.PROF_PATH_MAX_LEN}",
        )
    if is_file:
        if not os.path.isfile(path):
            raise ProfException(
                ProfException.PROF_INVALID_PATH_ERROR, f"The path \"{path}\" is not a file. Please check the path."
            )
        if os.path.getsize(path) > max_size:
            raise ProfException(
                ProfException.PROF_INVALID_PATH_ERROR,
                f"The path \"{path}\" is too large to read. Please check the path.",
            )
    else:
        if not os.path.isdir(path):
            raise ProfException(
                ProfException.PROF_INVALID_PATH_ERROR, f"The path \"{path}\" is not a directory. Please check the path."
            )
    if is_root_user():
        return
    if not os.access(path, os.R_OK):
        raise ProfException(
            ProfException.PROF_INVALID_PATH_ERROR,
            f"The path \"{path}\" does not have permission to read. Please check that the path is readable.",
        )
    if is_other_writable(path):
        raise ProfException(
            ProfException.PROF_INVALID_PATH_ERROR, f"The path \"{path}\" is writable by any other users."
        )
    if not check_path_owner(path):
        raise ProfException(
            ProfException.PROF_INVALID_PATH_ERROR, f"The path \"{path}\" owner is not root or current user."
        )


def check_db_path_valid(path: str, is_create: bool = False, max_size: int = Constant.MAX_READ_DB_FILE_BYTES) -> None:
    if is_link(path):
        ReturnCodeCheck.print_and_return_status(
            json.dumps(
                {'status': NumberConstant.ERROR, 'info': "The db file '%s' is link. Please check the path." % path}
            )
        )

    if not is_create and os.path.exists(path) and os.path.getsize(path) > max_size:
        ReturnCodeCheck.print_and_return_status(
            json.dumps(
                {
                    'status': NumberConstant.ERROR,
                    'info': "The db file '%s' is too large to read. Please check the path." % path,
                }
            )
        )


def check_file_readable(path: str) -> None:
    """
    check path is file and readable
    """
    check_path_valid(path, True)


def check_file_writable(path: str) -> None:
    """
    check path is file and writable
    """
    if path and os.path.exists(path):
        check_path_valid(path, True)
        if not os.access(path, os.W_OK):
            raise ProfException(
                ProfException.PROF_INVALID_PATH_ERROR,
                f"The path \"{path}\" does not have permission to write. Please check that the path is writeable.",
            )


def check_dir_readable(path: str) -> None:
    """
    check path is dir and readable
    """
    check_path_valid(path, False)


def check_dir_writable(path: str) -> None:
    """
    check path is dir and writable
    """
    check_path_valid(path, False)
    if is_root_user():
        return
    if not os.access(path, os.W_OK):
        raise ProfException(
            ProfException.PROF_INVALID_PATH_ERROR,
            f"The path \"{path}\" does not have permission to write. Please check that the path is writeable.",
        )


def is_other_writable(path: str) -> bool:
    """
    check whether others can write
    0o022 means 755
    """
    stat_info = os.stat(path)
    return bool(stat_info.st_mode & 0o022)


def check_path_owner(path: str) -> bool:
    """
    Check the file owner is the root user or the current user.
    """
    if not is_linux():
        return True
    stat_info = os.stat(path)
    if stat_info.st_uid == 0:  # so的所有者为root用户
        return True
    else:
        current_uid = os.geteuid()
        return current_uid == stat_info.st_uid


def check_parent_dir_invalid(paths: list) -> None:
    for path in paths:
        check_path_valid(path, False)


def check_so_valid(path: str) -> bool:
    """
    check so file is file and executable
    """
    if not os.path.isfile(path):
        logging.warning("The path %s is not a file. Please check the path.", path)
        return False
    elif is_link(path):
        logging.warning("The path %s is link. Please check the path.", path)
        return False
    elif not os.access(path, os.R_OK):
        logging.warning("The path %s does not have permission to open. Please check that the path is readable.", path)
        return False
    elif is_other_writable(path):
        logging.warning("The path %s is writable by others", path)
        return False
    elif not check_path_owner(path):
        logging.warning("The path %s owner is not the current user.", path)
        return False
    else:
        return True


def is_root_user() -> bool:
    return not is_linux() or os.getuid() == 0
