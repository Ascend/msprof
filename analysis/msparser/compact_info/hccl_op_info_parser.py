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
# pylint: skip-file

import logging
import sqlite3
from typing import List

from common_func.constant import Constant
from common_func.hash_dict_constant import HashDictData
from common_func.hccl_info_common import trans_enum_name, DataType
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.compact_info.hccl_op_info_model import HcclOpInfoModel
from msparser.compact_info.hccl_op_info_bean import HcclOpInfoBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class HcclOpInfoParser(DataParser, MsMultiProcess):
    """
    Hccl op Info Data Parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self._hccl_op_info_data = []
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    def reformat_data(self: any, bean_data: List[HcclOpInfoBean]) -> None:
        """
        transform bean to data
        """
        type_info_data = HashDictData(self._project_path).get_type_hash_dict().get("node", {})
        ge_hash_dict = HashDictData(self._project_path).get_ge_hash_dict()
        self._hccl_op_info_data = []
        for data in bean_data:
            data_type = trans_enum_name(DataType, data.data_type)
            self._hccl_op_info_data.append(
                [
                    data.level,
                    type_info_data.get(data.struct_type, data.struct_type),
                    data.thread_id,
                    data.timestamp,
                    data.relay,
                    data.retry,
                    data_type,
                    ge_hash_dict.get(data.alg_type, data.alg_type),
                    data.count,
                    data.group_name,
                ]
            )

    def save(self: any) -> None:
        """
        save hccl information parser data to db
        :return: None
        """
        if not self._hccl_op_info_data:
            return
        with HcclOpInfoModel(self._project_path) as model:
            model.flush(self._hccl_op_info_data)

    def parse(self: any) -> None:
        """
        parse hccl op info data
        """
        hccl_op_info_files = self._file_list.get(DataTag.HCCL_OP_INFO, [])
        hccl_op_info_files = self.group_aging_file(hccl_op_info_files)
        if not hccl_op_info_files:
            return
        bean_data = []
        for files in hccl_op_info_files.values():
            bean_data += self.parse_bean_data(
                files,
                StructFmt.HCCL_OP_INFO_SIZE,
                HcclOpInfoBean,
                format_func=lambda x: x,
                check_func=self.check_magic_num,
            )
        self.reformat_data(bean_data)

    def ms_run(self: any) -> None:
        """
        parse and save hccl op info data
        :return:
        """
        if not self._file_list.get(DataTag.HCCL_OP_INFO, []):
            return
        try:
            self.parse()
        except (OSError, IOError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        try:
            self.save()
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
