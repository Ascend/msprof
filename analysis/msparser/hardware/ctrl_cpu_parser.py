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
from common_func.db_name_constant import DBNameConstant
from common_func.file_name_manager import get_ctrl_cpu_compiles
from msparser.hardware.cpu_parser import ParsingCPUData
from profiling_bean.prof_enum.data_tag import DataTag


class ParsingCtrlCPUData(ParsingCPUData):
    """
    parsing ctrl cpu data file
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self.curs = None
        self.conn = None
        self.type = 'ctrl'
        self.dbname = DBNameConstant.DB_NAME_CTRLCPU
        self.patterns = get_ctrl_cpu_compiles()
        self._file_list = file_list.get(DataTag.CTRLCPU, [])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    @property
    def cpu_type(self: any) -> str:
        """
        cpu type
        """
        return self.type

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return ParsingCtrlCPUData.__name__
