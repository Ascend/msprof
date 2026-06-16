#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024. All rights reserved.

import logging
import os
import struct

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from framework.offset_calculator import OffsetCalculator
from msmodel.hardware.ub_model import UBModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag


class ParsingUBData(MsMultiProcess):
    """
    parsing UB data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.project_path = sample_config.get("result_dir", "")
        self._file_list = file_list.get(DataTag.UB, [])
        self._calculate = OffsetCalculator(self._file_list, StructFmt.UB_FMT_SIZE, self.project_path)
        self._model = UBModel(self.project_path, DBNameConstant.DB_UB, [DBNameConstant.TABLE_UB_BW])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        self._data_list = []
        self._device_id = "0"

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        try:
            self._device_id = InfoConfReader().get_device_id()
            if self._file_list:
                self.parse()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as ub_err:
            logging.error(str(ub_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def parse(self: any) -> None:
        """
        parsing data file
        """
        for file_name in self._file_list:
            if is_valid_original_data(file_name, self.project_path):
                self._handle_original_data(file_name)

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if self._data_list:
            with self._model as model:
                model.flush(self._data_list)

    def read_binary_data(self: any, file_name: str) -> int:
        """
        parsing ub data and insert into ub.db
        """
        ub_file = PathManager.get_data_file_path(self.project_path, file_name)
        if not os.path.exists(ub_file):
            return NumberConstant.ERROR
        _file_size = os.path.getsize(ub_file)
        with FileOpen(ub_file, "rb") as ub_f:
            ub_data = self._calculate.pre_process(ub_f.file_reader, _file_size)
            struct_nums = _file_size // StructFmt.UB_FMT_SIZE
            struct_data = struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.UB_FMT * struct_nums, ub_data)
            for i in range(struct_nums):
                # do not save Magic Num and reserved fields, and device_id is obtained from InfoConfReader
                self._data_list.append(
                    [self._device_id, struct_data[i * 19 + 2]] + list(struct_data[i * 19 + 4 : i * 19 + 19])
                )
        return NumberConstant.SUCCESS

    def _handle_original_data(self: any, file_name: str) -> None:
        logging.info("start parsing UB data file: %s", file_name)
        status = self.read_binary_data(file_name)
        FileManager.add_complete_file(self.project_path, file_name)
        if status:
            logging.error('Insert UB data error.')
        logging.info("Create UB DB finished!")
