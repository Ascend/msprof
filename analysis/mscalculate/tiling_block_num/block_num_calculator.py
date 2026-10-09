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
import os

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from mscalculate.flip.flip_calculator import FlipCalculator
from mscalculate.interface.icalculator import ICalculator
from msmodel.ge.ge_info_model import GeInfoViewModel
from msmodel.step_trace.ts_track_model import TsTrackViewModel


class BlockNumCalculator(ICalculator, MsMultiProcess):
    BITS_FOR_BLOCK_NUM = 16
    INVALID_BLOCK_NUM_VALUE = 65535

    def __init__(self: any, file_list: dict, sample_config: dict):
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._ge_model = GeInfoViewModel(self._project_path, [DBNameConstant.TABLE_GE_TASK])
        self._ts_model = TsTrackViewModel(self._project_path, [DBNameConstant.TABLE_BLOCK_NUM])
        self._data = []

    @staticmethod
    def _process_block_num_data(data):
        if ChipManager().is_chip_v6():
            return {datum.task_id: datum for datum in data}
        return {(datum.stream_id, datum.task_id, datum.batch_id): datum for datum in data}

    def calculate(self: any) -> None:
        with self._ge_model as _ge_model:
            if not _ge_model.check_table():
                return
            ge_task_data = _ge_model.get_ge_info_by_device_id(
                DBNameConstant.TABLE_GE_TASK, InfoConfReader().get_device_id()
            )
        if not ge_task_data:
            return

        with self._ts_model as _ts_model:
            if not _ts_model.check_table():
                return
            tiling_block_num_data = _ts_model.get_tiling_block_num_data()
        if not tiling_block_num_data:
            return

        tiling_block_num_data = FlipCalculator.set_device_batch_id(tiling_block_num_data, self._project_path)
        processed_block_num_data = self._process_block_num_data(tiling_block_num_data)
        for ge_data in ge_task_data:
            if ChipManager().is_chip_v6():
                search_key = ge_data.task_id
            else:
                search_key = (ge_data.stream_id, ge_data.task_id, ge_data.batch_id)
            if search_key in processed_block_num_data:
                tiling_block_num = processed_block_num_data.get(search_key).block_num
                self._data.append(
                    ge_data.replace(
                        block_num=tiling_block_num & self.INVALID_BLOCK_NUM_VALUE,
                        mix_block_num=(tiling_block_num & self.INVALID_BLOCK_NUM_VALUE)
                        * (tiling_block_num >> self.BITS_FOR_BLOCK_NUM),
                    )
                )
            else:
                self._data.append(ge_data)

    def save(self: any) -> None:
        if not self._data:
            return
        with self._ge_model as _ge_model:
            delete_sql = (
                f"delete from {DBNameConstant.TABLE_GE_TASK} where device_id={InfoConfReader().get_device_id()}"
            )
            DBManager.execute_sql(_ge_model.conn, delete_sql)
            _ge_model.insert_data_to_db(DBNameConstant.TABLE_GE_TASK, self._data)

    def ms_run(self: any) -> None:
        if not os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_GE_INFO)):
            return
        self.calculate()
        self.save()
