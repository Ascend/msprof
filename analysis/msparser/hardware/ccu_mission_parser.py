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
import sqlite3

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from msmodel.hardware.ccu_mission_model import CCUMissionModel
from msparser.ccu_decoder_registry import CcuDecoderRegistry
from msparser.ccu_source_calculator import CcuSourceCalculator
from msparser.ccu_source_calculator import ccu_table_exists
from msparser.ccu_source_calculator import mark_ccu_files_complete
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.hardware.ccu_mission_v62_bean import CCULoopGroupV62Bean
from profiling_bean.hardware.ccu_mission_v62_bean import CCUSetCKBV62Bean
from profiling_bean.hardware.ccu_profile import CCU_V6_2_PROFILE
from profiling_bean.hardware.ccu_profile import CcuDataKind
from profiling_bean.hardware.ccu_profile import CcuHardwareProfileRegistry


class CCUMissionParser(MsMultiProcess):
    """
    class used to parser ccu mission data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._file_list = file_list.get(DataTag.CCU_MISSION, [])
        self._project_path = sample_config.get("result_dir", "")
        self._profile = CcuHardwareProfileRegistry.require_profile(
            sample_config.get(StrConstant.SAMPLE_CONFIG_CHIP_MODEL)
        )
        self._decoder = CcuDecoderRegistry.get_decoder(self._profile, CcuDataKind.MISSION)
        self._source_calculator = CcuSourceCalculator(
            self._file_list, self._profile, CcuDataKind.MISSION, self._decoder.record_size, self._project_path
        )
        self._is_v62 = self._profile == CCU_V6_2_PROFILE
        self._tables = (
            [
                DBNameConstant.TABLE_CCU_SETCKB_V62,
                DBNameConstant.TABLE_CCU_LOOPGROUP_V62,
                DBNameConstant.TABLE_CCU_MISSION_SOURCE_V62,
            ]
            if self._is_v62
            else [DBNameConstant.TABLE_CCU_MISSION]
        )
        self._model = CCUMissionModel(self._project_path, DBNameConstant.DB_CCU, self._tables)
        self.setckb_v62_data = []
        self.loopgroup_v62_data = []
        self.mission_data = []
        self._pending_complete_files = []
        self._parse_failed = False
        self._v62_sources = []
        self._v62_committed = False

    @staticmethod
    def get_single_mission_data(dev_id, end_time, single_bean):
        return [
            single_bean.stream_id,
            single_bean.task_id,
            single_bean.lp_instr_id,
            single_bean.lp_start_time,
            single_bean.lp_end_time,
            single_bean.setckebit_instr_id,
            single_bean.setckebit_start_time,
            dev_id,
            end_time,
        ]

    def read_binary_data(self: any, file_name: str) -> int:
        """
        parsing ccu mission data
        """
        files = PathManager.get_data_file_path(self._project_path, file_name)
        if not os.path.exists(files):
            return NumberConstant.ERROR
        _file_size = os.path.getsize(files)
        if _file_size <= 0:
            logging.error("CCU mission data file is empty, please check the file.")
            return NumberConstant.ERROR
        try:
            die_id, _ = self._profile.get_source_scope(CcuDataKind.MISSION, file_name)
        except ValueError as error:
            logging.error(str(error))
            return NumberConstant.ERROR
        with FileOpen(files, "rb") as f:
            mission_bin = self._source_calculator.pre_process(file_name, f.file_reader, _file_size)
            struct_nums = len(mission_bin) // self._decoder.record_size
            for i in range(struct_nums):
                single_bean = self._decoder.decode(
                    mission_bin[i * self._decoder.record_size : (i + 1) * self._decoder.record_size]
                )
                if not self._is_v62:
                    self.get_mission_data(single_bean)
                elif isinstance(single_bean, CCUSetCKBV62Bean):
                    self.setckb_v62_data.append(self._build_setckb_v62_row(single_bean, die_id))
                elif isinstance(single_bean, CCULoopGroupV62Bean):
                    self.loopgroup_v62_data.append(self._build_loopgroup_v62_row(single_bean, die_id))
                else:
                    logging.error("Invalid CCU v6.2 mission record %d in %s", i, file_name)
                    return NumberConstant.ERROR
        return NumberConstant.SUCCESS

    def get_mission_data(self, single_bean):
        if single_bean and len(single_bean.rel_end_time) > 0:
            for dev_id, end_time in single_bean.rel_end_time:
                self.mission_data.append(self.get_single_mission_data(dev_id, end_time, single_bean))
        elif single_bean and len(single_bean.rel_end_time) == 0:
            self.mission_data.append(
                self.get_single_mission_data(Constant.DEFAULT_VALUE, Constant.DEFAULT_VALUE, single_bean)
            )

    def parse(self: any) -> None:
        """
        parsing data file
        """
        logging.info("Start parsing CCU mission data file.")
        pending_files = [
            name for name in self._source_calculator.source_files if is_valid_original_data(name, self._project_path)
        ]
        # Completed files rely on their markers. The batch record is needed when a marker is missing.
        if self._is_v62 and pending_files:
            self._v62_sources = self._snapshot_v62_sources()
            try:
                committed_sources = (
                    self._model.get_all_data(DBNameConstant.TABLE_CCU_MISSION_SOURCE_V62)
                    if self._model.check_db()
                    else []
                )
            finally:
                self._model.finalize()
            if committed_sources:
                if self._v62_sources != sorted(committed_sources):
                    raise ValueError(
                        "CCU mission sources differ from the persisted batch; clear parsed output and reimport"
                    )
                self._v62_committed = True
                self._pending_complete_files = pending_files
                return
        for file_name in pending_files:
            self._handle_original_data(file_name)
        self._source_calculator.validate()
        if not self._is_v62:
            self._set_stream_id_by_host()
        logging.info("Parse CCU mission data finished!")

    def save(self: any) -> bool:
        """
        save data to db
        :return: None
        """
        if self._parse_failed:
            return False
        if self._v62_sources and self._v62_sources != self._snapshot_v62_sources():
            logging.error("CCU mission sources changed while parsing.")
            return False
        if (self._v62_committed or self._source_calculator.has_completed_files) and not ccu_table_exists(
            self._project_path, DBNameConstant.DB_CCU, *self._tables
        ):
            logging.error("Completed CCU mission source has no persisted table; clear markers and reimport")
            return False
        save_success = True
        if self._is_v62:
            if not self._v62_committed and (
                self._pending_complete_files or self.setckb_v62_data or self.loopgroup_v62_data
            ):
                with self._model as model:
                    save_success = model.flush_v62(self.setckb_v62_data, self.loopgroup_v62_data, self._v62_sources)
                self._v62_committed = save_success
        elif self._pending_complete_files or self.mission_data:
            time_fields = (3, 4, 6, 8)
            if any(any(row[index] > NumberConstant.INT64_MAX for index in time_fields) for row in self.mission_data):
                logging.error("CCU mission timestamp exceeds the SQLite integer range")
                return False
            with self._model as model:
                save_success = model.flush(self.mission_data, DBNameConstant.TABLE_CCU_MISSION)
        if save_success:
            save_success = mark_ccu_files_complete(
                self._project_path, self._pending_complete_files, FileManager.add_complete_file
            )
        if save_success:
            self._pending_complete_files.clear()
        return save_success

    def _snapshot_v62_sources(self: any) -> list:
        sources = []
        for name in sorted(set(self._source_calculator.source_files)):
            info = os.stat(PathManager.get_data_file_path(self._project_path, name))
            sources.append((name, info.st_size, info.st_mtime_ns))
        return sources

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        try:
            if self._file_list:
                self.parse()
                if not self.save():
                    logging.error("Save CCU mission data failed; source files remain incomplete.")
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _handle_original_data(self: any, file_name: str) -> None:
        if self.read_binary_data(file_name) == NumberConstant.ERROR:
            logging.error('Parse CCU mission data file: %s error.', file_name)
            self._parse_failed = True
            return
        self._pending_complete_files.append(file_name)

    def _set_stream_id_by_host(self: any):
        try:
            device_id = int(InfoConfReader().get_device_id())
        except (ValueError, TypeError) as err:
            logging.error("Device id is not a valid integer, skip setting stream id by host. Error: %s", err)
            return
        host_task_dict = self._get_host_task_stream_table(device_id)
        for data in self.mission_data:
            task_id = data[1]
            data[0] = host_task_dict.get(task_id, Constant.UINT16_MAX)

    def _get_host_task_stream_table(self, device_id):
        runtime_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_RUNTIME)
        if not os.path.isfile(runtime_path):
            return {}
        connection = None
        try:
            connection = sqlite3.connect(runtime_path)
            cursor = connection.cursor()
            table = cursor.execute(
                "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?",
                (DBNameConstant.TABLE_HOST_TASK,),
            ).fetchone()
            if table is None:
                logging.warning("No table %s.%s found", DBNameConstant.DB_RUNTIME, DBNameConstant.TABLE_HOST_TASK)
                return {}
            rows = cursor.execute(
                "SELECT task_id, stream_id FROM HostTask WHERE device_id=? ORDER BY timestamp",
                (device_id,),
            ).fetchall()
        except sqlite3.Error as error:
            raise RuntimeError("Query HostTask failed") from error
        finally:
            if connection is not None:
                connection.close()

        stream_by_task = {}
        for task_id, stream_id in rows:
            if task_id in stream_by_task:
                logging.error("Duplicate task_id found: %s", task_id)
                continue
            stream_by_task[task_id] = stream_id
        return stream_by_task

    @staticmethod
    def _build_setckb_v62_row(single_bean: any, die_id: int) -> list:
        return [
            single_bean.task_id,
            single_bean.setckb_instr_id,
            single_bean.loop_id,
            single_bean.loop_cnt,
            single_bean.setckb_ckbinfo,
            single_bean.setckb_syscnt,
            str(single_bean.udf_profiling_data),
            single_bean.udf_flag,
            single_bean.syscnt_type,
            single_bean.lp_flg,
            die_id,
        ]

    @staticmethod
    def _build_loopgroup_v62_row(single_bean: any, die_id: int) -> list:
        return [
            single_bean.task_id,
            single_bean.loopgroup_instr_id,
            single_bean.loopgroup_start_syscnt,
            single_bean.loopgroup_stop_syscnt,
            die_id,
        ]
