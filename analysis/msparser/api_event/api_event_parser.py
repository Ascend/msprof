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
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from framework.offset_calculator import OffsetCalculator
from msmodel.api.api_data_model import ApiDataModel
from msmodel.event.event_data_model import EventDataModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.api_data_bean import ApiDataBean
from profiling_bean.struct_info.event_data_bean import EventDataBean


class ApiEventParser(DataParser, MsMultiProcess):
    """
    api_event data parser
    """

    CHECK_RESERVE = 18446744073709551615  # 0xFFFF FFFF FFFF FFFF
    connection_id = 0

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=bad-super-call
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._event_data = []
        self._api_data = []
        self.invalid_api_num = 0

    def parse(self: any) -> None:
        """
        parse function
        """
        api_event_tag_files = self._file_list.get(DataTag.API_EVENT, [])
        api_event_tag_files = self.group_aging_file(api_event_tag_files)
        for file_list in api_event_tag_files.values():
            offset_calculator = OffsetCalculator(
                file_list, struct.calcsize(StructFmt.API_EVENT_FMT), self._project_path
            )
            for _file in file_list:
                if not is_valid_original_data(_file, self._project_path):
                    continue
                _file_path = self.get_file_path_and_check(_file)
                logging.info("start parsing api_event data file: %s", _file)
                self._read_data(_file_path, offset_calculator)

    def save(self: any) -> None:
        """
        save data to db
        :return:
        """
        if self._event_data:
            event_model = EventDataModel(self._project_path)
            with event_model:
                event_model.flush(self._event_data)
        if self._api_data:
            api_model = ApiDataModel(self._project_path)
            with api_model:
                api_model.flush(self._api_data)

    def ms_run(self: any) -> None:
        """
        entrance for api_event parser
        :return:
        """
        if not (self._file_list.get(DataTag.API_EVENT, [])):
            return
        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        if self.invalid_api_num:
            logging.warning(
                "A total of %d api data entries have a start time of 0, or are duplicated.", self.invalid_api_num
            )
        self.save()

    def _check_reserve_num(self, data: bytes) -> bool:
        reserve = struct.unpack("=Q", data[24:32])
        return reserve[0] == self.CHECK_RESERVE

    def _decode_data(self, data: bytes):
        if self._check_reserve_num(data):
            self._event_data.append((self.connection_id, EventDataBean.decode(data)))
        else:
            api_data = ApiDataBean.decode(data)
            if api_data.start == 0:
                self.invalid_api_num += 1
                return
            self._api_data.append((self.connection_id, api_data))
        self.connection_id += 1

    def _read_data(self: any, file_path: str, offset: OffsetCalculator) -> None:
        file_size = os.path.getsize(file_path)
        if not file_size:
            return
        struct_size = struct.calcsize(StructFmt.API_EVENT_FMT)
        with FileOpen(file_path, 'rb') as _open_file:
            _all_data = offset.pre_process(_open_file.file_reader, file_size)
            for _index in range(file_size // struct_size):
                data = _all_data[_index * struct_size : (_index + 1) * struct_size]
                self.check_magic_num(data)
                self._decode_data(data)
