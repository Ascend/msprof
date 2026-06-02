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

"""
The MsprofOutputSummary's function:
1 search all file in summary/timeline dir,and save their name in set
2 cyclically reads the file by the name in the set, with multiprocess
3 dump by FileSliceHelper

for timeline:
find the file by name in timeline dir, and classify them by the slice number
                       ----------   ----------   ----------         ----------
process_0---slice_0----|device_0|   |device_1|   |device_2|   ...   |device_*|---> gather all to slice_0.json
                       ----------   ----------   ----------         ----------
                       ----------   ----------   ----------         ----------
process_1---slice_1----|device_0|   |device_1|   |device_2|   ...   |device_*|---> gather all to slice_1.json
                       ----------   ----------   ----------         ----------
   ...        ...         ....         ....         ....      ...      ....
                       ----------   ----------   ----------         ----------
process_*---slice_*----|device_0|   |device_1|   |device_2|   ...   |device_*|---> gather all to slice_*.json
                       ----------   ----------   ----------         ----------
In slice_*, will be 8 or more device's data, all off them will be gathered,
and dump to a file named slice_*.json.
Enable multiple processes based on the number of slice.
"""

import logging
import multiprocessing
import os
import re
import shutil

from common_func.common import print_info
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.file_manager import FdOpen
from common_func.file_manager import FileOpen
from common_func.file_slice_helper import FileSliceHelper
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msprof_common import get_path_dir
from common_func.msprof_common import get_valid_sub_path
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_number
from common_func.path_manager import PathManager
from common_func.utils import Utils
from msconfig.config_manager import ConfigManager


