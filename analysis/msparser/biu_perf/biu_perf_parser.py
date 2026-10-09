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

from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.biu_perf.biu_perf_model import BiuPerfModel
from msparser.biu_perf.biu_core_parser import BiuCubeParser
from msparser.biu_perf.biu_core_parser import BiuVectorParser
from msparser.interface.iparser import IParser
from profiling_bean.biu_perf.core_info_bean import CoreInfo
from profiling_bean.prof_enum.data_tag import DataTag


class BiuPerfParser(IParser, MsMultiProcess):
    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = self._sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = BiuPerfModel(
            self._project_path, [DBNameConstant.TABLE_FLOW_MONITOR, DBNameConstant.TABLE_CYCLES_MONITOR]
        )
        self._flow_data_list = []
        self._cycles_data_list = []

    def ms_run(self: any) -> None:
        """
        parse biu perf data and save it to db.
        :return:None
        """
        if self._file_list.get(DataTag.BIU_PERF, []):
            self.parse()
            self.save()

    def parse(self: any) -> None:
        """
        to read biu perf data
        :return: None
        """
        biu_perf_file_dict = self._file_group_by_core()
        for core_info in biu_perf_file_dict.values():
            if core_info.core_type == CoreInfo.AI_CUBE:
                biucycles_data, flow_data = BiuCubeParser(self._sample_config, core_info).get_monitor_data()
                self._cycles_data_list.extend(biucycles_data)
                self._flow_data_list.extend(flow_data)
            elif core_info.core_type in [CoreInfo.AI_VECTOR0, CoreInfo.AI_VECTOR1]:
                biucycles_data = BiuVectorParser(self._sample_config, core_info).get_monitor_data()
                self._cycles_data_list.extend(biucycles_data)
            else:
                logging.error("Core type %d is unknown.", core_info.core_type)

    def save(self: any) -> None:
        """
        save parser data to db
        :return: None
        """
        if not self._flow_data_list or not self._cycles_data_list:
            logging.warning("Monitor data list is empty!")
            return

        with self._model as _model:
            self._model.create_table()
            _model.flush(DBNameConstant.TABLE_FLOW_MONITOR, self._flow_data_list)
            _model.flush(DBNameConstant.TABLE_CYCLES_MONITOR, self._cycles_data_list)

    def _file_group_by_core(self: any) -> dict:
        """
        create dict whose key is core id and value is file list
        :return: None
        """
        biu_perf_all_files = self._file_list.get(DataTag.BIU_PERF, [])
        biu_perf_file_dict = {}
        for _file in biu_perf_all_files:
            core_name = _file.split(".")[1]
            core_info = biu_perf_file_dict.setdefault(core_name, CoreInfo(core_name))
            core_info.file_list.append(_file)
        return biu_perf_file_dict
