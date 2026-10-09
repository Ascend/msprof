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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from common_func.utils import Utils
from common_func.file_manager import FileOpen
from framework.offset_calculator import FileCalculator
from framework.offset_calculator import OffsetCalculator
from mscalculate.hwts.task_dispatch_model_index import TaskDispatchModelIndex
from mscalculate.interface.icalculator import ICalculator
from mscalculate.ts_task.ai_cpu.aicpu_from_ts_collector import AICpuFromTsCollector
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from msmodel.task_time.hwts_log_model import HwtsLogModel
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.aicore_task import TaskExecuteBean
from profiling_bean.struct_info.hwts_log import HwtsLogBean


class HwtsCalculator(ICalculator, MsMultiProcess):
    """
    class used to calculate hwts offset and parse log by iter
    """

    # Tags for different HWTS log type.
    HWTS_TASK_START = 0
    HWTS_TASK_END = 1
    HWTS_TASK_TYPE = 2
    HWTS_LOG_SIZE = 64
    HWTS_MAX_CNT = 16
    TASK_TIME_COMPLETE_INDEX = 3

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._file_list = file_list.get(DataTag.HWTS, [])
        self._hwts_log_model = HwtsLogModel(self._project_path)
        self._aicpu_collector = AICpuFromTsCollector(self._project_path)
        self._iter_model = HwtsIterModel(self._project_path)
        self._log_data = []
        self._iter_range = self._sample_config.get(StrConstant.PARAM_ITER_ID)
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def is_need_parse_all_file(self):
        return ProfilingScene().is_all_export() or not os.path.exists(
            PathManager.get_db_path(self._project_path, DBNameConstant.DB_HWTS_REC)
        )

    def calculate(self: any) -> None:
        """
        calculate hwts data
        :return: None
        """
        if self.is_need_parse_all_file():
            db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_HWTS)
            if DBManager.check_tables_in_db(
                db_path, DBNameConstant.TABLE_HWTS_TASK, DBNameConstant.TABLE_HWTS_TASK_TIME
            ):
                logging.info(
                    "The Table %s or %s already exists in the %s, and won't be calculate again.",
                    DBNameConstant.TABLE_HWTS_TASK,
                    DBNameConstant.TABLE_HWTS_TASK_TIME,
                    DBNameConstant.DB_HWTS,
                )
                return
            self._parse_all_file()
        else:
            self._parse_by_iter()

    def save(self: any) -> None:
        """
        save hwts data
        :return: None
        """
        if self._log_data:
            self._hwts_log_model.init()
            self._hwts_log_model.flush(Utils.obj_list_to_list(self._log_data), DBNameConstant.TABLE_HWTS_TASK)
            self._hwts_log_model.flush(self._reform_data(self._prep_data()), DBNameConstant.TABLE_HWTS_TASK_TIME)
            self._hwts_log_model.finalize()

    def ms_run(self: any) -> None:
        """
        entrance for calculating hwts
        :return: None
        """
        if self._file_list:
            self.calculate()
            self.save()

    def _prep_data(self: any) -> list:
        """
        prepare data for tasktime table
        :return:
        """
        prep = []
        start_log_dict = {}
        for task in self._log_data:
            stream_task_id = f'{str(task.stream_id)}-{str(task.task_id)}'
            if task.sys_tag == self.HWTS_TASK_END:
                start_log = start_log_dict.get(stream_task_id)
                start_time = start_log.sys_cnt if start_log else NumberConstant.INVALID_OP_EXE_TIME
                prep.append(TaskExecuteBean(task.stream_id, task.task_id, start_time, task.sys_cnt, task.task_type))
                start_log_dict.pop(stream_task_id, None)
            else:
                start_log_dict[stream_task_id] = task
        train_data = []
        for task in prep:
            data_info = [task.stream_id, task.task_id, task.start_time, task.end_time, task.task_type]
            train_data.append(data_info)
            if not ChipManager().is_chip_v2():
                self._aicpu_collector.filter_aicpu(data_info)
        if not ChipManager().is_chip_v2():
            self._aicpu_collector.save_aicpu()
        return sorted(train_data, key=lambda data: data[self.TASK_TIME_COMPLETE_INDEX])

    def _parse_by_iter(self: any) -> None:
        """
        Parse the specified iteration data
        :return: None
        """
        # if parse by iter, the table should be cleared before
        self._hwts_log_model.clear()
        if self._iter_model.check_db() and self._iter_model.check_table():
            task_offset, task_count = self._iter_model.get_task_offset_and_sum(
                self._iter_range, HwtsIterModel.TASK_TYPE
            )
            if not task_count:
                return
            _file_calculator = FileCalculator(
                self._file_list, self.HWTS_LOG_SIZE, self._project_path, task_offset, task_count
            )
            self._parse(_file_calculator.prepare_process())
            self._iter_model.finalize()

    def _parse_all_file(self: any) -> None:
        _offset_calculator = OffsetCalculator(self._file_list, self.HWTS_LOG_SIZE, self._project_path)
        for _file in self._file_list:
            _file = PathManager.get_data_file_path(self._project_path, _file)
            with FileOpen(_file, 'rb') as _hwts_log_reader:
                self._parse(_offset_calculator.pre_process(_hwts_log_reader.file_reader, os.path.getsize(_file)))

    def _reform_data(self: any, prep_data_res: list) -> list:
        if self.is_need_parse_all_file() or ProfilingScene().is_step_export():
            for index, datum in enumerate(prep_data_res):
                # index 0 stream id, index 1 task id
                prep_data_res[index] = list(datum[:2]) + [
                    InfoConfReader().time_from_syscnt(datum[2]),
                    InfoConfReader().time_from_syscnt(datum[3]),
                    datum[-1],
                    self._iter_range.iteration_id,
                    self._iter_range.model_id,
                ]
            return prep_data_res
        task_dispatcher = TaskDispatchModelIndex(self._iter_range, self._project_path)
        result_data = []
        for index, datum in enumerate(prep_data_res):
            # type of batch is tuple
            # 3 is end time
            if prep_data_res[index][2] == NumberConstant.INVALID_OP_EXE_TIME:
                continue

            model_id, index_id = task_dispatcher.dispatch(datum[3])
            result_data.append(
                list(datum[:2])
                + [
                    InfoConfReader().time_from_syscnt(datum[2]),
                    InfoConfReader().time_from_syscnt(datum[3]),
                    datum[-1],
                    index_id,
                    model_id,
                ]
            )
        return result_data

    def _parse(self: any, all_log_bytes: bytes) -> None:
        for log_data in Utils.chunks(all_log_bytes, self.HWTS_LOG_SIZE):
            _task_log = HwtsLogBean.decode(log_data)
            if _task_log.is_log_type():
                self._log_data.append(_task_log)
