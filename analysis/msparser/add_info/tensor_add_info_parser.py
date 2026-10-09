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
from msmodel.add_info.tensor_add_info_model import TensorAddInfoModel
from msparser.add_info.tensor_add_info_bean import TensorAddInfoBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class TensorAddInfoParser(DataParser, MsMultiProcess):
    """
    ge tensor info data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=bad-super-call
        self._sample_config = sample_config
        self._file_list = file_list
        self._ge_tensor_info_data = []
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    @staticmethod
    def _get_tensor_info_data(bean_data: any) -> list:
        if not bean_data:
            return []
        return [
            bean_data.level,
            bean_data.struct_type,
            bean_data.thread_id,
            bean_data.timestamp,
            bean_data.node_id,
            bean_data.tensor_num,
            bean_data.input_format,
            bean_data.input_data_type,
            bean_data.input_shape,
            bean_data.output_format,
            bean_data.output_data_type,
            bean_data.output_shape,
        ]

    @classmethod
    def _generate_new_hash_dict_data(cls: any, hash_dict: dict, key: any, data: list) -> None:
        hash_dict[key] = {
            'level': data[0],
            'add_info_type': data[1],
            'thread_id': data[2],
            'timestamp': data[3],
            'node_id': data[4],
            'tensor_num': 0,
            'input_format': [],
            'input_data_type': [],
            'input_shape': [],
            'output_format': [],
            'output_data_type': [],
            'output_shape': [],
        }
        cls._update_hash_dict_data(hash_dict, key, data)

    @classmethod
    def _update_hash_dict_data(cls: any, hash_dict: dict, key: any, data: list) -> None:
        value = hash_dict.get(key, {})
        value['tensor_num'] += data[5]
        if data[6] or data[7] or data[8]:
            value['input_format'].append(str(data[6]) if data[6] else '')
            value['input_data_type'].append(str(data[7]) if data[7] else '')
            value['input_shape'].append(str(data[8]) if data[8] else '')
        if data[9] or data[10] or data[11]:
            value['output_format'].append(str(data[9]) if data[9] else '')
            value['output_data_type'].append(str(data[10]) if data[10] else '')
            value['output_shape'].append(str(data[11]) if data[11] else '')
        hash_dict[key] = value

    def parse(self: any) -> None:
        """
        parse ge tensor info data
        """
        tensor_info_files = self._file_list.get(DataTag.TENSOR_ADD_INFO, [])
        tensor_info_files = self.group_aging_file(tensor_info_files)
        for file_list in tensor_info_files.values():
            self._ge_tensor_info_data.extend(
                self.parse_bean_data(
                    file_list,
                    StructFmt.TENSOR_ADD_INFO_SIZE,
                    TensorAddInfoBean,
                    format_func=self._get_tensor_info_data,
                    check_func=self.check_magic_num,
                )
            )
        self._update_tensor_data()

    def save(self: any) -> None:
        """
        save data
        """
        if not self._ge_tensor_info_data:
            return
        model = TensorAddInfoModel(self._project_path)
        with model as _model:
            _model.flush(self._transform_tensor_info_data(self._ge_tensor_info_data))

    def ms_run(self: any) -> None:
        """
        parse and save ge tensor info data
        :return:
        """
        if not self._file_list.get(DataTag.TENSOR_ADD_INFO, []):
            logging.info("No ge tensor info data, exit without parser running.")
            return
        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        try:
            self.save()
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _update_tensor_data(self: any) -> None:
        hash_dict = {}
        for data in self._ge_tensor_info_data:
            if len(data) < 12:
                continue
            # 3 timestamp 2 thread_id 4 node_id
            key = (str(data[3]), str(data[2]), str(data[4]))
            if key not in hash_dict:
                self._generate_new_hash_dict_data(hash_dict, key, data)
            else:
                self._update_hash_dict_data(hash_dict, key, data)
        self._assemble_tensor_data(hash_dict)

    def _assemble_tensor_data(self: any, hash_dict: dict) -> None:
        self._ge_tensor_info_data = []
        for key, value in hash_dict.items():
            timestamp, thread_id, node_id = key
            self._ge_tensor_info_data.append(
                [
                    value['level'],
                    value['add_info_type'],
                    thread_id,
                    timestamp,
                    node_id,
                    value['tensor_num'],
                    ";".join(value['input_format']) if value['input_format'] else "N/A",
                    ";".join(value['input_data_type']) if value['input_data_type'] else "N/A",
                    "\"" + ";".join(value['input_shape']) + "\"" if value['input_shape'] else "N/A",
                    ";".join(value['output_format']) if value['output_format'] else "N/A",
                    ";".join(value['output_data_type']) if value['output_data_type'] else "N/A",
                    "\"" + ";".join(value['output_shape']) + "\"" if value['output_shape'] else "N/A",
                ]
            )

    def _transform_tensor_info_data(self: any, data_list: list) -> list:
        hash_dict_data = HashDictData(self._project_path)
        type_hash_dict = hash_dict_data.get_type_hash_dict()
        ge_hash_dict = hash_dict_data.get_ge_hash_dict()
        for data in data_list:
            # 1 type hash, 4 node hash
            data[1] = type_hash_dict.get('node', {}).get(data[1], data[1])
            data[4] = ge_hash_dict.get(data[4], data[4])
        return data_list
