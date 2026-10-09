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
from collections import OrderedDict

from common_func.common import check_number_valid
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from common_func.utils import Utils
from msmodel.hardware.mini_llc_model import MiniLlcModel
from profiling_bean.prof_enum.data_tag import DataTag


class MiniLLCParser(MsMultiProcess):
    """
    parsing LLC data class
    """

    SPLIT_FMT = "/"

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list.get(DataTag.LLC, [])
        self.sample_config = sample_config
        self.project_path = self.sample_config.get("result_dir")
        self._model = MiniLlcModel(
            self.project_path,
            DBNameConstant.DB_LLC,
            [
                DBNameConstant.TABLE_MINI_LLC_METRICS,
                DBNameConstant.TABLE_LLC_DSID,
                DBNameConstant.TABLE_LLC_ORIGIN,
                DBNameConstant.TABLE_LLC_BANDWIDTH,
                DBNameConstant.TABLE_LLC_CAPACITY,
            ],
        )
        self.metric_tmp = OrderedDict()
        self.dsid_tmp = OrderedDict()
        self.llc_data = {'metric': [], 'dsid': [], 'original_data': []}
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    @staticmethod
    def handle_llc_data(tmp: any, llc_data: list, container: list, device_id: str, replay_id: str) -> None:
        """
        handle llc tmp data
        """
        if llc_data[0] == tmp["timestamp"] and tmp["timestamp"] != 0:
            tmp[llc_data[-1]] = llc_data[1]
        elif tmp["timestamp"] == 0:
            tmp["timestamp"] = llc_data[0]
            tmp["device_id"] = device_id
            tmp["replayid"] = replay_id
            tmp[llc_data[-1]] = llc_data[1]
        elif llc_data[0] != tmp["timestamp"]:
            container.append(list(tmp.values()))
            for k in list(tmp.keys()):
                tmp[k] = 0
            tmp["timestamp"] = llc_data[0]
            tmp["device_id"] = device_id
            tmp["replayid"] = replay_id
            tmp[llc_data[-1]] = llc_data[1]

    def init_tmp_data(self: any) -> None:
        """
        init tmp data
        """
        # init metric tmp OrderedDict
        self.metric_tmp["device_id"] = 0
        self.metric_tmp["replayid"] = 0
        self.metric_tmp["timestamp"] = 0
        self.metric_tmp["read_allocate"] = 0
        self.metric_tmp["read_noallocate"] = 0
        self.metric_tmp["read_hit"] = 0
        self.metric_tmp["write_allocate"] = 0
        self.metric_tmp["write_noallocate"] = 0
        self.metric_tmp["write_hit"] = 0
        # init dsid tmp OrderedDict
        self.dsid_tmp["device_id"] = 0
        self.dsid_tmp["replayid"] = 0
        self.dsid_tmp["timestamp"] = 0
        self.dsid_tmp["dsid0"] = 0
        self.dsid_tmp["dsid1"] = 0
        self.dsid_tmp["dsid2"] = 0
        self.dsid_tmp["dsid3"] = 0
        self.dsid_tmp["dsid4"] = 0
        self.dsid_tmp["dsid5"] = 0
        self.dsid_tmp["dsid6"] = 0
        self.dsid_tmp["dsid7"] = 0

    def format_metric_data(self) -> OrderedDict:
        metric_tmp = OrderedDict()
        metric_tmp["device_id"] = self.metric_tmp["device_id"]
        metric_tmp["replayid"] = self.metric_tmp["replayid"]
        metric_tmp["timestamp"] = self.metric_tmp["timestamp"]
        metric_tmp["read_allocate"] = self.metric_tmp["read_allocate"]
        metric_tmp["read_noallocate"] = self.metric_tmp["read_noallocate"]
        metric_tmp["read_hit"] = self.metric_tmp["read_hit"]
        metric_tmp["write_allocate"] = self.metric_tmp["write_allocate"]
        metric_tmp["write_noallocate"] = self.metric_tmp["write_noallocate"]
        metric_tmp["write_hit"] = self.metric_tmp["write_hit"]
        return metric_tmp

    def read_binary_data(self: any, file_name: str, device_id: str, replay_id: str) -> None:
        """
        parsing llc data and insert into llc.db
        """
        self.init_tmp_data()
        headers = [device_id, replay_id]
        file_path = PathManager.get_data_file_path(self.project_path, file_name)
        start_time = InfoConfReader().get_start_timestamp() / NumberConstant.NANO_SECOND
        try:
            with FileOpen(file_path, 'r') as llc_file:
                self._read_binary_helper(llc_file.file_reader, start_time, headers)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", file_name, err, exc_info=Constant.TRACE_BACK_SWITCH)
            return

        metric_tmp = self.format_metric_data()
        self.llc_data.setdefault('metric', []).append(list(metric_tmp.values()))
        self.llc_data.setdefault('dsid', []).append(list(self.dsid_tmp.values()))

    def start_parsing_data_file(self: any) -> None:
        """
        parsing data file
        """
        try:
            for file_name in self._file_list:
                if is_valid_original_data(file_name, self.project_path):
                    device_id = self.sample_config.get("device_id", "0")
                    logging.info("start parsing llc data file: %s", file_name)
                    self.read_binary_data(file_name, device_id, '0')  # replay id is 0
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        logging.info("Create LLC DB finished!")

    def save(self: any) -> None:
        """
        save mini llc data to db
        :return: None
        """
        if self.llc_data and self._model:
            self._model.init()
            self._model.create_table()
            self._model.flush(self.llc_data)
            self._model.calculate(self.sample_config.get("llc_profiling"))
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        llc parse entry
        """
        try:
            if self._file_list:
                self.start_parsing_data_file()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as mini_llc_err:
            logging.error(str(mini_llc_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _update_line(self: any, line: list, start_time: int, headers: list) -> None:
        line[0] = start_time + float(line[0])
        self.llc_data.setdefault('original_data', []).append(headers + line)
        if line[-1].startswith("dsid"):
            self.handle_llc_data(self.dsid_tmp, line, self.llc_data.get('dsid', []), headers[0], headers[1])
        else:
            self.handle_llc_data(self.metric_tmp, line, self.llc_data.get('metric', []), headers[0], headers[1])

    def _read_binary_helper(self: any, llc_file: any, start_time: int, headers: list) -> None:
        while 1:
            line = llc_file.readline(Constant.MAX_READ_FILE_BYTES)
            if line:
                if line.startswith("#") or not line.strip():
                    continue
                line = line.strip().replace("<not counted>", "0").split(" ")
                line = Utils.generator_to_list(i for i in line if i != '' and not i.startswith("("))
                if not line:
                    continue
                line = line[:-1] + line[-1].split(self.SPLIT_FMT)[:-1]
                # time, counts, events_sp0, events_sp1
                if len(line) != 4:
                    continue
                if not check_number_valid(line[0]):  # must be float number
                    continue
                self._update_line(line, start_time, headers)
            else:
                break
