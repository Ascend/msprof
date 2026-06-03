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

import configparser
import importlib

from common_func.file_manager import check_file_readable
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from common_func.cpp_enable_scene import CannCalculatorScene, DeviceParseScene
from common_func.utils import Utils
from msconfig.config_manager import ConfigManager


class ConfigDataParsers:
    """
    get data parsers from config file
    """

    KEY_PATH = "path"
    KEY_CHIP_MODEL = "chip_model"
    KEY_LEVEL = "level"
    KEY_POSITION = "position"
    POSITION_OF_HOST = "H"
    POSITION_OF_DEVICE = "D"
    DEFAULT_PARSER_LEVEL = "1"

    @classmethod
    def get_parsers(cls: any, config_name: str, chip_model: str, task_flag: bool) -> dict:
        """
        get parsers by chip model
        :param config_name: DataParsersConfig, DataCalculatorConfig
        :param chip_model: 0,1,2
        :param task_flag: whether pmu is task-based and not custom pmu scene
        :return: data parsers
        """
        parsers_dict = {}
        position = cls.POSITION_OF_DEVICE
        if int(InfoConfReader().get_device_list()[0]) == NumberConstant.HOST_ID:
            position = cls.POSITION_OF_HOST
        conf_parser_read = ConfigManager.get(config_name)
        parser_section = conf_parser_read.sections()
        host_cpp_parse_flag = CannCalculatorScene().is_cpp_enable()
        device_cpp_parse_flag = DeviceParseScene().is_cpp_enable()
        all_export_flag = ProfilingScene().is_all_export() and InfoConfReader().is_all_export_version() and task_flag
        for _section in parser_section:
            chip_model_list = cls._load_parser_chip_model(conf_parser_read, _section)
            if chip_model not in chip_model_list:
                continue
            if position not in cls._load_parser_position(conf_parser_read, _section):
                continue
            if host_cpp_parse_flag and cls._load_can_cpp_parse(_section):
                continue
            if device_cpp_parse_flag and all_export_flag and cls._load_can_cpp_parse_or_calculate_device_data(_section):
                continue
            parser_level = cls._load_parser_level(conf_parser_read, _section)
            parsers_dict.setdefault(parser_level, []).append(cls._load_parser_module(conf_parser_read, _section))
        parsers_dict = dict(sorted(parsers_dict.items(), key=lambda x: int(x[0])))
        return parsers_dict

    @classmethod
    def load_conf_file(cls: any, config_file_path: str) -> any:
        """
        load conf file
        :param config_file_path: config file path
        :return: ConfigParser
        """
        check_file_readable(config_file_path)
        conf_parser_read = configparser.ConfigParser()
        conf_parser_read.read(config_file_path)
        return conf_parser_read

    @classmethod
    def _load_parser_level(cls: any, conf_parser_read: any, section: str) -> str:
        """
        get parser level
        :param conf_parser_read:config parser object
        :param section:parser name config in the config file
        :return: parser level
        """
        parser_level = cls.DEFAULT_PARSER_LEVEL
        if conf_parser_read.has_option(section, cls.KEY_LEVEL):
            parser_level = conf_parser_read.get(section, cls.KEY_LEVEL)
        return parser_level

    @classmethod
    def _load_parser_chip_model(cls: any, conf_parser_read: any, section: str) -> list:
        """
        load chip model from per parser
        :param conf_parser_read:config parser object
        :param section:parser name config in the config file
        :return: chip model list
        """
        chip_model_list = []
        if conf_parser_read.has_option(section, cls.KEY_CHIP_MODEL):
            chip_models = conf_parser_read.get(section, cls.KEY_CHIP_MODEL)
            chip_model_list = Utils.generator_to_list(_chip.strip() for _chip in chip_models.strip().split(","))
        return chip_model_list

    @classmethod
    def _load_parser_module(cls: any, conf_parser_read: any, section: str) -> any:
        """
        load data parser
        :param conf_parser_read: config parser object
        :param section: parser name config in the config file
        :return: parsers
        """
        if conf_parser_read.has_option(section, cls.KEY_PATH):
            parser_path = conf_parser_read.get(section, cls.KEY_PATH)
            parser_module = importlib.import_module(parser_path)
            if hasattr(parser_module, section):
                parser_obj = getattr(parser_module, section)
                return parser_obj
        return []

    @classmethod
    def _load_parser_position(cls, conf_parser_read, section):
        """
        load data position
        :param conf_parser_read: config parser object
        :param section: parser name config in the config file
        :return: positions for host or device
        """
        positions = [cls.POSITION_OF_HOST, cls.POSITION_OF_DEVICE]
        if conf_parser_read.has_option(section, cls.KEY_POSITION):
            positions = conf_parser_read.get(section, cls.KEY_POSITION)
        return positions

    @classmethod
    def _load_can_cpp_parse(cls, section) -> bool:
        """
        load can cpp parse
        :param section: parser name config in the config file
        """
        if section in [
            "ApiEventParser",
            "HashDicParser",
            "TaskTrackParser",
            "MemcpyInfoParser",
            "HcclInfoParser",
            "MultiThreadParser",
            "TensorAddInfoParser",
            "FusionAddInfoParser",
            "GraphAddInfoParser",
            "NodeBasicInfoParser",
            "NodeAttrInfoParser",
            "MemoryApplicationParser",
            "CtxIdParser",
            "HcclOpInfoParser",
        ]:
            return True
        return False

    @classmethod
    def _load_can_cpp_parse_or_calculate_device_data(cls, section) -> bool:
        """
        load can cpp parse
        :param section: parser name config in the config file
        """
        if ChipManager().is_chip_v4() and section in [
            "AscendTaskCalculator",
            "StarsLogCalCulator",
            "FftsPmuCalculator",
            "HcclCalculator",
            "SubTaskCalculator",
            "FreqParser",
        ]:
            return True
        return False
