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
from msparser.aicpu.data_preparation_parser import DataPreparationParser


class DataPreparationParserSubclass(DataPreparationParser):
    @staticmethod
    def read_data_queue(file_path: str) -> list:
        return DataPreparationParser._read_data_queue(file_path)

    def check_host_queue_mode(self: any, file_list: list) -> None:
        super()._check_host_queue_mode(file_list)

    def parse_host_queue(self: any, file_list: list) -> list:
        return super()._parse_host_queue(file_list)

    def parse_data_queue(self: any, file_list: list) -> list:
        return super()._parse_data_queue(file_list)

    def parse_data_queue_file(self: any, file_path: str) -> list:
        return super()._parse_data_queue_file(file_path)

    def get_data(self: any) -> dict:
        return self._data

    def get_host_queue_mode(self: any) -> int:
        return self._host_queue_mode
