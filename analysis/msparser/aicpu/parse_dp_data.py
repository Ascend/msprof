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
import struct

from common_func.common import warn
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.empty_class import EmptyClass
from common_func.file_manager import FileOpen
from common_func.file_name_manager import get_data_preprocess_compiles
from common_func.file_name_manager import get_file_name_pattern_match
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import is_valid_original_data
from common_func.db_manager import DBManager
from common_func.path_manager import PathManager
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel


class ParseDpData:
    """
    class for parse dp data
    """

    DP_MARKS = "Last queue dequeue"
    TAG_DP = "DP"
    DEFAULT_BATCH = 32
    BIN_DP_HEADER_FMT = "=HH"
    BIN_DP_DATA_TAG = 100
    DP_FILE_STR_TYPE = "str_or_bytes"
    DP_FILE_BIN_TYPE = "bin"
    DP_BIN_START_TAG = "="
    DP_DATA_FMT = "HHIQ16s64sQQ2Q"
    DP_DATA_FMT_SIZE = 128
    DP_TUPLE_LENGTH = 10

    FILE_NAME = os.path.basename(__file__)

    @staticmethod
    def get_dp_judge_data(dp_file: list) -> tuple:
        """
        calculate Data aging
        """
        dp_file.sort(key=lambda x: int(x.split("_")[-1]), reverse=True)
        total_size = 0
        size_list = []
        for file in dp_file:
            size_list.append(os.path.getsize(file))
            total_size += os.path.getsize(file)
        if total_size < ParseDpData.DP_DATA_FMT_SIZE:
            return EmptyClass(str("Insufficient file size")), 0
        judge_file = dp_file[0]
        offset = 0
        if size_list[0] < ParseDpData.DP_DATA_FMT_SIZE:
            offset = size_list[0]
            judge_file = dp_file[1]
        return judge_file, offset

    @staticmethod
    def get_dp_tuple(index: int, dp_data: tuple) -> tuple:
        """
        split dp data into dp_tuple
        """
        dp_data_length = index * ParseDpData.DP_TUPLE_LENGTH
        timestamp = InfoConfReader().trans_into_local_time(dp_data[dp_data_length + 3])
        action = dp_data[dp_data_length + 4].partition(b"\x00")[0].decode("utf-8", "ignore")
        source = dp_data[dp_data_length + 5].partition(b"\x00")[0].decode("utf-8", "ignore")
        size = dp_data[dp_data_length + 7]
        dp_data_message = (float(timestamp), action, source, size)
        return dp_data_message

    @classmethod
    def get_files(cls: any, path: str, tag: any, device_id: any) -> list:
        """
        function used to get parsing file list
        """
        files = []
        project_path = os.path.dirname(path)
        for file_ in os.listdir(path):
            res = get_file_name_pattern_match(file_, *get_data_preprocess_compiles(tag))
            if res and is_valid_original_data(file_, project_path):
                files.append(os.path.join(path, file_))
        files.sort(key=lambda x: int(x.split("_")[-1]))

        if not files:
            return files
        # file name should be longer than 3, so split(".")[-2] is in the safe range
        # file name is in the format DATAPREPROCESS.dev.AICPU.0.slice_0
        if len(files[0].split(".")) < Constant.LINE_LEN or int(device_id) != int(files[0].split(".")[(-2)]):
            warn(cls.FILE_NAME, 'The file name "%s" is not correct.' % files[0])
        return files

    @classmethod
    def analyse_dp(cls: any, dp_path: str, device_id: any) -> any:
        """
        analysis dp data
        """
        files = cls.get_files(dp_path, cls.TAG_DP, device_id)
        db_path = PathManager.get_db_path(os.path.dirname(dp_path), DBNameConstant.DB_AI_CPU)
        if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_AI_CPU_DP):
            with AiCpuModel(os.path.dirname(dp_path)) as model:
                data = model.get_all_data(DBNameConstant.TABLE_AI_CPU_DP)
            # 表内 timestamp 为 ns（Python/C++ 落盘对齐），CSV 头为 Timestamp(us)
            # 与 C++ LoadDpData 的 ORDER BY timestamp 对齐，保证 dp.csv 行序可重现
            data.sort(key=lambda row: row[0])
            return [
                (
                    float(InfoConfReader().trans_into_local_time(float(row[0]))),
                    row[1],
                    row[2],
                    row[3],
                )
                for row in data
            ]
        if ParseDpData.dp_data_dispatch(files) == cls.DP_FILE_BIN_TYPE:
            return ParseDpData.analyse_bin_dp(files)
        lines = []
        infos = {cls.DP_MARKS: []}
        if not files:
            return []
        for file_ in files:
            with FileOpen(file_, "rb") as file_reader:
                # replace \n and \x00 in lines
                file_str = str(file_reader.file_reader.read().replace(b"\n\x00", b" ___ ").replace(b"\x00", b" ___ "))
                if len(file_str) > Constant.LINE_LEN:
                    lines += str(file_str)[2:-1].split(" ___ ")
        for line in lines:
            if cls.DP_MARKS in line:
                infos.get(cls.DP_MARKS).append(line)
        data = []
        for info in infos.get(cls.DP_MARKS):
            # 3 is used to ensure the length of info.split(",") longer than 3
            info_split = info.split(",")
            if len(info_split) > Constant.LINE_LEN:
                # info are in the following format
                # [13135969231] Last queue dequeue, source:iterator_default, index:1, size:131
                timestamp = InfoConfReader().trans_into_local_time(float(info_split[0].split("]")[0].strip("[")))
                action = info_split[0].split("]")[-1].strip()
                source = info_split[-3].split(":")[-1]
                size = info_split[-1].split(":")[-1]
                data.append((float(timestamp), action, source, size))
        return data

    @classmethod
    def dp_data_dispatch(cls: any, dp_file: list) -> str:
        """
        check the dp data and pick up the correct parser to analysis
        """
        judge_file, offset = ParseDpData.get_dp_judge_data(dp_file)
        if not judge_file:
            return cls.DP_FILE_STR_TYPE
        file_size = os.path.getsize(judge_file)
        if ParseDpData.DP_DATA_FMT_SIZE - offset < struct.calcsize(cls.BIN_DP_HEADER_FMT):
            offset -= ParseDpData.DP_DATA_FMT_SIZE
        try:
            with FileOpen(judge_file, "rb") as dp_f:
                _ = dp_f.file_reader.read(file_size + offset - ParseDpData.DP_DATA_FMT_SIZE)
                magic_num, data_tag = struct.unpack(
                    cls.BIN_DP_HEADER_FMT,
                    dp_f.file_reader.read(struct.calcsize(cls.BIN_DP_HEADER_FMT)),
                )
                if magic_num == NumberConstant.MAGIC_NUM and data_tag == cls.BIN_DP_DATA_TAG:
                    return cls.DP_FILE_BIN_TYPE
                return cls.DP_FILE_STR_TYPE
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return cls.DP_FILE_STR_TYPE

    @classmethod
    def analyse_bin_dp(cls: any, dp_files: list) -> any:
        """
        analyse bin dp data
        """
        if not dp_files:
            return []
        origin_data = []
        try:
            for file_ in dp_files:
                origin_data = ParseDpData.read_bin_data(file_)
            return origin_data
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return []

    @classmethod
    def read_bin_data(cls: any, file_: str) -> list:
        """
        read DP data in binary format
        """
        origin_data = []
        file_size = os.path.getsize(file_)
        try:
            with FileOpen(file_, "rb") as file_reader:
                dp_bin_data = file_reader.file_reader.read()
                struct_nums = file_size // cls.DP_DATA_FMT_SIZE
                dp_data = struct.unpack(cls.DP_BIN_START_TAG + cls.DP_DATA_FMT * struct_nums, dp_bin_data)
                for index in range(struct_nums):
                    origin_data.append(ParseDpData.get_dp_tuple(index, dp_data))
            return origin_data
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return []
