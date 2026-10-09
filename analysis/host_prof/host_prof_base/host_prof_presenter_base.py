# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2025 Huawei Technologies Co.,Ltd.
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

import logging
from abc import abstractmethod

from common_func.common import call_sys_exit
from common_func.constant import Constant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.file_manager import check_file_readable
from common_func.path_manager import PathManager


class HostProfPresenterBase:
    """
    class for parsing host prof data
    """

    def __init__(self: any, result_dir: str, file_name: str) -> None:
        self.result_dir = result_dir
        self.file_name = PathManager.get_data_file_path(result_dir, file_name)
        self.cur_model = None

    @abstractmethod
    def init(self: any) -> None:
        """
        init model, sub class to instance
        :return: None
        """

    @abstractmethod
    def parse_prof_data(self: any) -> None:
        """
        parse prof data, sub class to instance
        :return: None
        """

    def process(self: any) -> None:
        """
        detail process
        :return: None
        """
        check_file_readable(self.file_name)
        self.init()
        self.cur_model.init()
        self._create_tables()
        self.parse_prof_data()
        self.cur_model.flush_data()

    def run(self: any) -> None:
        """
        process data
        :return: None
        """
        try:
            self.process()
        except (OSError, SystemError, ProfException, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            if self.cur_model is not None:
                self.cur_model.finalize()

    def set_model(self: any, model: any) -> None:
        """
        init model
        :param model: model
        :return: None
        """
        self.cur_model = model

    @abstractmethod
    def get_timeline_data(self: any) -> list:
        """
        return timeline data, sub class to instance
        :return: timeline data
        """

    @abstractmethod
    def get_timeline_header(self: any) -> list:
        """
        get timeline header
        :return: timeline headers
        """

    def _create_tables(self: any) -> None:
        if not self.cur_model.init():
            logging.error("Failed to connect to host usage database.")
            call_sys_exit(NumberConstant.ERROR)
        self.cur_model.create_table()
