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

from common_func.hash_dict_constant import HashDictData
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msparser.add_info.static_op_mem_bean import StaticOpMemBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from msmodel.add_info.static_op_mem_model import StaticOpMemModel
from profiling_bean.prof_enum.data_tag import DataTag


class StaticOpMemParser(DataParser, MsMultiProcess):
    """
    Operator memory data parser for static graph scenarios
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=bad-super-call
        self._file_list = file_list
        self._sample_config = sample_config
        self._static_op_data = []
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    def parse(self: any) -> None:
        """
        parse static op memory data
        """
        static_op_mem_files = self._file_list.get(DataTag.STATIC_OP_MEM, [])
        static_op_mem_files = self.group_aging_file(static_op_mem_files)
        for file_list in static_op_mem_files.values():
            self._static_op_data.extend(
                self.parse_bean_data(
                    file_list, StructFmt.STATIC_OP_MEM_SIZE, StaticOpMemBean, format_func=self._get_static_op_mem_data
                )
            )

    def save(self: any) -> None:
        """
        save data`
        """
        if not self._static_op_data:
            return
        with StaticOpMemModel(self._project_path) as model:
            model.flush(self._transform_static_op_mem_data(self._static_op_data))

    def ms_run(self: any) -> None:
        """
        parse and save static op memory data
        """
        if not self._file_list.get(DataTag.STATIC_OP_MEM, []):
            return
        self.parse()
        self.save()

    def _get_static_op_mem_data(self: any, bean_data: any) -> list:
        """
        Format static op memory data
        """
        if not bean_data:
            return []
        return [
            bean_data.op_name,
            bean_data.dyn_op_name,
            bean_data.graph_id,
            bean_data.life_start,
            bean_data.life_end,
            bean_data.op_mem_size,
        ]

    def _transform_static_op_mem_data(self: any, data_list: list) -> list:
        """
        Transform hash id to op name
        """
        hash_dict_data = HashDictData(self._project_path)
        ge_hash_dict = hash_dict_data.get_ge_hash_dict()
        for data in data_list:
            # op_name,如果是0则替换成TOTAL
            data[0] = ge_hash_dict.get(str(data[0]), str(data[0]))
            if data[0] == "0":
                data[0] = "TOTAL"
            # dyn_op_name
            data[1] = ge_hash_dict.get(str(data[1]), str(data[1]))
        return data_list
