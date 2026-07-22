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
from typing import List, Union

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.add_info.mc2_comm_info_model import Mc2CommInfoViewModel, Mc2CommInfoModel
from msmodel.compact_info.capture_stream_info_model import CaptureStreamInfoModel
from msparser.compact_info.capture_stream_info_bean import CaptureStreamInfoBean
from msparser.compact_info.capture_stream_info_bean import CaptureStreamInfoV2Bean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class CaptureStreamInfoParser(DataParser, MsMultiProcess):
    """
    capture stream data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._capture_stream_data = []
        self._capture_mc2_comm_info_data = []

    @staticmethod
    def _get_capture_stream_info_data(bean_data: CaptureStreamInfoBean) -> CaptureStreamInfoBean:
        return bean_data

    @staticmethod
    def _get_capture_stream_info_v2_data(bean_data: CaptureStreamInfoV2Bean) -> CaptureStreamInfoV2Bean:
        return bean_data

    def parse(self: any) -> None:
        """
        parse captrue stream data
        """
        stream_info_files = self._file_list.get(DataTag.CAPTURE_STREAM_INFO, [])
        if not stream_info_files:
            return
        # V1优先：优先使用v1格式数据，v1不存在则使用v2
        v1_files = [f for f in stream_info_files if "_v2." not in f]
        v2_files = [f for f in stream_info_files if "_v2." in f]

        if v1_files:
            capture_stream_data = self.parse_bean_data(
                v1_files,
                StructFmt.CAPTURE_STREAM_INFO_SIZE,
                CaptureStreamInfoBean,
                format_func=self._get_capture_stream_info_data,
            )
        elif v2_files:
            capture_stream_data = self.parse_bean_data(
                v2_files,
                StructFmt.CAPTURE_STREAM_INFO_V2_SIZE,
                CaptureStreamInfoV2Bean,
                format_func=self._get_capture_stream_info_v2_data,
            )
        else:
            return
        self._format_stream_data(capture_stream_data)

    def format_data(self: any) -> None:
        self._format_mc2_comm_info_data()

    def save(self: any) -> None:
        """
        save data
        """
        if self._capture_stream_data:
            with CaptureStreamInfoModel(self._project_path) as _model:
                _model.flush(self._capture_stream_data)

        if self._capture_mc2_comm_info_data:
            with Mc2CommInfoModel(self._project_path) as _model:
                _model.flush(self._capture_mc2_comm_info_data)

    def ms_run(self: any) -> None:
        """
        parse and save capture stream info
        :return:
        """
        if not self._file_list.get(DataTag.CAPTURE_STREAM_INFO, []):
            return
        try:
            self.parse()
            self.format_data()
            self.save()
        except Exception as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _format_mc2_comm_info_data(self):
        comm_info_view_model = Mc2CommInfoViewModel(self._project_path, [DBNameConstant.TABLE_MC2_COMM_INFO])
        if not comm_info_view_model.check_db() or not comm_info_view_model.check_table():
            return

        # 当前先不按device_id分组 后续mc2数据修改后再一起修改
        stream_info_dict = dict()
        for stream_info in self._capture_stream_data:
            # 0 device_id, 1 model_id, 2 original_stream_id, 3 model_stream_id
            stream_info_dict.setdefault(stream_info[2], set()).add(stream_info[3])

        comm_info_data_list = comm_info_view_model.get_kfc_stream(DBNameConstant.TABLE_MC2_COMM_INFO)
        for comm_info_data in comm_info_data_list:
            if comm_info_data.aicpu_kfc_stream_id not in stream_info_dict:
                continue
            # 把capture场景的内容手动补充数据上去 避免后续多处修改
            for models_stream_id in stream_info_dict[comm_info_data.aicpu_kfc_stream_id]:
                self._capture_mc2_comm_info_data.append(
                    [
                        comm_info_data.group_name,
                        comm_info_data.rank_size,
                        comm_info_data.rank_id,
                        comm_info_data.usr_rank_id,
                        models_stream_id,
                        comm_info_data.comm_stream_ids,
                    ]
                )

    def _format_stream_data(self, capture_stream_data: List[Union[CaptureStreamInfoBean, CaptureStreamInfoV2Bean]]):
        sorted_capture_stream_data = sorted(capture_stream_data, key=lambda x: x.timestamp)
        start_model_set, end_model_set, stream_info_set = set(), set(), set()
        repeated_num = 0
        batch_id_map = {}
        for bean_data in sorted_capture_stream_data:
            device_stream_key = (bean_data.device_id, bean_data.model_stream_id)
            # 获取当前batch_id
            if device_stream_key not in batch_id_map:
                batch_id = 0
                batch_id_map[device_stream_key] = batch_id
            else:
                # 已经出现过，递增batch_id
                batch_id = batch_id_map[device_stream_key]
            key_without_batch = (
                bean_data.device_id,
                bean_data.model_id,
                bean_data.original_stream_id,
                bean_data.model_stream_id,
                bean_data.capture_status,
                bean_data.timestamp,
            )
            if key_without_batch in stream_info_set:
                repeated_num += 1
                continue
            # 0 start; 1 end: 记录无需落盘,start 和end对不上属于正常情况（prof stop时 capture流还未被销毁）
            if bean_data.capture_status == 0 and bean_data.model_id not in start_model_set:
                start_model_set.add(bean_data.model_id)
            if bean_data.capture_status == 1:
                if bean_data.model_id not in end_model_set:
                    end_model_set.add(bean_data.model_id)
                else:
                    continue
            batch_id_map[device_stream_key] = batch_id + 1
            stream_info_set.add(key_without_batch)
            self._capture_stream_data.append(
                (
                    bean_data.device_id,
                    bean_data.model_id,
                    bean_data.original_stream_id,
                    bean_data.model_stream_id,
                    batch_id,
                    bean_data.capture_status,
                    bean_data.timestamp,
                )
            )

        if start_model_set != end_model_set:
            logging.warning("start model ids are %s, end model ids are %s.", start_model_set, end_model_set)
        if repeated_num:
            logging.warning("There are %s duplicate records in total.", repeated_num)