class MsprofOutputSummary:
    """
    class used to export all job data.
    """

    DEVICE_PREFIX = "device_"
    INVALID_SUFFIX = "invalid"
    MSPROF_HOST = "host"
    DEVICE_ID = "Device_id"
    DEVICE_ID_PREFIX_LEN = 7
    SLICE_LEN = 7
    README = "README.txt"
    FILE_MAX_SIZE = 1024 * 1024 * 1024
    JSON_LIST = ["msprof", "step_trace", "msprof_tx"]
    MSPROF_TX = "msprof_tx"
    MSPROFTX_DEVICE_START_TIME_HEADER = "Device Start_time(us)"
    MSPROFTX_DEVICE_END_TIME_HEADER = "Device End_time(us)"

    def __init__(self: any, output: str, export_format: str) -> None:
        self._output = output
        self._export_format = export_format
        self._output_dir = ""
        self._log_dir = ""
        self._msproftx_device_data_dict = {}
        self._insert_match_summary_list = {"msprof_tx": self._insert_msproftx_summary_data}

    @staticmethod
    def _valid_pos(underscore_pos: int, point_pos: int, filename: str) -> bool:
        if underscore_pos < 1 or point_pos < 0:
            return False
        elif underscore_pos > len(filename) - 1 or point_pos > len(filename):
            return False
        elif underscore_pos > point_pos:
            return False
        return True

    @staticmethod
    def _get_file_name(file_name: str) -> str:
        """
        get filename like "op_summary"
        """
        match = re.search(r'(_\d)?(_slice_\d+)?_\d+', file_name)
        if match and match.start() > 0:
            return file_name[: match.start()]
        logging.warning("The file name  %s is invalid!", file_name)
        return "invalid"

    @staticmethod
    def _get_readme_info(file_set: set, file_dict: dict, suffix: str) -> str:
        context = file_dict.get("begin", "")
        for index, filename in enumerate(file_set):
            desc = file_dict.get(filename)
            if not desc:
                desc = (
                    "Here is no description about this file: "
                    + filename
                    + ", please check in 'Profiling Instructions'!\n"
                )
            context += f"{str(index + 1)}.{filename}{suffix}:{desc}"
        context += "\n"
        return context

    @classmethod
    def get_newest_file_list(cls, file_list, data_type: str) -> list:
        """
        filename is key,timestamp is value,update time to get the newest file list
        """
        file_dict = {}
        for filename in file_list:
            if filename.endswith(data_type):
                underscore_pos = filename.rfind("_")
                point_pos = filename.rfind(".")
                if not MsprofOutputSummary._valid_pos(underscore_pos, point_pos, filename):
                    logging.warning("The file name  %s is invalid!", filename)
                    continue
                time_str = filename[underscore_pos + 1 : point_pos]
                if not is_number(time_str):
                    logging.warning("The file name  %s is invalid!", filename)
                    continue
                time = int(time_str)
                key = filename[: underscore_pos + 1]
                value = file_dict.get(key, 0)
                if not value or value < time:
                    file_dict.update({key: time})
        return [k + str(v) + data_type for k, v in file_dict.items()]

    @classmethod
    def read_file(cls, reader):
        while True:
            line = reader.readline()
            if not line:
                break
            yield line

    def export(self: any, command_type: str) -> None:
        """
        export all data
        :return:
        """
        print_info(MsProfCommonConstant.COMMON_FILE_NAME, f"Start exporting {command_type} output_file.")
        if not self._is_in_prof_file():
            return
        if not self._make_output_folder(self._get_file_suffix(command_type)):
            logging.error("Clear file in mindstudio_profiler_output.")
            return
        if command_type == MsProfCommonConstant.SUMMARY:
            self._export_msprof_summary()
        elif command_type == MsProfCommonConstant.TIMELINE:
            self._export_msprof_timeline()
        self._export_readme_file()
        output_path = os.path.join(self._output, PathManager.MINDSTUDIO_PROFILER_OUTPUT)
        print_info(
            MsProfCommonConstant.COMMON_FILE_NAME,
            f"End exporting {command_type} output_file.The file is stored in the {output_path} path.",
        )

    def _is_in_prof_file(self):
        """
        If the current directory contains host or device_* file
        Then consider this directory as PROF_XXX
        """
        file_list = os.listdir(self._output)
        for file_name in file_list:
            if file_name == self.MSPROF_HOST or file_name.startswith(self.DEVICE_PREFIX):
                return True
        return False

    def _get_file_suffix(self, command_type: str) -> str:
        """
        get summary or timeline's suffix, ".csv" or ".json"
        """
        if command_type == MsProfCommonConstant.SUMMARY and self._export_format == StrConstant.EXPORT_JSON:
            return StrConstant.FILE_SUFFIX_JSON
        if command_type == MsProfCommonConstant.SUMMARY and self._export_format == StrConstant.EXPORT_CSV:
            return StrConstant.FILE_SUFFIX_CSV
        if command_type == MsProfCommonConstant.TIMELINE:
            return StrConstant.FILE_SUFFIX_JSON

        logging.error("%s is invalid, can't export this type file.", command_type)
        return self.INVALID_SUFFIX

    def _make_output_folder(self, suffix: str) -> bool:
        if suffix == self.INVALID_SUFFIX:
            return False
        self._output_dir = os.path.join(self._output, PathManager.MINDSTUDIO_PROFILER_OUTPUT)
        if not os.path.exists(self._output_dir):
            os.makedirs(self._output_dir, Constant.FOLDER_MASK)
        return True

    def _export_msprof_summary(self):
        # summary 是 json格式的时候，按文件拷贝
        if self._export_format == StrConstant.EXPORT_JSON:
            sub_dirs = get_path_dir(self._output)
            for sub_dir in sub_dirs:
                self._copy_summary_data(sub_dir, StrConstant.FILE_SUFFIX_JSON)
            return
        else:
            self._merge_summary()

    def _copy_summary_data(self, folder_name: str, file_suffix: str, is_merge_summary=False):
        """
        拷贝summary文件夹下所有数据到output目录下，host可以看作一种特殊的device
        """
        if not (folder_name == self.MSPROF_HOST or folder_name.startswith(self.DEVICE_PREFIX)):
            return
        summary_path = os.path.realpath(os.path.join(self._output, folder_name, MsProfCommonConstant.SUMMARY))
        if not os.path.exists(summary_path):
            return
        file_list = self.get_newest_file_list(os.listdir(summary_path), file_suffix)
        for file_name in file_list:
            # host 和device 的 csv合并
            if is_merge_summary is False:
                params = {
                    StrConstant.PARAM_RESULT_DIR: self._output,
                    StrConstant.PARAM_DATA_TYPE: self._get_file_name(file_name),
                    StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.SUMMARY,
                    StrConstant.PARAM_EXPORT_FORMAT: StrConstant.EXPORT_JSON,
                    StrConstant.PARAM_EXPORT_DUMP_FOLDER: PathManager.MINDSTUDIO_PROFILER_OUTPUT,
                }
                shutil.copy(os.path.join(summary_path, file_name), FileSliceHelper.make_export_file_name(params))

    def _merge_summary(self):
        """
        merge csv file if they have the same name
        op_summary_0_0_1 and op_summary_0_0_2 both op_summary
        """
        sub_dirs = get_path_dir(self._output)
        summary_file_set = set()
        for sub_dir in sub_dirs:
            sub_path = get_valid_sub_path(self._output, sub_dir, False)
            if not DataCheckManager.contain_info_json_data(sub_path):
                continue
            summary_file_set.update(self._get_summary_file_name(sub_path))
            self._copy_summary_data(sub_dir, StrConstant.FILE_SUFFIX_JSON, True)

        error = "Output: An exception occurs when multiple processes process the summary file. The error is %s"
        pool = multiprocessing.Pool(processes=4)  # pylint: disable=R1732
        for summary_file in summary_file_set:
            pool.apply_async(
                func=self._save_summary_data,
                args=(summary_file, sub_dirs),
                error_callback=lambda error_info: logging.error(error, error_info),
            )
        pool.close()
        pool.join()

    def _get_summary_file_name(self, device_path: str) -> set:
        """
        get target summary file in summary dir
        """
        device_summary_set = set()
        summary_path = os.path.realpath(os.path.join(device_path, MsProfCommonConstant.SUMMARY))
        if not os.path.exists(summary_path):
            return device_summary_set
        file_list = os.listdir(summary_path)
        for file_name in self.get_newest_file_list(file_list, StrConstant.FILE_SUFFIX_CSV):
            device_summary_set.add(self._get_file_name(file_name))
        return device_summary_set

    def _save_summary_data(self, targe_name: str, sub_dirs: list):
        """
        get summary_data then create or open csv file in target dir
        """
        params = {
            StrConstant.PARAM_DATA_TYPE: targe_name,
            StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.SUMMARY,
            StrConstant.PARAM_EXPORT_FORMAT: self._export_format,
            StrConstant.PARAM_RESULT_DIR: self._output,
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: PathManager.MINDSTUDIO_PROFILER_OUTPUT,
        }
        helper = FileSliceHelper(params, [], [])
        for sub_dir in sorted(sub_dirs):
            sub_path = get_valid_sub_path(self._output, sub_dir, False)
            if not DataCheckManager.contain_info_json_data(sub_path):
                continue
            summary_path = os.path.realpath(os.path.join(sub_path, MsProfCommonConstant.SUMMARY))
            if not os.path.exists(summary_path):
                continue
            file_list = os.listdir(summary_path)
            if sub_dir == self.MSPROF_HOST:
                device_id = "host"
            else:
                device_id = os.path.basename(sub_path)[self.DEVICE_ID_PREFIX_LEN :]
            for file_name in self.get_newest_file_list(file_list, StrConstant.FILE_SUFFIX_CSV):
                if not file_name.startswith(targe_name) or (targe_name == "aicpu" and file_name.startswith("aicpu_mi")):
                    continue
                file_name_path = os.path.join(summary_path, file_name)
                if targe_name in self._insert_match_summary_list:
                    self._insert_match_summary_list[targe_name](file_name_path, device_id, helper, sub_dir)
                else:
                    self._insert_summary_data(file_name_path, device_id, helper)
        helper.dump_csv_data(force=True)

    def _insert_msproftx_summary_data(self, file_name_path: str, device_id: str, helper: FileSliceHelper, sub_dir: str):
        if sub_dir == self.MSPROF_HOST:
            self._update_msproftx_host_data(file_name_path, helper)
        else:
            self._update_msproftx_device_data(file_name_path, device_id)

    def _update_msproftx_device_data(self, file_name_path: str, device_id: str):
        cnt = 0
        with FileOpen(file_name_path, mode='r', max_size=self.FILE_MAX_SIZE) as _csv_file:
            _csv_file.file_reader.readline()
            line_num = 0
            for index, row in enumerate(self.read_file(_csv_file.file_reader)):
                line_num = index + 1
                if line_num > FileSliceHelper.CSV_LIMIT:
                    logging.error(
                        "The CSV file size limit is %d rows, and the size of the %s file has exceeded the limit. ",
                        FileSliceHelper.CSV_LIMIT,
                        file_name_path,
                    )
                    self._msproftx_device_data_dict = {}
                    return
                data = row.split(',')
                if len(data) < 3:
                    cnt += 1
                    continue
                self._msproftx_device_data_dict.setdefault(data[0], []).append(
                    {"start_time": data[1], "end_time": data[2], "device_id": device_id}
                )
            if cnt != 0:
                logging.error("The MSPROF_TX_DEVICE_CSV file contains %d lines whose length is less than 3. ", cnt)

    def _update_msproftx_host_data(self, file_name_path: str, helper: FileSliceHelper):
        max_limit = FileSliceHelper.CSV_LIMIT
        data_map = self._msproftx_device_data_dict
        all_data = []

        with FileOpen(file_name_path, mode='r', max_size=self.FILE_MAX_SIZE) as _csv_file:
            # handle csv header
            header = _csv_file.file_reader.readline()
            if header and helper.check_header_is_empty():
                header_parts = header.strip().split(',')[:-1]
                csv_header = [
                    self.DEVICE_ID,
                    *header_parts,
                    self.MSPROFTX_DEVICE_START_TIME_HEADER,
                    self.MSPROFTX_DEVICE_END_TIME_HEADER,
                ]
                csv_header[-1] += '\n'
                helper.set_header(csv_header)

            for row in self.read_file(_csv_file.file_reader):
                if len(all_data) >= max_limit:
                    logging.error(
                        "The CSV file size limit is %d rows, and the size of the %s file has exceeded the limit. ",
                        FileSliceHelper.CSV_LIMIT,
                        file_name_path,
                    )
                    return
                stripped_row = row.strip()
                mark_id = stripped_row.split(',')[-1]
                origin_row = row.rsplit(',', 1)[0]

                # match multi tx data with same mark id
                item_list = data_map.get(mark_id)
                if item_list:
                    for item in item_list:
                        start_t = item["start_time"]
                        end_t = item["end_time"]
                        dev_id = item["device_id"]
                        new_row = ','.join([origin_row, start_t, end_t])
                        all_data.append(f"{dev_id},{new_row}")

                    if len(all_data) >= max_limit:
                        logging.error(
                            "The CSV file size limit is %d rows, and the size of the %s file has exceeded the limit. ",
                            FileSliceHelper.CSV_LIMIT,
                            file_name_path,
                        )
                        return

                else:
                    new_row = ','.join([origin_row, Constant.NA, Constant.NA]) + '\n'
                    all_data.append(f"host,{new_row}")

            helper.insert_data(all_data)

    def _insert_summary_data(self, file_name_path: str, device_id: str, helper: FileSliceHelper):
        with FileOpen(file_name_path, mode='r', max_size=self.FILE_MAX_SIZE) as _csv_file:
            header = _csv_file.file_reader.readline()
            if header and helper.check_header_is_empty():
                csv_header = [self.DEVICE_ID, *list(header.split(','))]
                helper.set_header(csv_header)

            line_num = 0
            all_data = [''] * FileSliceHelper.CSV_LIMIT
            for index, row in enumerate(self.read_file(_csv_file.file_reader)):
                line_num = index + 1
                if line_num > FileSliceHelper.CSV_LIMIT:
                    logging.error(
                        "The CSV file size limit is %d rows, and the size of the %s file has exceeded the limit. ",
                        FileSliceHelper.CSV_LIMIT,
                        file_name_path,
                    )
                    return
                all_data[index] = f'{device_id},{row}'
            helper.insert_data(all_data[:line_num])

    def _export_msprof_timeline(self: any) -> None:
        self._export_all_timeline_data()

    def _export_all_timeline_data(self: any) -> None:
        """
        get json file from different dir,and then to load
        """
        sub_dirs = get_path_dir(self._output)
        processes = []
        for json_file in self.JSON_LIST:
            try:
                process = multiprocessing.Process(target=self._save_timeline_data, args=(json_file, sub_dirs))
                process.start()
                processes.append(process)
            except ProfException as err:
                logging.error(
                    "Output: An exception occurs when multiple processes process the timeline file. The error is %s",
                    err,
                )
                return
        for process in processes:
            process.join()

    def _save_timeline_data(self, targe_name: str, sub_dirs: list):
        timeline_file_dict = {}
        slice_max_count = 0
        for sub_dir in sub_dirs:
            sub_path = get_valid_sub_path(self._output, sub_dir, False)
            if not DataCheckManager.contain_info_json_data(sub_path):
                continue
            timeline_file_dict, slice_count = self._get_timeline_file_with_slice(
                targe_name, sub_path, timeline_file_dict
            )
            slice_max_count = max(slice_count, slice_max_count)

        params = {
            StrConstant.PARAM_DATA_TYPE: targe_name,
            StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.TIMELINE,
            StrConstant.PARAM_EXPORT_FORMAT: self._export_format,
            StrConstant.PARAM_RESULT_DIR: self._output,
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: PathManager.MINDSTUDIO_PROFILER_OUTPUT,
        }

        error = "Output: An exception occurs when multiple processes process the timeline file. The error is %s"
        pool = multiprocessing.Pool(processes=4)  # pylint: disable=R1732
        is_need_slice = bool(slice_max_count)
        for index in range(slice_max_count + 1):
            helper = FileSliceHelper(params, [], [])
            pool.apply_async(
                func=self._insert_json_data,
                args=(timeline_file_dict.get(index, []), helper, is_need_slice, index),
                error_callback=lambda error_info: logging.error(error, error_info),
            )
        pool.close()
        pool.join()

    def _insert_json_data(self, file_list: list, helper: FileSliceHelper, is_need_slice: bool, slice_index: int):
        """
        1 one device only have one "slice_0"
        2 if only one device, then no need to merge file, copy will be better.
        """
        if len(file_list) == 1:
            params = {
                StrConstant.PARAM_RESULT_DIR: self._output,
                StrConstant.PARAM_DATA_TYPE: self._get_file_name(os.path.basename(file_list[0])),
                StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.TIMELINE,
                StrConstant.PARAM_EXPORT_FORMAT: self._export_format,
                StrConstant.PARAM_EXPORT_DUMP_FOLDER: PathManager.MINDSTUDIO_PROFILER_OUTPUT,
            }
            shutil.copy(
                file_list[0], FileSliceHelper.make_export_file_name(params, slice_index, slice_switch=is_need_slice)
            )
            return
        for _file_name in file_list:
            helper.insert_data(Utils.get_json_data(_file_name))
        helper.dump_json_data(slice_index, is_need_slice=is_need_slice)

    def _get_timeline_file_with_slice(self, target_name: str, dir_path: str, timeline_file_dict: dict) -> tuple:
        """
        Differentiate files with the same name by slice_count.
        in timeline_file_dict:
        0:xxx_slice_0.json, xxx.json
        1:xxx_slice_1.json
        """
        slice_max_count = 0
        timeline_path = os.path.realpath(os.path.join(dir_path, MsProfCommonConstant.TIMELINE))
        if not os.path.exists(timeline_path):
            return timeline_file_dict, slice_max_count
        file_list = os.listdir(timeline_path)
        for _file_name in self.get_newest_file_list(file_list, StrConstant.FILE_SUFFIX_JSON):
            if not _file_name.startswith(target_name) or (
                target_name == "msprof" and _file_name.startswith("msprof_tx")
            ):
                continue
            match = re.search(r'_slice_\d+', _file_name)
            file_name = os.path.join(timeline_path, _file_name)
            slice_count = 0
            if match and match.start() > 0:
                slice_count = _file_name[match.start() + self.SLICE_LEN : match.end()]
                if not is_number(slice_count):
                    logging.warning("This file name is invalid: %s", file_name)
                    continue
                slice_max_count = max(int(slice_count), slice_max_count)
            slice_file_list = timeline_file_dict.get(int(slice_count), [])
            slice_file_list.append(file_name)
            timeline_file_dict.update({int(slice_count): slice_file_list})
        return timeline_file_dict, slice_max_count

    def _export_readme_file(self):
        cfg_data = ConfigManager.get("FilenameIntroductionConfig").DATA
        timeline_dict = dict(cfg_data.get(MsProfCommonConstant.TIMELINE))
        timeline_set = set()
        summary_dict = dict(cfg_data.get(MsProfCommonConstant.SUMMARY))
        summary_set = set()
        file_list = os.listdir(self._output_dir)
        for ori_filename in file_list:
            if ori_filename.endswith(StrConstant.FILE_SUFFIX_CSV):
                summary_set.add(self._get_file_name(ori_filename))
            elif ori_filename.endswith(StrConstant.FILE_SUFFIX_JSON):
                timeline_set.add(self._get_file_name(ori_filename))
        file_path = os.path.join(self._output_dir, self.README)
        with FdOpen(file_path) as readme:
            context = self._get_readme_info(timeline_set, timeline_dict, StrConstant.FILE_SUFFIX_JSON)
            context += self._get_readme_info(summary_set, summary_dict, StrConstant.FILE_SUFFIX_CSV)
            readme.write(context)
