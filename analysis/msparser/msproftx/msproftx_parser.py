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
from common_func.file_manager import FileOpen
from common_func.hash_dict_constant import HashDictData
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from common_func.utils import Utils
from framework.offset_calculator import OffsetCalculator
from msmodel.msproftx.msproftx_model import MsprofTxModel, MsprofTxExModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.msproftx_decoder import MsprofTxDecoder


class MsprofTxParser(IParser, MsMultiProcess):
    """
    parsing MsprofTx data class
    """

    EVENT_DICT = {
        NumberConstant.MARKER: 'marker',
        NumberConstant.PUSH_AND_POP: 'push/pop',
        NumberConstant.START_AND_END: 'start/end',
        NumberConstant.MARKER_EX: 'marker_ex',
    }
    TX_INFO_TYPE = 0
    TX_EX_INFO_TYPE = 1

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list.get(DataTag.MSPROFTX, [])
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH, '')
        self._cur_file_list = []
        self._msproftx_data = []
        self._msproftx_ex_data_dict = {}

    def parse(self: any) -> None:
        """
        parsing data file
        """
        for file_name in self._file_list:
            if is_valid_original_data(file_name, self._project_path):
                logging.info("start parsing file: %s", file_name)
                try:
                    self._read_binary_data(file_name)
                except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
                    logging.error("%s: %s", file_name, err, exc_info=Constant.TRACE_BACK_SWITCH)
                logging.info("Parsing %s data file finished!", file_name)

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if self._msproftx_data:
            with MsprofTxModel(
                self._project_path, DBNameConstant.DB_MSPROFTX, [DBNameConstant.TABLE_MSPROFTX]
            ) as tx_model:
                tx_model.flush(self._msproftx_data)
        msproftx_ex_data = []
        hash_data = HashDictData(self._project_path).get_ge_hash_dict()
        if self._msproftx_ex_data_dict:
            for mark_id, obj_list in self._msproftx_ex_data_dict.items():
                obj_list.sort(key=lambda x: x.seg_idx)  # sort by seg_idx
                seg_all = [obj.seg_idx for obj in obj_list]

                # seg_idx should be continuous and start from 0, if not, log a warning
                missing_segs = set(range(0, obj_list[-1].seg_idx + 1)) - set(seg_all)
                if missing_segs:
                    logging.warning(
                        "mark_id=%s segment lost, missing seg index: %s, received seg indices: %s",
                        mark_id,
                        missing_segs,
                        seg_all,
                    )

                full_msg = "".join(obj.message for obj in obj_list)  # concatenate message by seg_idx
                first_obj = obj_list[0]
                domain_str = str(first_obj.domain)
                final_row = (
                    first_obj.process_id,
                    first_obj.thread_id,
                    self.EVENT_DICT.get(first_obj.event_type, ''),
                    first_obj.start_time,
                    first_obj.end_time,
                    mark_id,
                    hash_data.get(domain_str, 'invalid'),
                    full_msg,
                )
                msproftx_ex_data.append(final_row)
            if msproftx_ex_data:
                with MsprofTxExModel(
                    self._project_path, DBNameConstant.DB_MSPROFTX, [DBNameConstant.TABLE_MSPROFTX_EX]
                ) as tx_ex_model:
                    tx_ex_model.flush(msproftx_ex_data)

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        if self._file_list:
            self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
            self.parse()
            try:
                self.save()
            except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
                logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _read_binary_data(self: any, file_name: str):
        """
        parsing msproftx data and insert into msproftx.db
        """
        calculate = OffsetCalculator(self._cur_file_list, StructFmt.MSPROFTX_FMT_SIZE, self._project_path)
        msproftx_file = PathManager.get_data_file_path(self._project_path, file_name)
        with FileOpen(msproftx_file, 'rb') as msproftx_f:
            msproftx_data = calculate.pre_process(msproftx_f.file_reader, os.path.getsize(msproftx_file))
            for chunk in Utils.chunks(msproftx_data, StructFmt.MSPROFTX_FMT_SIZE):
                data_object = MsprofTxDecoder.decode(chunk)
                if data_object.info_type == MsprofTxParser.TX_INFO_TYPE:
                    self._msproftx_data.append(
                        (
                            data_object.process_id,
                            data_object.thread_id,
                            data_object.category,
                            self.EVENT_DICT.get(data_object.event_type, ''),
                            data_object.payload_type,
                            data_object.payload_value,
                            data_object.start_time,
                            data_object.end_time,
                            data_object.message_type,
                            data_object.message,
                        )
                    )
                elif data_object.info_type == MsprofTxParser.TX_EX_INFO_TYPE:
                    mark_id = data_object.mark_id
                    if mark_id not in self._msproftx_ex_data_dict:
                        self._msproftx_ex_data_dict[mark_id] = []
                    self._msproftx_ex_data_dict[mark_id].append(data_object)
                else:
                    logging.error("Invalid info_type: %d", data_object.info_type)
