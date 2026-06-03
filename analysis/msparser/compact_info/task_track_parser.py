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
from typing import List

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.hash_dict_constant import HashDictData
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from common_func.ms_constant.ge_enum_constant import GeTaskType
from msmodel.compact_info.task_track_model import TaskTrackModel
from msmodel.dpu.dpu_task_model import DPUTaskModel
from msparser.compact_info.task_track_bean import TaskTrackBean
from msparser.compact_info.task_track_bean import TaskTrackChip6Bean
from msparser.compact_info.task_track_bean import DPUTaskTrackBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag
from mscalculate.flip.flip_calculator import FlipCalculator


class TaskTrackParser(DataParser, MsMultiProcess):
    """
    task track data parser
    """
    TASK_FLIP = "FLIP_TASK"
    TASK_MAINTENANCE = "MAINTENANCE"

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._ge_hash_dict = {}
        self._type_hash_dict = {}
        self._task_track_data = []
        self._task_flip_data = []
        self._dpu_data = []

    def format_task_track_data(self: any, bean_data: List[TaskTrackBean]) -> None:
        """
        transform bean to data
        """
        for data in bean_data:
            level_hash = self._type_hash_dict.get(data.level, {})
            task_type = level_hash.get(data.task_type, data.task_type)

            if task_type == self.TASK_FLIP:
                setattr(data, 'flip_num', data.batch_id)
                self._task_flip_data.append(data)
            elif task_type != self.TASK_MAINTENANCE:
                if data.is_dpu:
                    self._dpu_data.append(data)
                else:
                    self._task_track_data.append(data)

        if ChipManager().is_chip_all_data_export() and InfoConfReader().is_all_export_version():
            self._task_track_data = FlipCalculator.compute_batch_id(self._task_track_data, self._task_flip_data)
        self._task_track_data = [
            [
                bean.device_id,
                bean.timestamp,
                self._type_hash_dict.get(bean.level, {}).get(bean.task_type, bean.task_type),  # task type
                bean.stream_id,
                bean.task_id,
                bean.thread_id,
                bean.batch_id,
                self._type_hash_dict.get(bean.level, {}).get(bean.struct_type, bean.struct_type),  # task track type
                bean.level,
                bean.data_len,
                self._ge_hash_dict.get(bean.kernel_name, Constant.NA)
            ] for bean in self._task_track_data
        ]
        self._task_flip_data = [
            [
                bean.stream_id,
                bean.timestamp,
                bean.task_id,
                bean.batch_id,
            ] for bean in self._task_flip_data
        ]

    def parse_task_track(self: any) -> None:
        track_files = self._file_list.get(DataTag.TASK_TRACK, [])
        track_files = self.group_aging_file(track_files)
        if not track_files:
            return
        bean_data = []
        task_track_bean = TaskTrackChip6Bean if ChipManager().is_chip_v6() else TaskTrackBean
        for files in track_files.values():
            bean_data.extend(
                self.parse_bean_data(
                    files,
                    StructFmt.TASK_TRACK_DATA_SIZE,
                    task_track_bean,
                    format_func=lambda x: x,
                    check_func=self.check_magic_num,
                )
            )
        self.format_task_track_data(bean_data)

    def format_dpu_track_data(self: any, bean_data: List[DPUTaskTrackBean]) -> None:
        """
        transform bean to data
        """
        logging.info(f"self._dpu_data date len is {len(self._dpu_data)}")
        kernel_name_map = {
            (bean.device_id, bean.task_id): self._ge_hash_dict.get(bean.kernel_name, Constant.NA)
            for bean in self._dpu_data
        }

        dpu_task_list = []
        for track in bean_data:
            task_type = self._type_hash_dict.get(track.level, {}).get(track.task_type, track.task_type)
            dpu_task_list.append([
                track.device_id,
                track.thread_id,
                track.start_time,
                track.timestamp,
                GeTaskType.AI_CPU.name if task_type == Constant.KERNEL_AICPU else Constant.TASK_TYPE_UNKNOWN,
                track.stream_id,
                track.task_id,
                kernel_name_map.get((track.device_id, track.task_id), Constant.NA)
            ])

        self._dpu_data = dpu_task_list

    def parse_dpu_track(self: any) -> None:
        track_files = self._file_list.get(DataTag.DPU_TASK_TRACK, [])
        track_files = self.group_aging_file(track_files)
        if not track_files:
            return
        bean_data = []
        for files in track_files.values():
            bean_data.extend(
                self.parse_bean_data(
                    files,
                    StructFmt.DPU_TASK_TRACK_SIZE,
                    DPUTaskTrackBean,
                    format_func=lambda x: x,
                    check_func=self.check_magic_num,
                )
            )
        self.format_dpu_track_data(bean_data)

    def save(self: any) -> None:
        """
        save task track data
        """
        if self._task_track_data:
            with TaskTrackModel(self._project_path, [DBNameConstant.TABLE_TASK_TRACK]) as model:
                model.flush(self._task_track_data)
        if self._task_flip_data:
            with TaskTrackModel(self._project_path, [DBNameConstant.TABLE_HOST_TASK_FLIP]) as model:
                model.flush(self._task_flip_data)
        if self._dpu_data:
            with DPUTaskModel(self._project_path) as model:
                model.flush(self._dpu_data, DBNameConstant.TABLE_DPU_TASK_TRACK)

    def parse(self: any) -> None:
        """
        parse task track data
        """
        hash_dict_data = HashDictData(self._project_path)
        self._ge_hash_dict = hash_dict_data.get_ge_hash_dict()
        self._type_hash_dict = hash_dict_data.get_type_hash_dict()
        self.parse_task_track()
        self.parse_dpu_track()

    def ms_run(self: any) -> None:
        """
        parse and save task track data
        :return:
        """
        has_task_track = bool(self._file_list.get(DataTag.TASK_TRACK, []))
        has_dpu_track = bool(self._file_list.get(DataTag.DPU_TASK_TRACK, []))

        if not has_task_track and not has_dpu_track:
            return

        if has_task_track:
            logging.info("start parsing task track data, files: %s",
                         str(self._file_list.get(DataTag.TASK_TRACK, [])))
        if has_dpu_track:
            logging.info("start parsing DPU task track data, files: %s",
                         str(self._file_list.get(DataTag.DPU_TASK_TRACK, [])))
        self.parse()
        self.save()
