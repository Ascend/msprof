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

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from framework.offset_calculator import OffsetCalculator
from msmodel.npu_mem.npu_mem_model import NpuMemModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from msparser.npu_mem.npu_mem_bean import NpuMemDataBean
from msparser.npu_mem.npu_mem_bean import NpuMemDataBeanV2
from msparser.versioned_struct_parser import VersionedStructParser
from msparser.versioned_struct_parser import VersionedStructSpec
from profiling_bean.prof_enum.data_tag import DataTag


def decode_npu_mem_v1(record: tuple) -> any:
    return NpuMemDataBean().npu_mem_decode(record)


def decode_npu_mem_v2(record: tuple) -> any:
    return NpuMemDataBeanV2().npu_mem_decode(record)


NPU_MEM_STRUCT_SPECS = {
    "1.0": VersionedStructSpec(StructFmt.NPU_MEM_FMT, StructFmt.NPU_MEM_DATA_SIZE, decode_npu_mem_v1),
    "2.0": VersionedStructSpec(StructFmt.NPU_MEM_FMT, StructFmt.NPU_MEM_DATA_SIZE, decode_npu_mem_v2),
}


class NpuMemParser(DataParser, MsMultiProcess):
    """
    parsing npu mem data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        DataParser.__init__(self, sample_config)
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list

        self._npu_mem_data = []
        self._struct_parser = VersionedStructParser("NPU_MEM", NPU_MEM_STRUCT_SPECS)
        self._npu_mem_model = NpuMemModel(self._project_path, DBNameConstant.DB_NPU_MEM, [DBNameConstant.TABLE_NPU_MEM])

    def parse(self: any) -> None:
        """
        start parsing the data
        """
        num_mem_files = self._file_list.get(DataTag.NPU_MEM, [])
        if not num_mem_files:
            return
        for _file in num_mem_files:
            _file_path = PathManager.get_data_file_path(self._project_path, _file)

            _file_size = os.path.getsize(_file_path)
            if not _file_size:
                logging.warning("The size of file: %s is zero. Check whether the file size is correct.", _file)
                continue
            logging.info("start parsing npu mem data file: %s", _file)
            self._process_npu_mem_data(_file_path, _file_size, num_mem_files)
            FileManager.add_complete_file(self._project_path, _file)

    def save(self: any) -> bool:
        """
        save npu mem data
        """
        if self._npu_mem_data:
            self._npu_mem_model.init()
            self._npu_mem_model.flush(self._npu_mem_data)
            self._npu_mem_model.finalize()

    def ms_run(self: any) -> None:
        """
        run function
        """
        if not self._file_list.get(DataTag.NPU_MEM, []):
            return

        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.save()

    def _process_npu_mem_data(self: any, file_path: str, file_size: int, file_list: list) -> None:
        offset_calculator = OffsetCalculator(file_list, self._struct_parser.spec.size, self._project_path)
        with FileOpen(file_path, "rb") as _npu_mem_file:
            _all_npu_mem_data = offset_calculator.pre_process(_npu_mem_file.file_reader, file_size)
            for npu_mem_data_bean in self._struct_parser.iter_decoded_records(_all_npu_mem_data):
                if npu_mem_data_bean:
                    timestamp_ns = InfoConfReader().get_absolute_time_by_sampling_timestamp(npu_mem_data_bean.timestamp)
                    self._npu_mem_data.append(
                        [
                            npu_mem_data_bean.event,
                            npu_mem_data_bean.ddr,
                            npu_mem_data_bean.hbm,
                            timestamp_ns,
                            npu_mem_data_bean.ddr + npu_mem_data_bean.hbm,
                        ]
                    )
