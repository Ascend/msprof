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

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from msmodel.l2_cache.l2_cache_parser_model import L2CacheParserModel
from msmodel.l2_cache.soc_pmu_model import SocPmuModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.soc_pmu import SocPmuBean
from profiling_bean.struct_info.soc_pmu import SocPmuChip6Bean


class SocPmuParser(IParser, MsMultiProcess):
    """
    soc pmu data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._file_list = file_list
        self._model = SocPmuModel(self._project_path, [DBNameConstant.TABLE_SOC_PMU])
        self._soc_pmu_data = []
        self._ha_data = []
        self._page_fault = []

    @staticmethod
    def _check_file_complete(file_path: str) -> int:
        _file_size = os.path.getsize(file_path)
        if _file_size and _file_size % StructFmt.SOC_PMU_FMT_SIZE != 0:
            logging.error("soc pmu data is not complete, and the file name is %s", os.path.basename(file_path))
        return _file_size

    def save(self: any) -> None:
        """
        save the result of data parsing
        :return: NA
        """
        if self._soc_pmu_data:
            with SocPmuModel(self._project_path, [DBNameConstant.TABLE_SOC_PMU]) as _model:
                _model.flush(self._soc_pmu_data)
        if self._ha_data:
            with L2CacheParserModel(self._project_path, [DBNameConstant.TABLE_L2CACHE_PARSE]) as _model:
                _model.flush(self._ha_data)
        if self._page_fault:
            with SocPmuModel(self._project_path, [DBNameConstant.TABLE_PAGE_FAULT]) as _model:
                _model.flush(self._page_fault, DBNameConstant.TABLE_PAGE_FAULT)

    def parse(self: any) -> None:
        """
        parse the data under the file path
        :return: NA
        """
        soc_pmu_files = self._file_list.get(DataTag.SOC_PMU)
        is_chip_v6 = ChipManager().is_chip_v6()
        for _file in soc_pmu_files:
            _file_path = PathManager.get_data_file_path(self._project_path, _file)
            _file_size = SocPmuParser._check_file_complete(_file_path)
            if not _file_size:
                logging.error("soc pmu data is empty, and the file name is %s", os.path.basename(_file_path))
                return
            with FileOpen(_file_path, 'rb') as _soc_pmu_file:
                _all_soc_pmu_data = _soc_pmu_file.file_reader.read(_file_size)
                for index in range(_file_size // StructFmt.SOC_PMU_FMT_SIZE):
                    soc_pmu_data_bean = SocPmuChip6Bean() if is_chip_v6 else SocPmuBean()
                    soc_pmu_data_bean.decode(
                        _all_soc_pmu_data[index * StructFmt.SOC_PMU_FMT_SIZE : (index + 1) * StructFmt.SOC_PMU_FMT_SIZE]
                    )
                    self._add_data_by_type(soc_pmu_data_bean)
            FileManager.add_complete_file(self._project_path, _file_path)

    def ms_run(self: any) -> None:
        """
        main entry
        """
        try:
            if self._file_list.get(DataTag.SOC_PMU):
                self.parse()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _add_data_by_type(self, soc_pmu_data_bean):
        data = [
            soc_pmu_data_bean.task_type,
            soc_pmu_data_bean.stream_id,
            soc_pmu_data_bean.task_id,
            ",".join(soc_pmu_data_bean.events_list),
        ]
        if soc_pmu_data_bean.data_type == 1:
            self._ha_data.append(data)
        elif soc_pmu_data_bean.data_type == 2:
            self._soc_pmu_data.append(data)
        elif soc_pmu_data_bean.data_type == 4:
            self._page_fault.append(data)
        else:
            logging.warning("Unsupported data type: %s", soc_pmu_data_bean.data_type)
