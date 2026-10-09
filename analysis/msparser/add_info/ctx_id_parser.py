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
import sqlite3

from common_func.constant import Constant
from common_func.hash_dict_constant import HashDictData
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.add_info.ctx_id_model import CtxIdModel
from msparser.add_info.ctx_id_bean import CtxIdBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class CtxIdParser(DataParser, MsMultiProcess):
    """
    Ctx Id Data Parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=bad-super-call
        self._file_list = file_list
        self._sample_config = sample_config
        self._ctx_id_data = []
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    @staticmethod
    def _get_ctx_id_data(bean_data: CtxIdBean) -> list:
        if not bean_data:
            return []
        return [
            bean_data.level,
            bean_data.struct_type,
            bean_data.thread_id,
            bean_data.timestamp,
            bean_data.node_id,
            bean_data.ctx_id_num,
            bean_data.ctx_id,
        ]

    def parse(self: any) -> None:
        """
        parse ctx id data
        """
        ctx_id_files = self._file_list.get(DataTag.CTX_ID, [])
        ctx_id_files = self.group_aging_file(ctx_id_files)
        for file_list in ctx_id_files.values():
            self._ctx_id_data.extend(
                self.parse_bean_data(
                    file_list,
                    StructFmt.CTX_ID_SIZE,
                    CtxIdBean,
                    format_func=self._get_ctx_id_data,
                    check_func=self.check_magic_num,
                )
            )

    def save(self: any) -> None:
        """
        save data
        """
        if not self._ctx_id_data:
            return
        model = CtxIdModel(self._project_path)
        with model as _model:
            _model.flush(self._transform_ctx_id_data(self._ctx_id_data))

    def ms_run(self: any) -> None:
        """
        parse and save ctx id data
        :return:
        """
        if not self._file_list.get(DataTag.CTX_ID, []):
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

    def _transform_ctx_id_data(self: any, data_list: list) -> list:
        hash_dict_data = HashDictData(self._project_path)
        type_hash_dict = hash_dict_data.get_type_hash_dict()
        ge_hash_dict = hash_dict_data.get_ge_hash_dict()
        for data in data_list:
            # 1 type hash, 4 node hash
            data[1] = type_hash_dict.get('node', {}).get(data[1], data[1])
            data[4] = ge_hash_dict.get(data[4], data[4])
        return data_list
