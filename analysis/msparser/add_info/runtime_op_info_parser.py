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

import logging
import os
from typing import Union

from common_func.constant import Constant
from common_func.file_manager import FileOpen
from common_func.hash_dict_constant import HashDictData
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from msmodel.add_info.runtime_op_info_model import RuntimeOpInfoModel
from msparser.add_info.runtime_op_info_bean import RuntimeTensorBean
from msparser.add_info.runtime_op_info_bean import RuntimeOpInfoBean
from msparser.add_info.runtime_op_info_bean import RuntimeOpInfo256Bean
from msparser.compact_info.node_basic_info_parser import NodeBasicInfoParser
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class RuntimeOpInfoParser(DataParser, MsMultiProcess):
    """
    parse runtime op info
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=E1003
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._runtime_op_info_data = []
        self._256_size = 0
        self._var_size = 0

    def parse(self: any) -> None:
        """
        parse function
        """
        fixed_length, variable_length = self._group_var_file()
        self._parse_for_256_data(fixed_length)
        self._parse_for_var_data(variable_length)

    def save(self: any) -> None:
        """
        save data to db
        :return:
        """
        if not self._runtime_op_info_data:
            return
        logging.info(
            "256 size is %d, var size is %d, actually size is %d",
            self._256_size,
            self._var_size,
            len(self._runtime_op_info_data),
        )
        with RuntimeOpInfoModel(self._project_path) as _model:
            _model.flush(self._runtime_op_info_data)

    def ms_run(self: any) -> None:
        """
        entrance for runtime op info parser
        :return:
        """
        if not self._file_list.get(DataTag.RUNTIME_OP_INFO, []):
            return
        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.save()

    def _assemble_data(
        self: any,
        mode: str,
        body: Union[RuntimeOpInfoBean, RuntimeOpInfo256Bean],
        tensor: Union[RuntimeTensorBean, RuntimeOpInfo256Bean],
    ):
        hash_dict_data = HashDictData(self._project_path)
        type_hash_dict = hash_dict_data.get_type_hash_dict()
        ge_hash_dict = hash_dict_data.get_ge_hash_dict()
        struct_type = type_hash_dict.get('runtime', {}).get(body.struct_type, body.struct_type)
        op_name = ge_hash_dict.get(body.node_id, body.node_id)
        op_type = ge_hash_dict.get(body.op_type, body.op_type)
        op_state = NodeBasicInfoParser.get_op_state(mode)
        self._runtime_op_info_data.append(
            [
                body.level,
                struct_type,
                body.thread_id,
                body.timestamp,
                body.device_id,
                body.model_id,
                body.stream_id,
                body.task_id,
                op_name,
                body.task_type,
                op_type,
                body.hash_id,
                body.block_num,
                body.mix_block_num,
                body.op_flag,
                op_state,
                body.tensor_num,
                tensor.input_format if tensor.input_format else Constant.NA,
                tensor.input_data_type if tensor.input_data_type else Constant.NA,
                "\"" + tensor.input_shape + "\"" if tensor.input_shape else Constant.NA,
                tensor.output_format if tensor.output_format else Constant.NA,
                tensor.output_data_type if tensor.output_data_type else Constant.NA,
                "\"" + tensor.output_shape + "\"" if tensor.output_shape else Constant.NA,
            ]
        )

    def _read_data(self: any, mode: str, file_path: str) -> None:
        offset = 0
        file_size = os.path.getsize(file_path)
        with FileOpen(file_path, 'rb') as _open_file:
            _all_data = _open_file.file_reader.read(file_size)
        while offset < file_size:
            middle = offset + StructFmt.RUNTIME_OP_INFO_BODY_SIZE
            self.check_magic_num(_all_data[offset:middle])
            self._var_size = self._var_size + 1
            body = RuntimeOpInfoBean.decode(_all_data[offset:middle])

            # dataLen 描述 24 字节公共头之后的长度。能覆盖「固定体 + tensor」则合法，
            # 解 tensor 只用 tensor_num * TENSOR_SIZE；记录跳转始终以 dataLen 为准。
            header_size = StructFmt.RUNTIME_OP_INFO_BODY_SIZE - StructFmt.RUNTIME_OP_INFO_WITHOUT_HEAD_SIZE
            tensor_bytes = body.tensor_num * StructFmt.RUNTIME_OP_INFO_TENSOR_SIZE
            needed_after_header = StructFmt.RUNTIME_OP_INFO_WITHOUT_HEAD_SIZE + tensor_bytes
            record_end = offset + header_size + body.data_len
            if body.data_len < needed_after_header or record_end > file_size:
                logging.error("data_len error: data_len is %d, tensor num is %d", body.data_len, body.tensor_num)
                offset = record_end
                continue

            tensor = RuntimeTensorBean().decode(
                _all_data[middle : middle + tensor_bytes], StructFmt.RUNTIME_OP_INFO_TENSOR_FMT, body.tensor_num
            )
            self._assemble_data(mode, body, tensor)
            offset = record_end

    def _parse_for_var_data(self, parser_files: list):
        op_info_files = self.group_aging_file(parser_files)
        for mode, _file in op_info_files.items():
            for file_path in _file:
                if not is_valid_original_data(file_path, self._project_path):
                    continue
                _file_path = self.get_file_path_and_check(file_path)
                logging.info("start parsing runtime op info data file: %s", file_path)
                self._read_data(mode, _file_path)

    def _parse_for_256_data(self, parser_files: list):
        op_info_files = self.group_aging_file(parser_files)
        op_info_data = {}
        for mode, file_list in op_info_files.items():
            op_info_data[mode] = self.parse_bean_data(
                file_list,
                StructFmt.RUNTIME_OP_INFO_256_SIZE,
                RuntimeOpInfo256Bean,
                format_func=lambda x: x,
                check_func=self.check_magic_num,
            )
        for mode, op_infos in op_info_data.items():
            self._256_size = self._256_size + len(op_infos)
            for op_info in op_infos:
                self._assemble_data(mode, op_info, op_info)

    def _group_var_file(self):
        fixed_list = []
        variable_list = []
        for file_name in self._file_list.get(DataTag.RUNTIME_OP_INFO, []):
            if "variable" in file_name:
                variable_list.append(file_name)
            elif "additional" in file_name:
                fixed_list.append(file_name)
        return fixed_list, variable_list
