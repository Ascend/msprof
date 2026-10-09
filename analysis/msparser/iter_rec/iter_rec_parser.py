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

import logging
import os
import sqlite3
from collections import OrderedDict

from common_func.batch_counter import BatchCounter
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.iter_recorder import IterRecorder
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from common_func.utils import Utils
from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from framework.offset_calculator import OffsetCalculator
from msmodel.ge.ge_info_calculate_model import GeInfoModel
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from msmodel.step_trace.ts_track_model import TsTrackModel
from msparser.interface.iparser import IParser
from msparser.iter_rec.iter_info_updater.iter_info import IterInfo
from msparser.iter_rec.iter_info_updater.iter_info_updater import IterInfoUpdater
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.aic_pmu import AicPmuBean
from profiling_bean.struct_info.hwts_log import HwtsLogBean
from mscalculate.flip.flip_calculator import FlipCalculator


class IterParser(IParser, MsMultiProcess):
    HWTS_LOG_SIZE = 64
    AICORE_LOG_SIZE = 128
    STREAM_TASK_KEY_FMT = "{0}-{1}"
    STREAM_TASK_BATCH_KEY_FMT = "{0}-{1}-{2}"
    STATIC_ITER_ID = 0
    DEFAULT_ITER_ID = -1
    HWTS_TASK_END = 1
    AI_CORE_SIZE = 128
    DEFAULT_TASK_TIME_SIZE = 5000000

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._batch_counter = BatchCounter(self._project_path)
        self._iter_recorder = IterRecorder(self._project_path)
        self._iter_op_set = MsprofIteration(self._project_path).get_step_trace_op()
        self._iter_info_updater = IterInfoUpdater(self._project_path)
        self._hwts_task_time_data = [None] * self.DEFAULT_TASK_TIME_SIZE
        self.ge_info_model = GeInfoModel(PathManager.get_host_result_dir(self._project_path))
        self._task_start_dict = {}
        self.default_index = 0
        self.hwts_iter_model = HwtsIterModel(self._project_path)
        self.ai_core_task = set()
        self._task_cnt_not_in_iter = dict()
        # len(self._iter_recorder.iter_end_dict) + 1 means pmu cnt after last iteration
        self._pmu_cnt_not_in_iter = OrderedDict()
        self._task_flip = dict()

    def save(self: any) -> None:
        """
        multiprocess to parse hwts data
        :return: None
        """
        self._iter_info_updater.update_iter_without_hwts()
        iter_to_iter_info = self._iter_info_updater.iteration_manager.iter_to_iter_info
        try:
            if iter_to_iter_info:
                hwts_iter_data = [
                    [
                        iter_info.iter_id,
                        iter_info.model_id,
                        iter_info.index_id,
                        iter_info.hwts_count,
                        iter_info.hwts_offset,
                        iter_info.aic_count,
                        iter_info.aic_offset,
                    ]
                    for iter_info in iter_to_iter_info.values()
                ]
                self.hwts_iter_model.flush(hwts_iter_data, DBNameConstant.TABLE_HWTS_ITER_SYS)
                self.hwts_iter_model.finalize()
        except sqlite3.Error as trace_err:
            logging.error("Save hwts iter failed, %s", str(trace_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def parse(self: any) -> None:
        """
        parse hwts data by ge info and iter sys cnt
        :return: None
        """

    def ms_run(self):
        """

        :return:
        """
        pass

    def is_need_to_calculate(self):
        return not (
            self.hwts_iter_model.check_iter_data_in_db(DBNameConstant.TABLE_HWTS_ITER_SYS)
            or self.hwts_iter_model.check_iter_data_in_db(DBNameConstant.TABLE_HWTS_BATCH)
        )

    def _read_hwts_data(self: any, all_bytes: bytes) -> None:
        for _chunk in Utils.chunks(all_bytes, self.HWTS_LOG_SIZE):
            _task_log = HwtsLogBean.decode(_chunk)
            if not _task_log.is_log_type():
                continue
            stream_task_id = self.STREAM_TASK_KEY_FMT.format(_task_log.stream_id, _task_log.task_id)
            if stream_task_id not in self._iter_op_set:
                self._iter_recorder.set_current_iter_id(_task_log.sys_cnt)
            curr_iter = self._iter_recorder.current_iter_id
            iter_info = self._iter_info_updater.iteration_manager.iter_to_iter_info.get(curr_iter, IterInfo())
            # Because of cache in runtime to be reported, the hwts may contain tasks which are not in iteration.
            # we should filter out these tasks.
            if not self._iter_recorder.check_task_in_iter(_task_log.sys_cnt, list(iter_info.behind_parallel_iter)):
                self._add_cnt_not_in_iter(curr_iter, _task_log)
                continue
            if _task_log.sys_tag == self.HWTS_TASK_END:
                self._calculate_batch_list(_task_log)
                self._create_hwts_task_time_data(_task_log, stream_task_id)
            else:
                self._task_start_dict[stream_task_id] = _task_log
            self._iter_info_updater.update_parallel_iter_info_pool(self._iter_recorder.current_iter_id)
            self._iter_info_updater.update_count_and_offset(_task_log)

    def _add_cnt_not_in_iter(self: any, curr_iter: int, task_log: HwtsLogBean):
        self._task_cnt_not_in_iter.setdefault(curr_iter, 0)
        self._task_cnt_not_in_iter[curr_iter] += 1
        if task_log.sys_tag == self.HWTS_TASK_END:
            self._calculate_batch_list(task_log)
            if self._iter_info_updater.judge_ai_core(task_log, self.ai_core_task):
                self._pmu_cnt_not_in_iter.setdefault(curr_iter, 0)
                self._pmu_cnt_not_in_iter[curr_iter] += 1

    def _calculate_batch_list(self: any, task_log: HwtsLogBean) -> None:
        if ChipManager().is_chip_all_data_export() and InfoConfReader().is_all_export_version():
            setattr(task_log, "batch_id", 0)
            setattr(task_log, "timestamp", InfoConfReader().time_from_syscnt(task_log.sys_cnt))
            [task_log] = FlipCalculator.compute_batch_id([task_log], self._task_flip)
            return
        # not all export
        setattr(
            task_log,
            "batch_id",
            self._batch_counter.calculate_batch(
                task_log.stream_id, task_log.task_id, self._iter_recorder.current_iter_id
            ),
        )

    def _create_hwts_task_time_data(self: any, task_log: HwtsLogBean, stream_task_id: str) -> None:
        start_task_log = self._task_start_dict.get(stream_task_id)
        setattr(
            task_log, "start_time", start_task_log.sys_cnt if start_task_log else NumberConstant.INVALID_OP_EXE_TIME
        )
        self._task_start_dict.pop(stream_task_id, None)

        setattr(task_log, "is_ai_core", self._iter_info_updater.judge_ai_core(task_log, self.ai_core_task))
        if self.default_index == self.DEFAULT_TASK_TIME_SIZE:
            self.hwts_iter_model.flush(self._hwts_task_time_data, DBNameConstant.TABLE_HWTS_BATCH)
            self._hwts_task_time_data = [None] * self.DEFAULT_TASK_TIME_SIZE
            self.default_index = 0
        self._hwts_task_time_data[self.default_index] = (
            task_log.stream_id,
            task_log.task_id,
            task_log.batch_id,
            self._iter_recorder.current_iter_id,
            task_log.start_time,
            task_log.sys_cnt,
            task_log.is_ai_core,
        )
        self.default_index = self.default_index + 1

    def _parse_hwts_data(self: any) -> None:
        if ChipManager().is_chip_all_data_export() and InfoConfReader().is_all_export_version():
            with TsTrackModel(
                self._project_path, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_DEVICE_TASK_FLIP]
            ) as model:
                self._task_flip = model.get_task_flip_data()
        hwts_files = self._file_list.get(DataTag.HWTS, [])
        hwts_files.sort(key=lambda x: int(x.split("_")[-1]))
        _offset_calculator = OffsetCalculator(hwts_files, self.HWTS_LOG_SIZE, self._project_path)
        self.hwts_iter_model.init()
        self.hwts_iter_model.clear_table()
        for _hwts_file in hwts_files:
            _hwts_file = PathManager.get_data_file_path(self._project_path, _hwts_file)
            logging.info("Begin to process hwts data file: %s", os.path.basename(_hwts_file))
            with FileOpen(_hwts_file, 'rb') as _hwts_file_reader:
                all_bytes = _offset_calculator.pre_process(_hwts_file_reader.file_reader, os.path.getsize(_hwts_file))
                self._read_hwts_data(all_bytes)
        for iter_num, task_offset in self._task_cnt_not_in_iter.items():
            self._iter_info_updater.calibrate_iter_info_offset(task_offset=task_offset, iter_offset=iter_num)
        # len(self._iter_recorder.iter_end_dict) + 1 means pmu cnt after last iteration
        self._pmu_cnt_not_in_iter.setdefault(len(self._iter_recorder.iter_end_dict) + 1, 0)
        self._iter_info_updater.calibrate_aic_offset(self._pmu_cnt_not_in_iter, self._get_aic_count_in_file())
        if self.default_index > 0:
            del self._hwts_task_time_data[self.default_index :]
            self.hwts_iter_model.flush(self._hwts_task_time_data, DBNameConstant.TABLE_HWTS_BATCH)

    def _get_aic_count_in_file(self: any) -> int:
        sum_file_size = 0
        for file in self._file_list.get(DataTag.AI_CORE, []):
            sum_file_size += os.path.getsize(PathManager.get_data_file_path(self._project_path, file))
        return sum_file_size // self.AICORE_LOG_SIZE


class IterRecParser(IterParser):
    """
    this class used to parse hwts log data.
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(file_list, sample_config)
        self._sample_config = sample_config
        self._file_list = file_list

    def parse(self: any) -> None:
        """
        parse hwts data by ge info and iter sys cnt
        :return: None
        """
        # The condition can not be remove, or hwts data would be flush two times without GE.
        if not DBManager.check_tables_in_db(
            PathManager.get_db_path(self._project_path, DBNameConstant.DB_GE_INFO), DBNameConstant.TABLE_GE_TASK
        ):
            return

        self._batch_counter.init()
        self._iter_info_updater.iteration_manager.initial_iter_to_info()
        self._parse_hwts_data()

    def ms_run(self: any) -> None:
        """
        multiprocess to parse hwts data
        :return: None
        """
        try:
            if (
                self._file_list.get(DataTag.HWTS, [])
                and not ProfilingScene().is_operator()
                and self.is_need_to_calculate()
            ):
                self.parse()
                self.save()
        except ProfException as rec_error:
            logging.warning("Iter rec parse failed, error code : %s", rec_error.code)
        finally:
            pass


class NoGeIterRecParser(IterParser):
    """
    this class used to parse hwts log data.
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(file_list, sample_config)
        self._file_list = file_list

    def judge_file_scene(self: any, file_dict: dict) -> bool:
        return bool(
            file_dict.get(DataTag.HWTS)
            and file_dict.get(DataTag.AI_CORE)
            and not DBManager.check_tables_in_db(
                PathManager.get_db_path(self._project_path, DBNameConstant.DB_GE_INFO), DBNameConstant.TABLE_GE_TASK
            )
            and not self.hwts_iter_model.check_iter_data_in_db(DBNameConstant.TABLE_HWTS_ITER_SYS)
            and self.hwts_iter_model.check_iter_data_in_db(DBNameConstant.TABLE_HWTS_BATCH)
        )

    def parse(self: any) -> None:
        """
        parse hwts data by ge info and iter sys cnt
        :return: None
        """
        self._parse_ai_core_data()
        self._iter_info_updater.iteration_manager.initial_iter_to_info()
        self._parse_hwts_data()

    def ms_run(self: any) -> None:
        """
        multiprocess to parse hwts data
        :return: None
        """
        try:
            if (
                self.judge_file_scene(self._file_list)
                and not ProfilingScene().is_operator()
                and self.is_need_to_calculate()
            ):
                self.parse()
                self.save()
        except ProfException as rec_error:
            logging.warning("Iter rec parse failed, error code : %s", rec_error.code)

    def _read_ai_core_data(self: any, all_bytes: bytes) -> None:
        for _chunk in Utils.chunks(all_bytes, self.AI_CORE_SIZE):
            _task_log = AicPmuBean.decode(_chunk)
            if _task_log:
                self.ai_core_task.add('{}-{}'.format(_task_log.stream_id, _task_log.task_id))

    def _parse_ai_core_data(self: any):
        ai_core_files = self._file_list.get(DataTag.AI_CORE, [])
        ai_core_files.sort(key=lambda x: int(x.split("_")[-1]))
        _offset_calculator = OffsetCalculator(ai_core_files, self.AI_CORE_SIZE, self._project_path)
        for ai_core_file in ai_core_files:
            ai_core_file_path = PathManager.get_data_file_path(self._project_path, ai_core_file)
            logging.info("Begin to process ai_core data file: %s with out ge data", os.path.basename(ai_core_file))
            with FileOpen(ai_core_file_path, 'rb') as _ai_core_file_reader:
                all_bytes = _offset_calculator.pre_process(
                    _ai_core_file_reader.file_reader, os.path.getsize(ai_core_file_path)
                )
                self._read_ai_core_data(all_bytes)
