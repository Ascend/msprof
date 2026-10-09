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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.path_manager import PathManager
from common_func.utils import Utils
from mscalculate.hwts.hwts_calculator import HwtsCalculator
from msmodel.task_time.hwts_aiv_model import HwtsAivModel
from profiling_bean.prof_enum.data_tag import DataTag


class HwtsAivCalculator(HwtsCalculator):
    """
    class used to calculate hwts offset and parse log by iter
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(file_list, sample_config)
        self._file_list = file_list.get(DataTag.HWTS_AIV, [])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        self._hwts_aiv_model = HwtsAivModel(
            self._project_path, [DBNameConstant.TABLE_HWTS_TASK, DBNameConstant.TABLE_HWTS_TASK_TIME]
        )

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return "HwtsAivCalculator"

    def ms_run(self: any) -> None:
        """
        entrance for aiv calculator
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
            self.save()

    def save(self: any) -> None:
        """
        save hwts data
        :return: None
        """
        self._hwts_aiv_model.clear()
        if self._log_data:
            self._hwts_aiv_model.init()
            self._hwts_aiv_model.flush_data(Utils.obj_list_to_list(self._log_data), DBNameConstant.TABLE_HWTS_TASK)
            self._hwts_aiv_model.flush_data(self._reform_data(self._prep_data()), DBNameConstant.TABLE_HWTS_TASK_TIME)
            self._hwts_aiv_model.finalize()

    def _reform_data(self: any, prep_data_res: list) -> list:
        for index, datum in enumerate(prep_data_res):
            prep_data_res[index] = list(datum[:2]) + [
                InfoConfReader().time_from_syscnt(datum[2]),
                InfoConfReader().time_from_syscnt(datum[3]),
                datum[-1],
                self._iter_range.iteration_id,
                self._iter_range.model_id,
            ]
        return prep_data_res
