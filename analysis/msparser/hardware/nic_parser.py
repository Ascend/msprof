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

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from common_func.utils import Utils
from msconfig.config_manager import ConfigManager
from msmodel.hardware.nic_model import NicModel
from profiling_bean.prof_enum.data_tag import DataTag


class ParsingNicData(MsMultiProcess):
    """
    parsing peripheral data class
    """

    DEFAULT_NIC_FUNC_ID = 0
    TABLE_PATH = ConfigManager.TABLES
    NETWORK_HEADER_TAG = 'rxPacket/s'

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.project_path = sample_config.get("result_dir", "")
        self._file_list = file_list.get(DataTag.NIC, [])
        self.curr_file_name = ''
        self.nic_data = []
        self.device_id = self.sample_config.get("device_id", "0")
        self.replay_id = Constant.DEFAULT_REPLAY
        self._model = NicModel(self.project_path, DBNameConstant.DB_NIC_ORIGIN, [DBNameConstant.TABLE_NIC_ORIGIN])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def read_binary_data(self: any, binary_data_path: str) -> None:
        """
        read binary data file
        """
        file_path = PathManager.get_data_file_path(self.project_path, binary_data_path)
        try:
            with FileOpen(file_path, "r") as _file:
                first_line_data = _file.file_reader.readline(Constant.MAX_READ_LINE_BYTES)
                if not first_line_data:
                    logging.warning("No network data Found!")
                    return
                has_header = False
                first_line_data_lst = list(first_line_data.split())
                if self.NETWORK_HEADER_TAG in first_line_data_lst:
                    has_header = True
                network_list = (list(x.split()) for x in _file.file_reader.readlines(Constant.MAX_READ_FILE_BYTES))
                network_data = Utils.generator_to_list(network_list)
                if not has_header:
                    network_data.insert(0, first_line_data_lst)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as error:
            logging.error("%s: %s", binary_data_path, str(error), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        if not network_data:
            logging.warning("No network data Found!")
            return
        self._insert_nic_data(network_data)

    def start_parsing_data_file(self: any) -> None:
        """
        start parsing data file
        """
        try:
            for file_name in self._file_list:
                if not is_valid_original_data(file_name, self.project_path):
                    continue
                logging.info("start parsing data file: %s", file_name)
                if file_name.startswith('nic'):
                    self.read_binary_data(file_name)
                    FileManager.add_complete_file(self.project_path, file_name)
                    logging.info("Create NIC DB finished!")
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as error:
            logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save data into database
        :return: None
        """
        if self.nic_data and self._model:
            self._model.init()
            self._model.create_table()
            self._model.flush(self.nic_data)
            self._model.report_data([self.device_id])
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        run function
        """
        try:
            if self._file_list:
                self.start_parsing_data_file()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as error:
            logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)

    def _insert_nic_data(self: any, network_data: list) -> None:
        nic_header_num = DBManager.get_table_field_num(DBNameConstant.TABLE_NIC_ORIGIN + "Map", self.TABLE_PATH)
        # chip 0 nic diff, exclude device_id and replay_id
        has_type_id = False  # 标识是否具有typeid
        has_port_id = True  # 标识是否具有portid
        if len(network_data) > 0:
            if nic_header_num < len(network_data[0]) + 2:
                has_type_id = True
            elif nic_header_num > len(network_data[0]) + 2:
                has_port_id = False
        for nd in network_data:
            item = [self.device_id, self.replay_id, float(nd[0].replace(":", ''))]
            if has_type_id:
                item.extend(nd[1:-1])
            elif not has_port_id:
                item.extend(nd[1:])
                item.append(self.DEFAULT_NIC_FUNC_ID)
            else:
                item.extend(nd[1:])
            self.nic_data.append(tuple(item))
