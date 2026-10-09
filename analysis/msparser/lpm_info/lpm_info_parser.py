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
import os
from collections import defaultdict

from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager, FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from common_func.info_conf_reader import InfoConfReader
from msmodel.freq.freq_parser_model import FreqParserModel
from msmodel.voltage.voltage_parser_model import AicVoltageParserModel
from msmodel.voltage.voltage_parser_model import BusVoltageParserModel
from framework.offset_calculator import OffsetCalculator
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.lpm_info_bean import LpmInfoDataBean


class LpmInfoConvParser(IParser, MsMultiProcess):
    """
    LpmInfoConv data parser
    The LpmInfoConv dataset comprises three data types: AicFreq data, AicVoltage data, and BusVoltage data.
    """

    LPM_INFO_TYPE_FREQ = 0
    LPM_INFO_TYPE_AIC_VOL = 1
    LPM_INFO_TYPE_BUS_VOL = 2

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._file_list = file_list.get(DataTag.LPM_INFO, [])
        self._calculate = OffsetCalculator(self._file_list, StructFmt.LPM_INFO_DATA_SIZE, self._project_path)
        self._model_dict = {
            self.LPM_INFO_TYPE_FREQ: FreqParserModel(self._project_path, [DBNameConstant.TABLE_FREQ_PARSE]),
            self.LPM_INFO_TYPE_AIC_VOL: AicVoltageParserModel(self._project_path),
            self.LPM_INFO_TYPE_BUS_VOL: BusVoltageParserModel(self._project_path),
        }
        self._lpm_conv_data = defaultdict(list)

    def add_lmp_info_conv_data_to_data_list(self: any, single_bean: LpmInfoDataBean) -> any:
        """
        add_lmp_info_conv_data_to_data_list
        :param single_bean: LpmInfoDataBean
        :return: None
        """
        dev_cnt = InfoConfReader().get_dev_cnt()
        if not self._lpm_conv_data.get(single_bean.type, []):
            self._lpm_conv_data[single_bean.type] = [
                [
                    dev_cnt,
                    (InfoConfReader().get_freq(StrConstant.AIC) / NumberConstant.FREQ_TO_MHz)
                    if single_bean.type == self.LPM_INFO_TYPE_FREQ
                    else NumberConstant.DEFAULT_VOLTAGE,
                ]
            ]
        for lpm_data in single_bean.lpm_data:
            if lpm_data.syscnt < dev_cnt:
                self._lpm_conv_data.get(single_bean.type)[0][1] = lpm_data.value
                continue
            self._lpm_conv_data.get(single_bean.type).append([lpm_data.syscnt, lpm_data.value])

    def read_binary_data(self: any, file_name):
        """
        parsing lpm info binary data
        """
        files = PathManager.get_data_file_path(self._project_path, file_name)
        if not os.path.exists(files):
            return NumberConstant.ERROR
        _file_size = os.path.getsize(files)
        if _file_size < StructFmt.LPM_INFO_DATA_SIZE:
            logging.error("Lpm info data struct is incomplete, please check the file.")
            return NumberConstant.ERROR
        with FileOpen(files, "rb") as f:
            lpm_info_bin = self._calculate.pre_process(f.file_reader, _file_size)
            struct_nums = _file_size // StructFmt.LPM_INFO_DATA_SIZE
            for i in range(struct_nums):
                single_bean = LpmInfoDataBean().decode(
                    lpm_info_bin[i * StructFmt.LPM_INFO_DATA_SIZE : (i + 1) * StructFmt.LPM_INFO_DATA_SIZE]
                )
                if single_bean:
                    self.add_lmp_info_conv_data_to_data_list(single_bean)
        return NumberConstant.SUCCESS

    def parse(self: any) -> None:
        """
        parse the data under the file path
        :return: NA
        """
        logging.info("Start parsing Lpm info data file.")
        for file_name in self._file_list:
            if is_valid_original_data(file_name, self._project_path):
                self._handle_original_data(file_name)
        logging.info("Parsing Lpm info data finished!")

    def save(self: any) -> None:
        """
        save the result of data parsing
        :return: NA
        """
        for key, data in self._lpm_conv_data.items():
            if data:
                with self._model_dict.get(key) as model:
                    model.flush(data)

    def ms_run(self: any) -> None:
        """
        main entry
        """
        if not self._file_list:
            return
        if self._file_list:
            logging.info("start parsing lpm data, files: %s", str(self._file_list))
            self.parse()
            self.save()

    def _handle_original_data(self: any, file_name: str) -> None:
        if self.read_binary_data(file_name) == NumberConstant.ERROR:
            logging.error('Parse lpm info data file: %s error.', file_name)
        FileManager.add_complete_file(self._project_path, file_name)
