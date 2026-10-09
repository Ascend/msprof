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
from msmodel.hardware.qos_model import QosModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag


class ParsingQosData(MsMultiProcess):
    """
    parsing QoS data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.qos_data = []
        self.sample_config = sample_config
        self._file_list = file_list.get(DataTag.QOS, [])
        self.project_path = sample_config.get("result_dir", "")
        self.calculate = OffsetCalculator(self._file_list, StructFmt.QOS_FMT_SIZE, self.project_path)
        self._model = QosModel(self.project_path, DBNameConstant.DB_QOS, [DBNameConstant.TABLE_QOS_BW])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def read_binary_data(self: any, file_name: str) -> int:
        """
        parsing qos data and insert into qos.db
        """
        status = NumberConstant.ERROR
        qos_file = PathManager.get_data_file_path(self.project_path, file_name)
        if not os.path.exists(qos_file):
            return status
        _file_size = os.path.getsize(qos_file)
        with FileOpen(qos_file, "rb") as qos_f:
            qos_data = self.calculate.pre_process(qos_f.file_reader, _file_size)
            struct_nums = _file_size // StructFmt.QOS_FMT_SIZE
            struct_data = struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.QOS_FMT * struct_nums, qos_data)
            for i in range(struct_nums):
                timestamp = InfoConfReader().time_from_syscnt(struct_data[i * 14 + 3])
                # 这里补齐一个None是因为qos.data里面没有die id,但是想共用一张表
                self.qos_data.append([timestamp, None] + list(struct_data[i * 14 + 4 : (i + 1) * 14]))
        return NumberConstant.SUCCESS

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
        if self.qos_data:
            with self._model as model:
                model.flush(self.qos_data)

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        try:
            if self._file_list:
                self.parse()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as qos_err:
            logging.error(str(qos_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _handle_original_data(self: any, file_name: str) -> None:
        logging.info("start parsing QoS data file: %s", file_name)
        status = self.read_binary_data(file_name)
        FileManager.add_complete_file(self.project_path, file_name)
        if status:
            logging.error('Insert QoS bandwidth data error.')
        logging.info("Create QoS DB finished!")
