# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.hash_dict_constant import HashDictData
from common_func.hccl_info_common import trans_enum_name, RoleType, OpType, DataType, LinkType, TransPortType, RdmaType
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.dpu.dpu_task_model import DPUTaskModel
from msparser.add_info.dpu_hccl_track_bean import DPUHcclTrackBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class DPUHcclInfoParser(DataParser, MsMultiProcess):
    """
    parsing DPU hccl information data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=E1003
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._hccl_info_data = []

    def parse(self: any) -> None:
        """
        parse function
        """
        hccl_info_files = self._file_list.get(DataTag.DPU_HCCL_TRACK, [])
        hccl_info_files = self.group_aging_file(hccl_info_files)
        if not hccl_info_files:
            return
        for files in hccl_info_files.values():
            self._hccl_info_data.extend(
                self.parse_bean_data(
                    files,
                    StructFmt.DPU_HCCL_TRACK_SIZE,
                    DPUHcclTrackBean,
                    format_func=lambda x: x,
                    check_func=self.check_magic_num,
                )
            )

    def save(self: any) -> None:
        """
        save hccl information parser data to db
        :return: None
        """
        if not self._hccl_info_data:
            return
        model = DPUTaskModel(self._project_path)
        with model as _model:
            _model.flush(self.reformat_data(), DBNameConstant.TABLE_DPU_HCCL_TRACK)

    def ms_run(self: any) -> None:
        """
        parse DPU hccl information data and save it to db.
        :return:
        """
        if not self._file_list.get(DataTag.DPU_HCCL_TRACK, []):
            return
        try:
            self.parse()
        except (OSError, IOError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.save()

    def reformat_data(self) -> list:
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
                [
                    data.npu_device_id,
                    data.dpu_device_id,
                    data.thread_id,
                    data.start_time,
                    data.timestamp,
                    hash_data.get(data.item_id, data.item_id),
                    hash_data.get(data.group_name, Constant.NA),
                    data.group_name,
                    data.local_rank,
                    data.remote_rank,
                    data.rank_size,
                    data.duration_estimated,
                    data.src_addr,
                    data.dst_addr,
                    data.data_size,
                    data.stream_id,
                    data.task_id,
                    data.aicpu_task_id,
                    data.plane_id,
                    op_type,
                    data_type,
                    link_type,
                    transport_type,
                    rdma_type,
                    role,
                    data.ccl_tag,
                    data.notify_id,
                    data.work_flow_mode,
                    data.stage,
                ]
            )
        return reformat
