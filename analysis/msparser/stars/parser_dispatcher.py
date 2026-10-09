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

import importlib
import logging

from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager
from msconfig.config_manager import ConfigManager


class ParserDispatcher:
    """
    class used to dispatch diff kind data type.
    And read the mapping relationship from the stars configuration file.
    The mapping relationship between processor, fmt, db and table is recorded in the stars configuration file.
    """

    def __init__(self: any, result_dir: str) -> None:
        self.parser_map = {}
        self.parser_list = []
        self.result_dir = result_dir
        self.cfg_parser = None
        self.known_func_type_list = (
            ConfigManager.get(ConfigManager.FUNC_TYPE).KNOWN_FUNC_TYPE_LIST_V6
            if ChipManager().is_chip_v6()
            else ConfigManager.get(ConfigManager.FUNC_TYPE).KNOWN_FUNC_TYPE_LIST
        )
        self.modules = importlib.import_module("msparser.stars")

    def init(self: any) -> None:
        """
        load stars config info and build parser map
        :return:
        """
        self.cfg_parser = ConfigManager.get(ConfigManager.STARS)
        self.build_parser_map()

    def build_parser_map(self: any) -> None:
        """
        build parser fmt map
        :return: NA
        """
        for parser in self.cfg_parser.sections():
            fmts = self.cfg_parser.get(parser, StrConstant.CONFIG_FMT).split(",")
            db_name = self.cfg_parser.get(parser, StrConstant.CONFIG_DB)
            table_list = self.cfg_parser.get(parser, StrConstant.CONFIG_TABLE).split(",")
            if hasattr(self.modules, parser):
                parser = getattr(self.modules, parser)(self.result_dir, db_name, table_list)
                for fmt in fmts:
                    self.parser_map[fmt.strip()] = parser

    def dispatch(self: any, func_type: str, data: bytes) -> None:
        """
        dispatch parser by func_type
        :param func_type: func_type
        :param data: binary data
        :return:NA
        """
        if func_type in self.parser_map:
            self.parser_map.get(func_type).handle(func_type, data)
        else:
            if func_type not in self.known_func_type_list:
                logging.error("Invalid func type %s", func_type)
            else:
                logging.warning("Not support func type %s", func_type)

    def flush_all_parser(self: any) -> None:
        """
        flush all parser data to db
        :return: NA
        """
        for parser in self.parser_map:
            self.parser_map.get(parser).flush()
