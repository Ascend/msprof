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

from abc import ABCMeta
from abc import abstractmethod


class IProfFactory(metaclass=ABCMeta):
    """
    interface of factory for dealing with data
    """

    @classmethod
    def _launch_parser_list(cls: any, sample_config: dict, file_list: dict, data_class: dict) -> None:
        """
        use multi-processing to run data class
        :param sample_config: sample config
        :param file_list: file list
        :param data_class: class ready to be parsed
        :return: NA
        """
        for _, parsing_class in data_class.items():
            cls._run_parsers(parsing_class, sample_config, file_list)

    @classmethod
    def _run_parsers(cls: any, parser_list: list, sample_config: dict, file_list: dict) -> None:
        parsing_obj = []
        for parsing_class in parser_list:
            parsing_obj.append(parsing_class(file_list, sample_config))
        # start parsing processor
        for item in parsing_obj:
            item.start()
        # join parsing processor
        for item in parsing_obj:
            item.join()

    @abstractmethod
    def run(self: any) -> any:
        """
        entry for factory to run
        :return: NA
        """

    @abstractmethod
    def generate(self: any, chip_model: any) -> any:
        """
        generate the data class
        :param chip_model: 0,1 or 2
        :return: set of data class
        """
