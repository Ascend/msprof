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

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from framework.offset_calculator import OffsetCalculator
from msmodel.npu_mem.npu_ai_stack_mem_model import NpuAiStackMemModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from msparser.npu_mem.npu_op_mem_bean import NpuOpMemDataBean
from profiling_bean.prof_enum.data_tag import DataTag


class NpuOpMemParser(DataParser, MsMultiProcess):
    """
    parsing npu op mem data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=bad-super-call
        self._file_list = file_list

        self._npu_op_mem_data = []
        self._npu_op_mem_model = NpuAiStackMemModel(
            self._project_path, DBNameConstant.DB_MEMORY_OP, [DBNameConstant.TABLE_NPU_OP_MEM_RAW]
        )

    def parse(self: any) -> None:
        """
        start parsing the data
        """
        num_op_mem_files = self._file_list.get(DataTag.MEMORY_OP, [])
        if not num_op_mem_files:
            return
        for _file in num_op_mem_files:
            _file_path = PathManager.get_data_file_path(self._project_path, _file)
            _file_size = os.path.getsize(_file_path)
            if not _file_size:
                logging.warning("The size of file: %s is zero. Check whether the file size is correct.", _file)
                continue
            logging.info("start parsing npu op mem data file: %s", _file)
            self._process_npu_op_mem_data(_file_path, _file_size, num_op_mem_files)
            FileManager.add_complete_file(self._project_path, _file)

    def save(self: any) -> bool:
        """
        save npu op mem data
        """
        if self._npu_op_mem_data:
            with self._npu_op_mem_model as _model:
                _model.flush(DBNameConstant.TABLE_NPU_OP_MEM_RAW, self._npu_op_mem_data)

    def ms_run(self: any) -> None:
        """
        run function
        """
        if not self._file_list.get(DataTag.MEMORY_OP, []):
            return
        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.save()

    def _process_npu_op_mem_data(self: any, file_path: str, file_size: int, file_list: list) -> None:
        offset_calculator = OffsetCalculator(file_list, StructFmt.MEMORY_OP_SIZE, self._project_path)
        with FileOpen(file_path, "rb") as _npu_op_mem_file:
            _all_npu_op_mem_data = offset_calculator.pre_process(_npu_op_mem_file.file_reader, file_size)
            for _index in range(file_size // StructFmt.MEMORY_OP_SIZE):
                npu_op_mem_data_bean = NpuOpMemDataBean().npu_op_mem_decode(
                    _all_npu_op_mem_data[_index * StructFmt.MEMORY_OP_SIZE : (_index + 1) * StructFmt.MEMORY_OP_SIZE]
                )
                if npu_op_mem_data_bean:
                    _device_type = "NPU" + ':' + str(npu_op_mem_data_bean.device_id)
                    self._npu_op_mem_data.append(
                        [
                            str(npu_op_mem_data_bean.node_id),
                            str(npu_op_mem_data_bean.addr),
                            npu_op_mem_data_bean.size,
                            npu_op_mem_data_bean.timestamp,
                            npu_op_mem_data_bean.thread_id,
                            npu_op_mem_data_bean.total_allocate_memory,
                            npu_op_mem_data_bean.total_reserve_memory,
                            npu_op_mem_data_bean.level,
                            npu_op_mem_data_bean.type,
                            _device_type,
                        ]
                    )
