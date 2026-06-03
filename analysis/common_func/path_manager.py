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

import logging
import os
import shutil
import stat

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant


class PathManager:
    """
    file path manager
    """

    DATA = "data"
    LOG = ".log"
    SQLITE = "sqlite"
    HOST = "host"
    COLLECTION_LOG = "collection_"
    DISPATCH = "dispatch"
    TIMELINE = 'timeline'
    SUMMARY = 'summary'
    SAMPLE_JSON = "sample.json"
    PROFILER = ".profiler"
    HCCL = "hccl"
    QUERY_CLUSTER = "query"
    ANALYZE = 'analyze'
    MINDSTUDIO_PROFILER_LOG = "mindstudio_profiler_log"
    MINDSTUDIO_PROFILER_OUTPUT = "mindstudio_profiler_output"

    @staticmethod
    def get_path_under_result_dir(result_dir: str, *paths: str) -> str:
        """
        get directory path
        """
        if result_dir is not None:
            return os.path.join(result_dir, *paths)
        return ""

    @staticmethod
    def check_map_file_path(map_file_path: str, cfg_parser: any) -> bool:
        map_file_path = os.path.realpath(map_file_path)
        try:
            if os.path.getsize(map_file_path) <= Constant.MAX_READ_FILE_BYTES:
                cfg_parser.read(map_file_path)
                return True
            return False
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as error:
            logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)
            return False

    @classmethod
    def get_data_dir(cls: any, result_dir: str) -> str:
        """
        get data directory in result directory
        """
        return cls.get_path_under_result_dir(result_dir, cls.DATA)

    @classmethod
    def get_data_file_path(cls: any, result_dir: str, file_name: str) -> str:
        """
        get data file path with file name
        """
        return os.path.join(cls.get_data_dir(result_dir), file_name)

    @classmethod
    def get_log_dir(cls: any, result_dir: str) -> str:
        """
        get log directory in result directory
        """
        return os.path.realpath(cls.get_path_under_result_dir(os.path.dirname(result_dir), cls.MINDSTUDIO_PROFILER_LOG))

    @classmethod
    def get_sql_dir(cls: any, result_dir: str) -> str:
        """
        get sqlite directory in result directory
        """
        return cls.get_path_under_result_dir(result_dir, cls.SQLITE)

    @classmethod
    def get_host_result_dir(cls, result_dir: str):
        """
        get host directory
        """
        return os.path.realpath(cls.get_path_under_result_dir(result_dir, "..", cls.HOST))

    @classmethod
    def get_db_path(cls: any, result_dir: str, db_name: str) -> str:
        """
        get db name path in result directory
        """
        db_filter = {
            DBNameConstant.DB_ACL_MODULE,
            DBNameConstant.DB_RUNTIME,
            DBNameConstant.DB_GE_MODEL_INFO,
            DBNameConstant.DB_GE_HOST_INFO,
            DBNameConstant.DB_GE_INFO,
            DBNameConstant.DB_RTS_TRACK,
            DBNameConstant.DB_HCCL,
            DBNameConstant.DB_MSPROFTX,
            DBNameConstant.DB_GE_HASH,
            DBNameConstant.DB_API_EVENT,
            DBNameConstant.DB_HCCL_INFO,
            DBNameConstant.DB_MULTI_THREAD,
            DBNameConstant.DB_TENSOR_ADD_INFO,
            DBNameConstant.DB_NODE_BASIC_INFO,
            DBNameConstant.DB_FUSION_ADD_INFO,
            DBNameConstant.DB_GRAPH_ADD_INFO,
            DBNameConstant.DB_CTX_ID,
            DBNameConstant.DB_SYNC_ACL_NPU,
            DBNameConstant.DB_MEMORY_OP,
            DBNameConstant.DB_GE_LOGIC_STREAM_INFO,
            DBNameConstant.DB_MC2_COMM_INFO,
            DBNameConstant.DB_STREAM_INFO,
            DBNameConstant.DB_DPU,
        }
        base_result_dir = result_dir
        if db_name in db_filter:
            base_result_dir = cls.get_host_result_dir(result_dir)
        return cls.get_path_under_result_dir(base_result_dir, cls.SQLITE, db_name)

    @classmethod
    def get_collection_log_path(cls: any, result_dir: str) -> str:
        """
        get collection log path
        """
        base_name = os.path.basename(result_dir)
        return cls.get_path_under_result_dir(cls.get_log_dir(result_dir), cls.COLLECTION_LOG + base_name + cls.LOG)

    @classmethod
    def get_dispatch_dir(cls: any, result_dir: str) -> str:
        """
        get dispatch dir in install path
        :return: the dispatch dir
        """
        return cls.get_path_under_result_dir(result_dir, cls.PROFILER, cls.DISPATCH)

    @classmethod
    def get_summary_dir(cls: any, result_dir: str) -> str:
        """
        get summary dir in result directory
        :return: the summary dir
        """
        return cls.get_path_under_result_dir(result_dir, cls.SUMMARY)

    @classmethod
    def get_timeline_dir(cls: any, result_dir: str) -> str:
        """
        get timeline dir in result directory
        :return: the timeline dir
        """
        return cls.get_path_under_result_dir(result_dir, cls.TIMELINE)

    @classmethod
    def get_analyze_dir(cls: any, result_dir: str) -> str:
        """
        get timeline dir in result directory
        :return: the timeline dir
        """
        return cls.get_path_under_result_dir(result_dir, cls.ANALYZE)

    @classmethod
    def get_sample_json_path(cls: any, result_dir: str) -> str:
        """
        get sample json path in result directory
        """
        return cls.get_path_under_result_dir(result_dir, cls.SAMPLE_JSON)

    @classmethod
    def get_hccl_path(cls: any, result_dir: str) -> str:
        """
        get hccl trace path in result directory
        """
        return cls.get_path_under_result_dir(result_dir, cls.HCCL)

    @classmethod
    def get_query_result_path(cls: any, result_dir: str, file_name: str) -> str:
        """
        get query result path in result directory
        """
        return cls.get_path_under_result_dir(result_dir, cls.QUERY_CLUSTER, file_name)

    @classmethod
    def get_analyze_result_path(cls: any, result_dir: str, file_name: str) -> str:
        """
        get query result path in result directory
        """
        return cls.get_path_under_result_dir(result_dir, cls.ANALYZE, file_name)

    @classmethod
    def get_device_count(cls: any, result_dir: str) -> int:
        device_count = 0
        file_list = os.listdir(os.path.dirname(result_dir))
        for file_name in file_list:
            if file_name.startswith("device_"):
                device_count += 1
        return device_count

    @classmethod
    def del_summary_and_timeline_dir(cls: any, sub_dirs: list):
        for sub_dir in sub_dirs:
            sub_path = os.path.realpath(sub_dir)
            summary_dir = PathManager.get_summary_dir(sub_path)
            timeline_dir = PathManager.get_timeline_dir(sub_path)
            PathManager.del_dir(summary_dir)
            PathManager.del_dir(timeline_dir)

    @classmethod
    def del_dir(cls: any, del_path):
        """
        先删除所有文件，再使用shutil.rmtree递归删除所有空的文件夹
        直接使用shutil.rmtree会遇到权限问题
        :param del_path:
        :return:
        """
        for dir_path, _, file_list in cls.safe_os_walk(del_path):
            for file in file_list:
                file_path = os.path.join(dir_path, file)
                os.chmod(file_path, stat.S_IWUSR)  # 这里解决删除不了的权限问题
                os.remove(file_path)
        shutil.rmtree(del_path, ignore_errors=True)

    @classmethod
    def safe_os_walk(cls, path, *args, max_depth=10, **kwargs):
        """
        带深度限制的目录遍历器，类似os.walk但限制最大递归深度
        :param path: 根目录路径
        :param args: 传递给os.walk的位置参数
        :param max_depth: 最大递归深度
        :param kwargs: 传递给os.walk的关键字参数
        :yield: (dir_path, dirs, files) 与os.walk相同的元组
              - dir_path: 当前目录的绝对路径
              - dirs: 当前目录下的子目录列表（可修改以控制遍历行为）
              - files: 当前目录下的文件列表
        """
        if not isinstance(path, str):
            return
        base_depth = path.count(os.sep)
        if path.endswith(os.sep):
            base_depth -= 1
        for root, dirs, files in os.walk(path, *args, **kwargs):
            if root.count(os.sep) - base_depth >= max_depth:
                dirs.clear()
                continue
            yield root, dirs, files
