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

from common_func.hash_dict_constant import HashDictData
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.compact_info.node_attr_info_model import NodeAttrInfoModel
from msparser.compact_info.node_attr_info_bean import NodeAttrInfoBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class NodeAttrInfoParser(DataParser, MsMultiProcess):
    """
    node attr info data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=bad-super-call
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._node_attr_info_data = []

    @staticmethod
    def _get_node_attr_info_data(bean_data: NodeAttrInfoBean) -> list:
        """
        transform bean to data
        """
        return [
            bean_data.level,
            bean_data.struct_type,
            bean_data.thread_id,
            bean_data.timestamp,
            bean_data.node_id,
            bean_data.hash_id,
        ]

    def save(self: any) -> None:
        """
        save node attr info data
        """
        if not self._node_attr_info_data:
            return
        with NodeAttrInfoModel(self._project_path) as model:
            model.flush(self._transform_node_attr_info_data(self._node_attr_info_data))

    def parse(self: any) -> None:
        """
        parse node attr info data
        """
        node_attr_info_files = self._file_list.get(DataTag.NODE_ATTR_INFO, [])
        node_attr_info_files = self.group_aging_file(node_attr_info_files)
        if not node_attr_info_files:
            return

        for file_list in node_attr_info_files.values():
            self._node_attr_info_data.extend(
                self.parse_bean_data(
                    file_list,
                    StructFmt.NODE_ATTR_INFO_SIZE,
                    NodeAttrInfoBean,
                    format_func=self._get_node_attr_info_data,
                    check_func=self.check_magic_num,
                )
            )

    def ms_run(self: any) -> None:
        """
        parse and save node attr info data
        :return:
        """
        if not self._file_list.get(DataTag.NODE_ATTR_INFO, []):
            return
        logging.info(
            "start parsing node attr info data, files: %s", str(self._file_list.get(DataTag.NODE_ATTR_INFO, []))
        )
        self.parse()
        self.save()

    def _transform_node_attr_info_data(self: any, data_list: list) -> list:
        hash_dict_data = HashDictData(self._project_path)
        type_hash_dict = hash_dict_data.get_type_hash_dict()
        ge_hash_dict = hash_dict_data.get_ge_hash_dict()
        for data in data_list:
            # data[1]: struct_type, data[4]: op_name, data[5]: hashid
            data[1] = type_hash_dict.get('node', {}).get(data[1], data[1])
            data[4] = ge_hash_dict.get(str(data[4]), str(data[4]))
            data[5] = str(data[5])
        return data_list
