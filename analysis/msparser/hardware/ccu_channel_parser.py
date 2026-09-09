# pylint: disable=duplicate-code
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

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from msmodel.hardware.ccu_channel_model import CcuChannelModel
from msparser.ccu_decoder_registry import CcuDecoderRegistry
from msparser.ccu_source_calculator import CcuSourceCalculator
from msparser.ccu_source_calculator import ccu_table_exists
from msparser.ccu_source_calculator import mark_ccu_files_complete
from profiling_bean.hardware.ccu_profile import CcuDataKind
from profiling_bean.hardware.ccu_profile import CcuHardwareProfileRegistry
from profiling_bean.prof_enum.data_tag import DataTag


class CCUChannelParser(MsMultiProcess):
    """
    class used to parser ccu channel data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._file_list = file_list.get(DataTag.CCU_CHANNEL, [])
        self._project_path = sample_config.get("result_dir", "")
        self._profile = CcuHardwareProfileRegistry.require_profile(
            sample_config.get(StrConstant.SAMPLE_CONFIG_CHIP_MODEL)
        )
        self._decoder = CcuDecoderRegistry.get_decoder(self._profile, CcuDataKind.CHANNEL)
        self._source_calculator = CcuSourceCalculator(
            self._file_list, self._profile, CcuDataKind.CHANNEL, self._decoder.record_size, self._project_path
        )
        self._model = CcuChannelModel(self._project_path, DBNameConstant.DB_CCU, [DBNameConstant.TABLE_CCU_CHANNEL])
        self.channel_data = []
        self._pending_complete_files = []
        self._parse_failed = False

    def read_binary_data(self: any, file_name: str) -> int:
        """
        parsing ccu channel data
        """
        files = PathManager.get_data_file_path(self._project_path, file_name)
        if not os.path.exists(files):
            return NumberConstant.ERROR
        _file_size = os.path.getsize(files)
        if _file_size <= 0:
            logging.error("CCU channel data file is empty, please check the file.")
            return NumberConstant.ERROR
        try:
            self._profile.get_source_scope(CcuDataKind.CHANNEL, file_name)
        except ValueError as error:
            logging.error(str(error))
            return NumberConstant.ERROR
        with FileOpen(files, "rb") as f:
            channel_bin = self._source_calculator.pre_process(file_name, f.file_reader, _file_size)
            struct_nums = len(channel_bin) // self._decoder.record_size
            for i in range(struct_nums):
                single_bean = self._decoder.decode(
                    channel_bin[i * self._decoder.record_size : (i + 1) * self._decoder.record_size]
                )
                if single_bean and len(single_bean.channels_bw_data) == self._profile.channel_count:
                    self.channel_data.extend(single_bean.channels_bw_data)
        return NumberConstant.SUCCESS

    def parse(self: any) -> None:
        """
        parsing data file
        """
        logging.info("Start parsing CCU channel data file.")
        for file_name in self._file_list:
            if is_valid_original_data(file_name, self._project_path):
                self._handle_original_data(file_name)
        self._source_calculator.validate()
        logging.info("Create CCU channel table finished!")

    def save(self: any) -> bool:
        """
        save data to db
        :return: None
        """
        if self._parse_failed:
            return False
        if self._source_calculator.has_completed_files and not ccu_table_exists(
            self._project_path, DBNameConstant.DB_CCU, DBNameConstant.TABLE_CCU_CHANNEL
        ):
            logging.error("Completed CCU channel source has no persisted table; clear markers and reimport")
            return False
        save_success = True
        if self._pending_complete_files or self.channel_data:
            if any(row[1] > NumberConstant.INT64_MAX for row in self.channel_data):
                logging.error("CCU channel timestamp exceeds the SQLite integer range")
                return False
            with self._model as model:
                save_success = model.flush(self.channel_data, DBNameConstant.TABLE_CCU_CHANNEL)
        if save_success:
            save_success = mark_ccu_files_complete(
                self._project_path, self._pending_complete_files, FileManager.add_complete_file
            )
        if save_success:
            self._pending_complete_files.clear()
        return save_success

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        try:
            if self._file_list:
                self.parse()
                if not self.save():
                    logging.error("Save CCU channel data failed; source files remain incomplete.")
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _handle_original_data(self: any, file_name: str) -> None:
        if self.read_binary_data(file_name) == NumberConstant.ERROR:
            logging.error('Parse CCU channel data file: %s error.', file_name)
            self._parse_failed = True
            return
        self._pending_complete_files.append(file_name)
