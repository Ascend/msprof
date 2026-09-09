# pylint: disable=duplicate-code
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
from collections import defaultdict

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from msmodel.add_info.ccu_add_info_model import CCUTaskInfoModel
from msmodel.add_info.ccu_add_info_model import CCUWaitSignalInfoModel
from msmodel.add_info.ccu_add_info_model import CCUGroupInfoModel
from msparser.add_info.ccu_add_info_bean import CCUTaskInfoBean
from msparser.add_info.ccu_add_info_bean import CCUWaitSignalInfoBean
from msparser.add_info.ccu_add_info_bean import CCUGroupInfoBean
from msparser.data_struct_size_constant import StructFmt
from msparser.ccu_source_calculator import CcuOffsetCalculator
from msparser.ccu_source_calculator import ccu_table_exists
from msparser.ccu_source_calculator import mark_ccu_files_complete
from profiling_bean.prof_enum.data_tag import DataTag

CCU_TABLE_BY_TAG = {
    DataTag.CCU_TASK: DBNameConstant.TABLE_CCU_TASK_INFO,
    DataTag.CCU_WAIT_SIGNAL: DBNameConstant.TABLE_CCU_WAIT_SIGNAL_INFO,
    DataTag.CCU_GROUP: DBNameConstant.TABLE_CCU_GROUP_INFO,
}


