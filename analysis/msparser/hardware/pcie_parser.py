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

# pylint: disable=duplicate-code

import logging
import os
import struct

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from framework.offset_calculator import OffsetCalculator
from msmodel.hardware.pcie_model import PcieModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag


class ParsingPcieData(MsMultiProcess):
    """
    parsing PCIe data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.pcie_data = []
        self._file_list = file_list.get(DataTag.PCIE, [])
        self.project_path = sample_config.get("result_dir", "")
        self._model = PcieModel(self.project_path, DBNameConstant.DB_PCIE, [DBNameConstant.TABLE_PCIE])
        self.calculate = OffsetCalculator(self._file_list, StructFmt.PCIE_FMT_SIZE, self.project_path)
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def read_binary_data(self: any, file_name: str, device_id: str) -> int:
        """
        read binary data
        """
        device_id_index = 1
        pcie_file = PathManager.get_data_file_path(self.project_path, file_name)
        if not os.path.exists(pcie_file):
            return NumberConstant.ERROR
        _file_size = os.path.getsize(pcie_file)
        try:
            with FileOpen(pcie_file, "rb") as pcie_:
                pcie_data = self.calculate.pre_process(pcie_.file_reader, _file_size)
                struct_nums = _file_size // StructFmt.PCIE_FMT_SIZE
                struct_data = struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.PCIE_FMT * struct_nums, pcie_data)
                binary_data_index = Constant.DEFAULT_COUNT
                for _ in range(struct_nums):
                    # pcie data structure has 23 parts
                    tmp = list(struct_data[binary_data_index : binary_data_index + 23])
                    binary_data_index += 23
                    tmp[device_id_index] = device_id
                    self.pcie_data.append(tmp)
            return NumberConstant.SUCCESS
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", file_name, err, exc_info=Constant.TRACE_BACK_SWITCH)
            return NumberConstant.ERROR

    def start_parsing_data_file(self: any) -> None:
        """
        start parsing pcie data
        """
        try:
            for file_name in self._file_list:
                if not is_valid_original_data(file_name, self.project_path):
                    continue
                device_id = self.sample_config.get("device_id", "0")
                logging.info("start parsing PCIe data file: %s", file_name)
                status = self.read_binary_data(file_name, device_id)
                if status:
                    logging.error('Insert PCIe data error.')
                    return
                FileManager.add_complete_file(self.project_path, file_name)
                logging.info("Create PCIe DB finished!")
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save data
        """
        if self.pcie_data and self._model:
            self._model.init()
            self._model.create_table()
            self._model.flush(self.pcie_data)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        run function
        """
        try:
            if self._file_list:
                self.start_parsing_data_file()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as pcie_err:
            logging.error(str(pcie_err), exc_info=Constant.TRACE_BACK_SWITCH)
