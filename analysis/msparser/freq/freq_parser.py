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

from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager, FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.utils import Utils
from common_func.info_conf_reader import InfoConfReader
from framework.offset_calculator import OffsetCalculator
from msmodel.freq.freq_parser_model import FreqParserModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.freq import FreqLpmConvBean


class FreqParser(IParser, MsMultiProcess):
    """
    frequency data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._file_list = file_list
        self._freq_data = []

    def save(self: any) -> None:
        """
        save the result of freq data parsing
        :return: NA
        """
        if not self._freq_data:
            return
        with FreqParserModel(self._project_path, [DBNameConstant.TABLE_FREQ_PARSE]) as model:
            model.flush(self._freq_data)

    def parse(self: any) -> None:
        """
        parse the data under the file path
        :return: NA
        """
        self._freq_data = [
            [InfoConfReader().get_dev_cnt(), InfoConfReader().get_freq(StrConstant.AIC) / NumberConstant.FREQ_TO_MHz]
        ]
        freq_files = self._file_list.get(DataTag.FREQ)
        freq_files.sort(key=lambda x: int(x.split("_")[-1]))
        offset_calculator = OffsetCalculator(freq_files, StructFmt.FREQ_DATA_SIZE, self._project_path)
        for _file in freq_files:
            _file_path = PathManager.get_data_file_path(self._project_path, _file)
            _file_size = os.path.getsize(_file_path)
            if not _file_size:
                return
            self._read_file(_file_path, _file_size, offset_calculator)

    def ms_run(self: any) -> None:
        """
        main entry
        """
        if not self._file_list:
            return
        if self._file_list.get(DataTag.FREQ):
            logging.info("start parsing frequency data, files: %s", str(self._file_list.get(DataTag.FREQ)))
            self.parse()
            self.save()

    def _read_file(self: any, _file_path: str, _file_size: int, offset_calculator: OffsetCalculator) -> None:
        freq_data_bean = FreqLpmConvBean()
        dev_cnt = InfoConfReader().get_dev_cnt()
        with FileOpen(_file_path, 'rb') as file_reader:
            _all_freq_data = offset_calculator.pre_process(file_reader.file_reader, _file_size)
        freq_data_chunks = Utils.chunks(_all_freq_data, StructFmt.FREQ_DATA_SIZE)
        for freq_data in freq_data_chunks:
            freq_data_bean.decode(freq_data)
            for lpm_data in freq_data_bean.lpm_data:
                if lpm_data.syscnt < dev_cnt:
                    self._freq_data[0][1] = lpm_data.value
                    continue
                self._freq_data.append([lpm_data.syscnt, lpm_data.value])
        FileManager.add_complete_file(self._project_path, _file_path)
