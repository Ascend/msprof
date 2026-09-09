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

import os
from collections import defaultdict

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.file_manager import FileManager
from common_func.path_manager import PathManager

from framework.offset_calculator import OffsetCalculator


def mark_ccu_files_complete(project_path, file_names, marker_writer=FileManager.add_complete_file):
    for file_name in file_names:
        marker_writer(project_path, file_name)
        marker_path = PathManager.get_data_file_path(project_path, file_name) + Constant.COMPLETE_TAG
        if not os.path.isfile(marker_path):
            return False
    return True


def ccu_table_exists(project_path, db_name, table_name):
    db_path = PathManager.get_db_path(project_path, db_name)
    return DBManager.check_tables_in_db(db_path, table_name)


class CcuOffsetCalculator(OffsetCalculator):
    """Consume an aging prefix across slices without changing the shared calculator."""

    def __init__(self, *args):
        super().__init__(*args)
        self.file_list = [name for name in self.file_list if not name.endswith((".complete", ".done", ".zip"))]
        self.last_cache = bytes()
        self._remaining_offset = 0

    @property
    def has_completed_files(self):
        return any(
            os.path.exists(PathManager.get_data_file_path(self.project_path, name) + Constant.COMPLETE_TAG)
            for name in self.file_list
        )

    def pre_process(self, file_reader, file_size):
        if not 0 < file_size <= Constant.MAX_READ_FILE_BYTES:
            raise ValueError("Invalid CCU source size: {}".format(file_size))
        if not self.has_read:
            completed = [
                os.path.exists(PathManager.get_data_file_path(self.project_path, name) + ".complete")
                for name in self.file_list
            ]
            if any(completed) and not all(completed):
                raise ValueError("Partially completed CCU source; clear parsed output and reimport")
            self._remaining_offset = self.calculate_total_offset()
        offset = min(self._remaining_offset, file_size)
        if len(file_reader.read(offset)) != offset:
            raise ValueError("CCU source changed while reading its prefix")
        self._remaining_offset -= offset
        data = file_reader.read(file_size - offset)
        if len(data) != file_size - offset:
            raise ValueError("CCU source changed while reading its records")
        complete_data = self.last_cache + data
        complete_size = len(complete_data) // self.struct_size * self.struct_size
        self.last_cache = complete_data[complete_size:]
        return complete_data[:complete_size]


class CcuSourceCalculator:
    """Keep truncated-record state isolated for each CCU hardware source."""

    def __init__(self, file_list, profile, data_kind, record_size, project_path):
        self._profile = profile
        self._data_kind = data_kind
        self._record_size = record_size
        self._project_path = project_path
        files_by_source = defaultdict(list)
        for file_name in file_list:
            if file_name.endswith((".complete", ".done", ".zip")):
                continue
            try:
                files_by_source[self._get_source_key(file_name)].append(file_name)
            except ValueError:
                # The parser reports unsupported sources when it processes each file.
                continue
        self._calculators = {
            source_key: CcuOffsetCalculator(source_files, record_size, project_path)
            for source_key, source_files in files_by_source.items()
        }

    @property
    def has_completed_files(self):
        return any(calculator.has_completed_files for calculator in self._calculators.values())

    def pre_process(self, file_name, file_reader, file_size):
        source_key = self._get_source_key(file_name)
        calculator = self._calculators.get(source_key)
        if calculator is None:
            calculator = CcuOffsetCalculator([file_name], self._record_size, self._project_path)
            self._calculators[source_key] = calculator
        return calculator.pre_process(file_reader, file_size)

    def validate(self):
        if any(calculator.last_cache for calculator in self._calculators.values()):
            raise ValueError("Incomplete CCU source after parsing; source files remain incomplete")

    def _get_source_key(self, file_name):
        return self._profile.get_source_scope(self._data_kind, file_name)
