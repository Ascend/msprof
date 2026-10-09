# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from framework.offset_calculator import OffsetCalculator
from msmodel.v5.v5_exeom_model import V5ExeomViewModel
from msmodel.v5.v5_stars_model import V5StarsModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from msparser.v5.v5_stars_bean import V5StarsBean
from profiling_bean.prof_enum.data_tag import DataTag


class V5StarsParser(DataParser, MsMultiProcess):
    """
    parse v5_stars_profile, to get v5 task info
    """

    INVALID_STREAM_ID = -1
    INVALID_BLOCK_NUM = 0

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=bad-super-call
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._stars_profile_data = []

    def parse(self: any) -> None:
        """
        parse function
        """
        v5_stars_file_list = self._file_list.get(DataTag.V5_STARS_PROFILE, [])
        offset_calculator = OffsetCalculator(
            v5_stars_file_list, struct.calcsize(StructFmt.V5_STARS_PROFILE_FMT), self._project_path
        )
        for _file in v5_stars_file_list:
            if not is_valid_original_data(_file, self._project_path):
                continue
            _file_path = self.get_file_path_and_check(_file)
            logging.info("start parsing v5_stars_profile data file: %s", _file)
            self._read_data(_file_path, offset_calculator)

    def save(self: any) -> None:
        """
        save data to db
        :return:
        """
        if self._stars_profile_data:
            stars_profile_model = V5StarsModel(self._project_path)
            with stars_profile_model:
                stars_profile_model.flush(self._reformat_data(self._stars_profile_data), DBNameConstant.TABLE_V5_TASK)

    def ms_run(self: any) -> None:
        """
        entrance for v5 task parser
        :return:
        """
        if not (self._file_list.get(DataTag.V5_STARS_PROFILE, [])):
            return
        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.save()

    def _read_data(self: any, file_path: str, offset: OffsetCalculator) -> None:
        file_size = os.path.getsize(file_path)
        if not file_size:
            return
        struct_size = struct.calcsize(StructFmt.V5_STARS_PROFILE_FMT)
        with FileOpen(file_path, 'rb') as _open_file:
            _all_data = offset.pre_process(_open_file.file_reader, file_size)
            for _index in range(file_size // struct_size):
                data = _all_data[_index * struct_size : (_index + 1) * struct_size]
                self._stars_profile_data.append(V5StarsBean.decode(data))

    def _reformat_data(self: any, data_list: list) -> list:
        """
        format device data,and get stream_id/block_num from host
        """
        res_list = [[]] * len(data_list)
        with V5ExeomViewModel(self._project_path) as _model:
            host_data_dict = _model.get_v5_host_data()
            for i, data in enumerate(data_list):
                start = InfoConfReader().time_from_syscnt(data.timestamp)
                end = InfoConfReader().time_from_syscnt(data.timestamp + data.duration)
                if (data.model_id, data.task_id) not in host_data_dict:
                    # steam_id and block_num
                    host_data = [self.INVALID_STREAM_ID, self.INVALID_BLOCK_NUM]
                    logging.error(
                        "Can not find model_id = %d and task_id = %d in host data info!", data.model_id, data.task_id
                    )
                else:
                    host_data = host_data_dict.get((data.model_id, data.task_id))[0]
                res_list[i] = [
                    data.model_id,
                    host_data[0],
                    data.task_id,
                    data.task_type,
                    start,
                    end,
                    end - start,
                    data.total_cycle,
                    host_data[1],
                ]
                res_list[i].extend(data.pmu_list)
        return res_list
