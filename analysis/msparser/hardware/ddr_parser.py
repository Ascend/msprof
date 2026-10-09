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
from common_func.file_manager import FileManager, FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from framework.offset_calculator import OffsetCalculator
from msmodel.hardware.ddr_model import DdrModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag


class ParsingDDRData(MsMultiProcess):
    """
    parsing DDR data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list.get(DataTag.DDR, [])
        self.sample_config = sample_config
        self.project_path = self.sample_config.get('result_dir')
        self.calculate = OffsetCalculator(self._file_list, StructFmt.DDR_FMT_SIZE, self.project_path)
        self._model = DdrModel(
            self.project_path, DBNameConstant.DB_DDR, [DBNameConstant.TABLE_DDR_ORIGIN, DBNameConstant.TABLE_DDR_METRIC]
        )
        self.ddr_data = []
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    @staticmethod
    def _update_ddr_data(time_start: int, item: list, headers: list) -> list:
        if len(item) >= 5:  # DDR item length
            item[2], item[3] = (
                'hisi_ddrc{}_0'.format(item[3]),
                'flux{}_{}'.format('id' if item[4] != 2**32 - 1 else '', 'read' if item[2] == 0 else 'write'),
            )
            item[0] = time_start + item[0] * NumberConstant.USTONS
            return headers + item[:4]
        return headers

    def read_binary_data(self: any, file_name: str, device_id: str, replay_id: str = '0') -> int:
        """
        parsing ddr data and insert into ddr.db
        """
        ddr_file = os.path.join(self.sample_config.get("result_dir", ""), 'data', file_name)
        try:
            with FileOpen(ddr_file, 'rb') as ddr_f:
                ddr_data = self.calculate.pre_process(ddr_f.file_reader, os.path.getsize(ddr_file))
                struct_nums = len(ddr_data) // StructFmt.DDR_FMT_SIZE
                struct_data = struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.DDR_FMT * struct_nums, ddr_data)
                start_time = InfoConfReader().get_start_timestamp()
                for i in range(struct_nums):
                    self.ddr_data.append(
                        self._update_ddr_data(
                            start_time, list(struct_data[i * 5 : (i + 1) * 5]), [device_id, replay_id]
                        )
                    )
                return NumberConstant.SUCCESS
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", file_name, err, exc_info=Constant.TRACE_BACK_SWITCH)
            return NumberConstant.ERROR

    def start_parsing_data_file(self: any) -> None:
        """
        parsing data file
        """
        project_path = self.sample_config.get("result_dir", "")
        try:
            for file_name in self._file_list:
                if is_valid_original_data(file_name, project_path):
                    self._original_data_handler(project_path, file_name)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if self.ddr_data and self._model:
            self._model.init()
            self._model.create_table()
            self._model.flush(self.ddr_data)
            self._model.insert_metric_data(self.sample_config.get("ddr_master_id", ""))
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        try:
            if self._file_list:
                self.start_parsing_data_file()
                self.save()
        except (OSError, SystemError, RuntimeError, TypeError, ValueError) as ddr_err:
            logging.error(str(ddr_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _original_data_handler(self: any, project_path: str, file_name: str) -> None:
        device_id = self.sample_config.get("device_id", "0")
        logging.info("start parsing ddr data file: %s", file_name)
        status_ = self.read_binary_data(file_name, device_id, '0')  # replay id is 0
        FileManager.add_complete_file(project_path, file_name)
        if status_:
            logging.error('Insert DDR metric data error.')
        logging.info("Create DDR DB finished!")
