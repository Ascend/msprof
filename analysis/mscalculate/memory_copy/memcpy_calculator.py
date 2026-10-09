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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.memcpy_constant import MemoryCopyConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_iteration import MsprofIteration
from common_func.profiling_scene import ProfilingScene
from mscalculate.interface.icalculator import ICalculator
from msmodel.memory_copy.memcpy_model import MemcpyModel
from profiling_bean.struct_info.memcpy_state_machine import MemcpyRecorder


class MemcpyCalculator(ICalculator, MsMultiProcess):
    """
    calculate memcpy data
    """

    def __init__(self: any, _: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_range = sample_config.get(StrConstant.PARAM_ITER_ID)

        self._model = MemcpyModel(
            self._project_path, DBNameConstant.DB_MEMORY_COPY, [DBNameConstant.TABLE_TS_MEMCPY_CALCULATION]
        )

        self._memcpy_data = []
        self._conn = None
        self._curs = None
        self._has_table = False

    @staticmethod
    def _state_groupby_stream_task(ts_data: tuple) -> dict:
        stream_task_group = {}
        for ts_datum in ts_data:
            memcpy_recorder = stream_task_group.setdefault(
                (ts_datum[MemoryCopyConstant.STREAM_INDEX], ts_datum[MemoryCopyConstant.TASK_INDEX]),
                MemcpyRecorder(ts_datum[MemoryCopyConstant.STREAM_INDEX], ts_datum[MemoryCopyConstant.TASK_INDEX]),
            )

            memcpy_recorder.process_state_tag(
                ts_datum[MemoryCopyConstant.TASK_STATE_INDEX], ts_datum[MemoryCopyConstant.TIMESTAMP_INDEX]
            )

        return stream_task_group

    def calculator_connect_db(self: any) -> None:
        """
        judge ts memcpy table exist
        :return: void
        """
        self._conn, self._curs = DBManager.check_connect_db(self._project_path, DBNameConstant.DB_STEP_TRACE)

        self._has_table = all(
            [self._conn, self._curs, DBManager.judge_table_exist(self._curs, DBNameConstant.TABLE_TS_MEMCPY)]
        )

    def calculate(self: any) -> None:
        """
        get ts memcpy data and reshape it
        :return: void
        """
        if self._has_table:
            if ProfilingScene().is_all_export():
                sql = "select stream_id, task_id, timestamp, task_state from {0} order by timestamp".format(
                    DBNameConstant.TABLE_TS_MEMCPY
                )
                self._curs.execute(sql)
            else:
                time_range = MsprofIteration(self._project_path).get_step_iteration_time(self._iter_range)
                if not time_range:
                    return

                iter_start, iter_end = (
                    (time_range[0][0], time_range[1][0]) if len(time_range) == 2 else (0, time_range[0][0])
                )
                sql = (
                    "select stream_id, task_id, timestamp, task_state from {0} "
                    "where timestamp>=? and timestamp<=? order by timestamp".format(DBNameConstant.TABLE_TS_MEMCPY)
                )
                self._curs.execute(sql, (iter_start, iter_end))
            ts_data = self._curs.fetchall()
            stream_task_group = self._state_groupby_stream_task(ts_data)
            self._reshape_memcpy_data(stream_task_group)
        DBManager.destroy_db_connect(self._conn, self._curs)

    def save(self: any) -> None:
        """
        save memcpy data
        """
        if self._memcpy_data:
            self._model.init()
            self._model.create_table()
            self._model.flush(DBNameConstant.TABLE_TS_MEMCPY_CALCULATION, self._memcpy_data)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        calculate for task scheduler
        :return:
        """
        self.calculator_connect_db()
        self.calculate()
        self.save()

    def _reshape_memcpy_data(self: any, stream_task_group: dict) -> None:
        for stream_task, memcpy_recorder in stream_task_group.items():
            for states_timestamp in memcpy_recorder.each_batch_timestamp:
                memcpy_datum = list(stream_task)
                if MemoryCopyConstant.DEFAULT_TIMESTAMP in states_timestamp:
                    continue

                memcpy_datum.extend(states_timestamp)
                memcpy_datum.append(
                    memcpy_datum[MemoryCopyConstant.END_INDEX] - memcpy_datum[MemoryCopyConstant.START_INDEX]
                )
                memcpy_datum.append(MemoryCopyConstant.ASYNC_MEMCPY_NAME)
                memcpy_datum.append(MemoryCopyConstant.TYPE)

                self._memcpy_data.append(tuple(memcpy_datum))
