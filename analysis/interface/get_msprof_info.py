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

# 该类会涉及到外部调用，因此如果基于相对路径引入其他文件的函数，可能在实际调用的时候报错找不到对应模块，因此涉及到新增引入的函数时，要将import
# 放在main函数调用内部，或者将设置路径的代码移到import最上面

import argparse
import importlib
import logging
import os
import sys


class MsprofInfoConstruct:
    """
    get basic info
    """

    FILE_NAME = os.path.basename(__file__)
    BASIC_MODEL_PATH = "profiling_bean.basic_info.msprof_basic_info"
    BASIC_INFO_CLASS_NAME = "MsProfBasicInfo"
    CLUSTER_INFO_MODEL_PATH = "profiling_bean.basic_info.msprof_cluster_info"
    CLUSTER_INFO_CLASS_NAME = "MsProfClusterInfo"
    PROF_PATH_MAX_LEN = 1024

    @staticmethod
    def construct_argument_parser() -> argparse.ArgumentParser:
        """
        construct argument parser for basic info
        :return: arg parser
        """
        parser = argparse.ArgumentParser()
        parser.add_argument(
            '-dir',
            '--collection-dir',
            dest='collection_path',
            default='',
            metavar='<dir>',
            type=str,
            help='<Mandatory> Specify the directory that is used for creating data collection results.',
            required=True,
        )
        return parser

    @staticmethod
    def _check_cluster_sqlite(path: str) -> bool:
        from common_func.db_name_constant import DBNameConstant
        from common_func.path_manager import PathManager

        path = os.path.realpath(path)
        return os.path.exists(PathManager.get_db_path(path, DBNameConstant.DB_CLUSTER_RANK))

    def load_basic_info_model(self: any, args: any) -> None:
        """
        load model of basic info class
        :param args: collection path
        :return: None
        """
        from common_func.common import print_msg

        if not hasattr(args, "collection_path"):
            return

        if MsprofInfoConstruct._check_cluster_sqlite(args.collection_path):
            model_obj = importlib.import_module(self.CLUSTER_INFO_MODEL_PATH)
            if hasattr(model_obj, self.CLUSTER_INFO_CLASS_NAME):
                msprof_cluster_info = getattr(model_obj, self.CLUSTER_INFO_CLASS_NAME)(args.collection_path)
                msprof_cluster_info.run()
        else:
            model_obj = importlib.import_module(self.BASIC_MODEL_PATH)
            if hasattr(model_obj, self.BASIC_INFO_CLASS_NAME):
                msprof_basic_info = getattr(model_obj, self.BASIC_INFO_CLASS_NAME)(args.collection_path)
                msprof_basic_info.init()
                print_msg(msprof_basic_info.run())

    def main(self: any) -> None:
        """
        interface entry for basic info
        :return:None
        """
        from common_func.common import error
        from common_func.msprof_common import check_path_valid
        from common_func.msprof_common import check_path_char_valid
        from common_func.msprof_exception import ProfException

        parser = self.construct_argument_parser()

        if len(sys.argv) < 2:
            parser.print_help()
            return

        args = parser.parse_args(sys.argv[1:])

        if hasattr(args, "collection_path"):
            path_len = len(os.path.realpath(args.collection_path))
            if path_len > self.PROF_PATH_MAX_LEN:
                error(
                    self.FILE_NAME,
                    "Please ensure the length of input dir absolute path(%s) less than %s"
                    % (path_len, self.PROF_PATH_MAX_LEN),
                )
                return
        try:
            check_path_char_valid(args.collection_path)
            check_path_valid(args.collection_path, False)
        except ProfException:
            logging.error("Input collection path is invalid, please check")
            return

        try:
            self.load_basic_info_model(args)
        except Exception as err:
            logging.error(err)


if __name__ == '__main__':
    sys.path.append(os.path.realpath(os.path.join(os.path.dirname(__file__), '..')))
    MsprofInfoConstruct().main()
