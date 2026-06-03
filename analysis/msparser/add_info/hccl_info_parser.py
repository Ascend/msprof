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
from common_func.file_manager import FileOpen
from common_func.hash_dict_constant import HashDictData
from common_func.hccl_info_common import trans_enum_name, RoleType, OpType, DataType, LinkType, TransPortType, RdmaType
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from framework.offset_calculator import OffsetCalculator
from msmodel.add_info.hccl_info_model import HcclInfoModel
from msparser.add_info.hccl_info_bean import HcclInfoBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class HcclInfoParser(DataParser, MsMultiProcess):
    """
    parsing hccl information data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._hccl_info_data = []

    def parse(self: any) -> None:
        """
        parse function
        """
        hccl_info_files = self._file_list.get(DataTag.HCCL_INFO, [])
        hccl_info_files = self.group_aging_file(hccl_info_files)
        for file_list in hccl_info_files.values():
            offset_calculator = OffsetCalculator(file_list, StructFmt.HCCL_INFO_FMT_SIZE, self._project_path)
            for _file in file_list:
                if not is_valid_original_data(_file, self._project_path):
                    continue
                _file_path = self.get_file_path_and_check(_file)
                logging.info("start parsing hccl information thread data file: %s", _file)
                self._read_hccl_info(_file_path, offset_calculator)

    def save(self: any) -> None:
        """
        save hccl information parser data to db
        :return: None
        """
        if not self._hccl_info_data:
            return
        model = HcclInfoModel(self._project_path)
        with model as _model:
            _model.flush(self.reformat_data())

    def ms_run(self: any) -> None:
        """
        parse hccl information data and save it to db.
        :return:
        """
        if not self._file_list.get(DataTag.HCCL_INFO, []):
            return
        try:
            self.parse()
        except (OSError, IOError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.save()

    def reformat_data(self) -> list:
        type_info_data = HashDictData(self._project_path).get_type_hash_dict().get("communication", {})
        hash_data = HashDictData(self._project_path).get_ge_hash_dict()
        reformat = []
        for data in self._hccl_info_data:
            role = trans_enum_name(RoleType, data.role)
            op_type = trans_enum_name(OpType, data.op_type)
            data_type = trans_enum_name(DataType, data.data_type)
            link_type = trans_enum_name(LinkType, data.link_type)
            transport_type = trans_enum_name(TransPortType, data.transport_type)
            rdma_type = trans_enum_name(RdmaType, data.rdma_type)
            reformat.append(
                [data.level, type_info_data.get(data.struct_type, data.struct_type), data.thread_id, data.data_len,
                 data.timestamp, hash_data.get(data.item_id, data.item_id), data.ccl_tag,
                 data.group_name, data.local_rank, data.remote_rank, data.rank_size,
                 data.work_flow_mode, data.plane_id, data.context_id, data.notify_id,
                 data.stage, role, str(data.duration_estimated), data.src_addr, data.dst_addr,
                 data.size, op_type, data_type, link_type, transport_type,
                 rdma_type])
        return reformat

    def _read_hccl_info(self: any, file_path: str, offset: OffsetCalculator) -> None:
        file_size = os.path.getsize(file_path)
        if not file_size:
            return
        struct_size = StructFmt.HCCL_INFO_FMT_SIZE
        with FileOpen(file_path, 'rb') as _hccl_info_file:
            _all_hccl_info_data = offset.pre_process(_hccl_info_file.file_reader, file_size)
            for _index in range(file_size // struct_size):
                data = _all_hccl_info_data[_index * struct_size:(_index + 1) * struct_size]
                self.check_magic_num(data)
                self._hccl_info_data.append(HcclInfoBean.decode(data))
