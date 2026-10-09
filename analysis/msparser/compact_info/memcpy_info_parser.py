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
from typing import List

from common_func.hash_dict_constant import HashDictData
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.compact_info.memcpy_info_model import MemcpyInfoModel
from msparser.compact_info.memcpy_info_bean import MemcpyInfoBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class MemcpyInfoParser(DataParser, MsMultiProcess):
    """
    memcpy info data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=bad-super-call
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._memcpy_info_data = []

    def reformat_data(self: any, bean_data: List[MemcpyInfoBean]) -> List[List]:
        """
        transform bean to data
        """
        hash_dict_data = HashDictData(self._project_path)
        type_hash_dict = hash_dict_data.get_type_hash_dict()
        return [
            [
                type_hash_dict.get(bean.level, {}).get(bean.struct_type, bean.struct_type),  # memcpy info type
                bean.level,
                bean.thread_id,
                bean.data_len,
                bean.timestamp,
                bean.data_size,
                bean.direction,
            ]
            for bean in bean_data
        ]

    def save(self: any) -> None:
        """
        save memcpy info data
        """
        if not self._memcpy_info_data:
            return
        with MemcpyInfoModel(self._project_path) as model:
            model.flush(self._memcpy_info_data)

    def parse(self: any) -> None:
        """
        parse memcpy info data
        """
        memcpy_info_files = self._file_list.get(DataTag.MEMCPY_INFO, [])
        memcpy_info_files = self.group_aging_file(memcpy_info_files)
        if not memcpy_info_files:
            return
        bean_data = []
        for files in memcpy_info_files.values():
            bean_data += self.parse_bean_data(
                files,
                StructFmt.MEMCPY_INFO_DATA_SIZE,
                MemcpyInfoBean,
                format_func=lambda x: x,
                check_func=self.check_magic_num,
            )
        self._memcpy_info_data = self.reformat_data(bean_data)

    def ms_run(self: any) -> None:
        """
        parse and save memcpy info data
        :return:
        """
        if not self._file_list.get(DataTag.MEMCPY_INFO, []):
            return
        logging.info("start parsing memcpy info data, files: %s", str(self._file_list.get(DataTag.MEMCPY_INFO, [])))
        self.parse()
        self.save()