class CCUAddInfoParser(MsMultiProcess):
    """
    class used to parser CCU add info binary data
    this class is for CCU additional data located in host side
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._file_list = {
            DataTag.CCU_TASK: file_list.get(DataTag.CCU_TASK, []),
            DataTag.CCU_WAIT_SIGNAL: file_list.get(DataTag.CCU_WAIT_SIGNAL, []),
            DataTag.CCU_GROUP: file_list.get(DataTag.CCU_GROUP, []),
        }
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH, "")
        self._calculate_dict = {
            DataTag.CCU_TASK: CcuOffsetCalculator(
                self._file_list.get(DataTag.CCU_TASK, []), StructFmt.CCU_TASK_INFO_FMT_SIZE, self._project_path
            ),
            DataTag.CCU_WAIT_SIGNAL: CcuOffsetCalculator(
                self._file_list.get(DataTag.CCU_WAIT_SIGNAL, []),
                StructFmt.CCU_WAIT_SIGNAL_INFO_FMT_SIZE,
                self._project_path,
            ),
            DataTag.CCU_GROUP: CcuOffsetCalculator(
                self._file_list.get(DataTag.CCU_GROUP, []), StructFmt.CCU_GROUP_INFO_FMT_SIZE, self._project_path
            ),
        }
        self._model_dict = {
            DataTag.CCU_TASK: CCUTaskInfoModel(self._project_path),
            DataTag.CCU_WAIT_SIGNAL: CCUWaitSignalInfoModel(self._project_path),
            DataTag.CCU_GROUP: CCUGroupInfoModel(self._project_path),
        }
        self.read_binary_func_dict = {
            DataTag.CCU_TASK: self.read_task_binary_data,
            DataTag.CCU_WAIT_SIGNAL: self.read_wait_binary_data,
            DataTag.CCU_GROUP: self.read_group_binary_data,
        }
        self._ccu_add_info_data = defaultdict(list)
        self._pending_complete_files = []

        self._processed_tags = set()

    @staticmethod
    def get_single_task_info_data(single_bean) -> any:
        """
        get formated single ccu task info data
        :param single_bean: any
        :return list: ccu task info data
        """
        return [
            single_bean.version,
            single_bean.work_flow_mode,
            single_bean.item_id,
            single_bean.group_name,
            single_bean.rank_id,
            single_bean.rank_size,
            single_bean.stream_id,
            single_bean.task_id,
            single_bean.die_id,
            single_bean.mission_id,
            single_bean.instr_id,
        ]

    @staticmethod
    def get_single_wait_signal_info_data(single_bean) -> any:
        """
        get formated single ccu wait signal data
        :param single_bean: any
        :return list: ccu wait signal data
        """
        wait_signal_info = []
        flag = False
        for i in range(16):
            if (
                single_bean.channel_id[i] != Constant.UINT16_MAX
                and single_bean.remote_rank_id[i] != Constant.UINT32_MAX
            ):
                wait_signal_info.append(
                    [
                        single_bean.version,
                        single_bean.item_id,
                        single_bean.group_name,
                        single_bean.rank_id,
                        single_bean.rank_size,
                        single_bean.work_flow_mode,
                        single_bean.stream_id,
                        single_bean.task_id,
                        single_bean.die_id,
                        single_bean.instr_id,
                        single_bean.mission_id,
                        single_bean.cke_id,
                        single_bean.mask,
                        single_bean.channel_id[i],
                        single_bean.remote_rank_id[i],
                    ]
                )
                flag = True
        if not flag:
            logging.warning(
                "The CCU wait signal info datum has no channel id and remote rank id, so will be filtered."
                "stream id is %s, task id is %s, instr id is %s",
                single_bean.stream_id,
                single_bean.task_id,
                single_bean.instr_id,
            )
        return wait_signal_info

    @staticmethod
    def get_single_group_info_data(single_bean) -> any:
        """
        get formated single ccu wait signal data
        :param single_bean: any
        :return list: ccu wait signal data
        """
        group_signal_info = []
        flag = False
        for i in range(16):
            if (
                single_bean.channel_id[i] != Constant.UINT16_MAX
                and single_bean.remote_rank_id[i] != Constant.UINT32_MAX
            ):
                group_signal_info.append(
                    [
                        single_bean.version,
                        single_bean.item_id,
                        single_bean.group_name,
                        single_bean.rank_id,
                        single_bean.rank_size,
                        single_bean.work_flow_mode,
                        single_bean.stream_id,
                        single_bean.task_id,
                        single_bean.die_id,
                        single_bean.instr_id,
                        single_bean.mission_id,
                        single_bean.reduce_op_type,
                        single_bean.input_data_type,
                        single_bean.output_data_type,
                        single_bean.data_size,
                        single_bean.channel_id[i],
                        single_bean.remote_rank_id[i],
                    ]
                )
                flag = True
        if not flag:
            logging.warning(
                "The CCU group info datum has no channel id and remote rank id, so will be filtered."
                "stream id is %s, task id is %s, instr id is %s",
                single_bean.stream_id,
                single_bean.task_id,
                single_bean.instr_id,
            )
        return group_signal_info

    def read_task_binary_data(self: any, file_name: str) -> any:
        """
        read CCU Task Info binary data
        :param file_name: CCU Task Info data
        :return bool flag: success signal
        """
        files = PathManager.get_data_file_path(self._project_path, file_name)
        if not os.path.exists(files):
            return NumberConstant.ERROR
        _file_size = os.path.getsize(files)
        if _file_size <= 0:
            logging.error("Ccu task info data struct is incomplete, please check the file.")
            return NumberConstant.ERROR
        with FileOpen(files, "rb") as f:
            task_info_bin = self._calculate_dict[DataTag.CCU_TASK].pre_process(f.file_reader, _file_size)
            struct_nums = len(task_info_bin) // StructFmt.CCU_TASK_INFO_FMT_SIZE
            for i in range(struct_nums):
                single_bean = CCUTaskInfoBean().decode(
                    task_info_bin[i * StructFmt.CCU_TASK_INFO_FMT_SIZE : (i + 1) * StructFmt.CCU_TASK_INFO_FMT_SIZE]
                )
                if single_bean:
                    self._ccu_add_info_data.setdefault(DataTag.CCU_TASK, []).append(
                        self.get_single_task_info_data(single_bean)
                    )
        return NumberConstant.SUCCESS

    def read_wait_binary_data(self: any, file_name: str) -> any:
        """
        read CCU wait signal info binary data
        :param file_name: CCU wait signal info data
        :return bool flag: success signal
        """
        files = PathManager.get_data_file_path(self._project_path, file_name)
        if not os.path.exists(files):
            return NumberConstant.ERROR
        _file_size = os.path.getsize(files)
        if _file_size <= 0:
            logging.error("Ccu wait signal data struct is incomplete, please check the file.")
            return NumberConstant.ERROR
        with FileOpen(files, "rb") as f:
            wait_info_bin = self._calculate_dict[DataTag.CCU_WAIT_SIGNAL].pre_process(f.file_reader, _file_size)
            struct_nums = len(wait_info_bin) // StructFmt.CCU_WAIT_SIGNAL_INFO_FMT_SIZE
            for i in range(struct_nums):
                single_bean = CCUWaitSignalInfoBean().decode(
                    wait_info_bin[
                        i * StructFmt.CCU_WAIT_SIGNAL_INFO_FMT_SIZE : (i + 1) * StructFmt.CCU_WAIT_SIGNAL_INFO_FMT_SIZE
                    ]
                )
                if single_bean:
                    self._ccu_add_info_data.setdefault(DataTag.CCU_WAIT_SIGNAL, []).extend(
                        self.get_single_wait_signal_info_data(single_bean)
                    )
        return NumberConstant.SUCCESS

    def read_group_binary_data(self: any, file_name: str) -> any:
        """
        read CCU group info binary data
        :param file_name: CCU group info data
        :return bool flag: success signal
        """
        files = PathManager.get_data_file_path(self._project_path, file_name)
        if not os.path.exists(files):
            return NumberConstant.ERROR
        _file_size = os.path.getsize(files)
        if _file_size <= 0:
            logging.error("Ccu group info data struct is incomplete, please check the file.")
            return NumberConstant.ERROR
        with FileOpen(files, "rb") as f:
            group_info_bean = self._calculate_dict[DataTag.CCU_GROUP].pre_process(f.file_reader, _file_size)
            struct_nums = len(group_info_bean) // StructFmt.CCU_GROUP_INFO_FMT_SIZE
            for i in range(struct_nums):
                single_bean = CCUGroupInfoBean().decode(
                    group_info_bean[i * StructFmt.CCU_GROUP_INFO_FMT_SIZE : (i + 1) * StructFmt.CCU_GROUP_INFO_FMT_SIZE]
                )
                if single_bean:
                    self._ccu_add_info_data.setdefault(DataTag.CCU_GROUP, []).extend(
                        self.get_single_group_info_data(single_bean)
                    )
        return NumberConstant.SUCCESS

    def parse(self: any) -> None:
        """
        parsing data file
        """
        logging.info("Start parsing ccu add info data file.")
        for data_tag, file_list in self._file_list.items():
            for file_name in file_list:
                if is_valid_original_data(file_name, self._project_path):
                    self._handle_original_data(data_tag, file_name)
        if any(calculator.last_cache for calculator in self._calculate_dict.values()):
            raise ValueError("Incomplete CCU add-info source; source files remain incomplete")
        logging.info("Create ccu add info table finished!")

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        for key, calculator in self._calculate_dict.items():
            if calculator.has_completed_files and not ccu_table_exists(
                self._project_path, DBNameConstant.DB_CCU_ADD_INFO, CCU_TABLE_BY_TAG[key]
            ):
                raise RuntimeError("Completed CCU add-info source has no persisted table; clear markers and reimport")
        group_data = self._ccu_add_info_data.get(DataTag.CCU_GROUP, [])
        if any(row[14] > NumberConstant.INT64_MAX for row in group_data):
            raise RuntimeError("CCU add-info data_size exceeds the SQLite integer range")
        for key in self._calculate_dict:
            data = self._ccu_add_info_data.get(key, [])
            if key in self._processed_tags or data:
                with self._model_dict.get(key) as model:
                    if not model.flush(data):
                        raise RuntimeError("Save CCU add-info failed; clear parsed output and reimport")
        if not mark_ccu_files_complete(self._project_path, self._pending_complete_files, FileManager.add_complete_file):
            raise RuntimeError("Create CCU add-info complete marker failed")
        self._pending_complete_files.clear()

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        try:
            if self._file_list:
                self.parse()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _handle_original_data(self: any, data_tag, file_name: str) -> None:
        if self.read_binary_func_dict.get(data_tag)(file_name) == NumberConstant.ERROR:
            raise ValueError("Parse CCU add-info data failed: {}".format(file_name))
        self._pending_complete_files.append(file_name)
        self._processed_tags.add(data_tag)
