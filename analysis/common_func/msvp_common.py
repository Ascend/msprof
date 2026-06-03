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

import configparser
import csv
import json
import logging
import os
import shutil
from collections import OrderedDict
from decimal import Decimal
from functools import reduce
from operator import add
from operator import mul
from operator import sub
from operator import truediv

from common_func.common import error
from common_func.constant import Constant
from common_func.file_manager import FdOpen
from common_func.file_manager import check_path_valid
from common_func.file_manager import is_link
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from common_func.return_code_checker import ReturnCodeCheck
from msconfig.config_manager import ConfigManager


class MsvpCommonConst:
    """
    msvp common
    """

    # cpu_config_type
    TS_CPU = "ts_cpu"
    AI_CPU = "ai_cpu"
    AI_CORE = "ai_core"
    CTRL_CPU = "ctrl_cpu"
    CONSTANT = "constant"

    CONFIG_PATH = os.path.join(os.path.realpath(os.path.dirname(__file__)), "..", "msconfig")
    FILE_NAME = os.path.basename(__file__)
    CPU_CONFIG_TYPE = {
        TS_CPU: ConfigManager.get("TsCPUConfig"),
        AI_CPU: ConfigManager.get("AICPUConfig"),
        AI_CORE: ConfigManager.get("AICoreConfig"),
        CTRL_CPU: ConfigManager.get("CtrlCPUConfig"),
        CONSTANT: ConfigManager.get("ConstantConfig"),
    }

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return "MsvpCommonConst"

    def file_name(self: any) -> str:
        """
        file name
        """
        return self.FILE_NAME


def config_file_obj(file_name: str = "constant") -> any:
    """
    :param file_name: config file name
    :return: config file object
    """
    try:
        config = MsvpCommonConst.CPU_CONFIG_TYPE.get(file_name)
        return config
    except configparser.Error:
        return []
    finally:
        pass


def _get_cpu_metrics(sections: str, cpu_cfg: any) -> dict:
    if sections == 'metrics':
        metrics_map = cpu_cfg.items(sections)
        metrics = OrderedDict(metrics_map)
        return metrics
    if sections == 'formula':
        return OrderedDict(cpu_cfg.items("formula"))
    if sections == 'formula_l2':
        return OrderedDict(cpu_cfg.items("formula_l2"))
    if sections == 'custom':
        return OrderedDict(cpu_cfg.items("custom"))
    return {}


def read_cpu_cfg(cpu_type: str, sections: str) -> dict:
    """
    read cpu configure file
    :param cpu_type: cpu type
    :param sections: section for cpu
    :return:
    """
    cpu_cfg = MsvpCommonConst.CPU_CONFIG_TYPE.get(cpu_type)
    try:
        if sections in ['events', 'event2metric']:
            events_map = cpu_cfg.items(sections)
            events = {int(k, Constant.HEX_NUMBER): v for k, v in events_map}
            return events
        return _get_cpu_metrics(sections, cpu_cfg)
    except configparser.Error:
        return {}
    finally:
        pass


def get_cpu_event_config(sample_config: dict, cpu_type: str) -> list:
    """
    get cpu event from config
    :param sample_config: sample config
    :param cpu_type: cpu type
    :return: cpu events
    """
    events = []
    for _events in sample_config.get(cpu_type + "_profiling_events").split(","):
        if _events != '0x11':
            events.append(_events)
    cpu_events = []
    new_events = (events[i : i + 6] for i in range(0, len(events), 6))
    for j in new_events:
        cpu_events.append(j)
    if cpu_type == 'ai_ctrl_cpu':
        cpu_events[0] += ['r11']
    else:
        cpu_events[0] += ['0x11']
    return cpu_events


def get_cpu_event_chunk(sample_config: dict, cpu_type: str) -> list:
    """
    get cpu event chunk
    :param sample_config: sample config
    :param cpu_type: cpu type
    :return: cpu events
    """
    return get_cpu_event_config(sample_config, cpu_type)


def _do_change_file_mod(file_path: str) -> None:
    files_list = []
    file_path = os.path.realpath(file_path)
    for lists in os.listdir(file_path):
        path = os.path.join(file_path, lists)
        files_list.append(path)
        if not is_link(path) and os.path.isdir(path):
            files_chmod(path)
    for data_file in files_list:
        if not os.path.isdir(data_file):
            os.chmod(data_file, NumberConstant.FILE_AUTHORITY)
        else:
            os.chmod(data_file, NumberConstant.DIR_AUTHORITY)
    os.chmod(file_path, NumberConstant.DIR_AUTHORITY)


def files_chmod(file_path: str) -> None:
    """
    change the rights of the files
    :param file_path: file path
    :return: None
    """
    try:
        if os.path.exists(file_path):
            _do_change_file_mod(file_path)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        error(MsvpCommonConst.FILE_NAME, str(err))
    finally:
        pass


def create_csv(csv_file: str, headers: list, data: list, save_old_file: bool = True, use_dict=False) -> str:
    """
    create csv table
    :param csv_file: csv file path
    :param headers: data header for csv file
    :param data: data for csv file
    :param save_old_file: save old file or not
    :param use_dict: csv use DictWriter or not
    :return: result for creating csv file
    """
    if not bak_and_make_dir(csv_file, save_old_file):
        return json.dumps({'status': NumberConstant.ERROR, 'info': str('bak or mkdir json dir failed'), 'data': ''})
    try:
        if use_dict:
            create_normal_writer(csv_file, headers, data)
        else:
            create_csv_writer(csv_file, headers, data)
        return json.dumps({'status': NumberConstant.SUCCESS, 'info': '', 'data': csv_file})
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        return json.dumps({'status': NumberConstant.ERROR, 'info': str(err), 'data': ''})


def create_csv_writer(csv_file: str, headers: list, data: list):
    with FdOpen(csv_file, newline='') as _csv_file:
        writer = csv.writer(_csv_file)
        if headers:
            writer.writerow(headers)
        slice_count = len(data) // NumberConstant.DATA_NUM
        for index in range(slice_count):
            writer.writerows(data[index * NumberConstant.DATA_NUM : (index + 1) * NumberConstant.DATA_NUM])
        writer.writerows(data[slice_count * NumberConstant.DATA_NUM :])


def create_normal_writer(csv_file: str, headers: list, data: list):
    with FdOpen(csv_file) as _csv_file:
        if not headers:
            return
        _csv_file.write(','.join(headers))
        slice_count = len(data) // NumberConstant.DATA_NUM
        for index in range(slice_count):
            _csv_file.writelines(data[index * NumberConstant.DATA_NUM : (index + 1) * NumberConstant.DATA_NUM])
        _csv_file.writelines(data[slice_count * NumberConstant.DATA_NUM :])


def create_json(json_file: str, headers: list, data: list, save_old_file: bool = True) -> str:
    """
    create json file
    :param json_file: json file name
    :param headers: data header for json file
    :param data: data for json file
    :param save_old_file: save old file or not
    :return: result of creating json file
    """
    json_result = []
    for each in data:
        json_result.append(OrderedDict(list(zip(headers, each))))
    if not bak_and_make_dir(json_file, save_old_file):
        return json.dumps({'status': NumberConstant.ERROR, 'info': str('bak or mkdir csv dir failed'), 'data': ''})
    try:
        with FdOpen(json_file) as _json_file:
            json.dump(json_result, _json_file)
        return json.dumps({'status': NumberConstant.SUCCESS, 'info': '', 'data': json_file})
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        return json.dumps({'status': NumberConstant.ERROR, 'info': str(err), 'data': ''})


def create_json_for_dict(json_file: str, dict_result: dict) -> str:
    """
    create json file for dict object
    :param json_file: json file name
    :param dict_result: dict
    :return: result of creating json file
    """
    if not bak_and_make_dir(json_file, False):
        return json.dumps({'status': NumberConstant.ERROR, 'info': str('bak or mkdir csv dir failed'), 'data': ''})
    try:
        with FdOpen(json_file) as _json_file:
            json.dump(dict_result, _json_file)
        return json.dumps({'status': NumberConstant.SUCCESS, 'info': '', 'data': json_file})
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        return json.dumps({'status': NumberConstant.ERROR, 'info': str(err), 'data': ''})


def bak_and_make_dir(file: str, save_old_file: bool = True) -> bool:
    json_file_back = file + ".bak"
    try:
        if os.path.exists(file):
            if save_old_file:
                shutil.move(file, json_file_back)
            else:
                os.remove(file)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        return False

    try:
        if not os.path.exists(os.path.dirname(file)):
            os.makedirs(os.path.dirname(file), Constant.FOLDER_MASK)
        return True
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        return False


def path_check(path: str) -> str:
    """
    check existence of path
    :param path: file path
    :return:
    """
    if os.path.exists(path):
        return path
    return ""


def add_aicore_units(header: list):
    """
    add unit for ai core report headers and modify total_time to aicore_time
    :param header:
    :return: headers
    """
    for index, item in enumerate(header):
        if item in {"total_time", "aic_total_time"}:
            item = "aicore_time"
        if item == "aiv_total_time":
            item = "aiv_time"
        if "time" in item:
            item += "(us)"
        if StrConstant.BANDWIDTH in item and "(GB/s)" not in item:
            item += "(GB/s)"
        header[index] = item


def check_dir_writable(path: str, create_dir: bool = False) -> None:
    """
    check path is dir and writable
    :param path: file path
    :param create_dir: create directory or not
    :return: None
    """
    if not os.path.exists(path) and create_dir:
        try:
            os.makedirs(path, Constant.FOLDER_MASK)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            ReturnCodeCheck.print_and_return_status(
                json.dumps({'status': NumberConstant.ERROR, 'info': "Failed to create path '%s'. %s" % (path, err)})
            )
        finally:
            pass
    check_path_valid(path, False)
    if not os.access(path, os.W_OK):
        ReturnCodeCheck.print_and_return_status(
            json.dumps(
                {
                    'status': NumberConstant.ERROR,
                    'info': "The path '%s' does not have permission to write. Please "
                    'check that the path is writeable.' % path,
                }
            )
        )


def is_valid_original_data(file_name: str, project_path: str, is_conf: bool = False) -> bool:
    """
    is original data
    :param file_name: file name
    :param project_path: project path
    :param is_conf: is config file or not
    :return: result of checking original data
    """
    file_parent_path = project_path if is_conf else PathManager.get_data_dir(project_path)
    if (
        file_name.endswith(Constant.COMPLETE_TAG)
        or file_name.endswith(Constant.DONE_TAG)
        or file_name.endswith(Constant.ZIP_TAG)
    ):
        return False
    if os.path.exists(os.path.join(file_parent_path, file_name + Constant.COMPLETE_TAG)):
        return False
    return True


def float_calculate(input_list: list, operator: str = '+') -> str:
    """
    float data calculate
    :param input_list: data for calculating
    :param operator: operator
    :return: result after calculated
    """
    operator_dict = {
        StrConstant.OPERATOR_PLUS: add,
        StrConstant.OPERATOR_MINUS: sub,
        StrConstant.OPERATOR_MULTIPLY: mul,
        StrConstant.OPERATOR_DIVISOR: truediv,
    }
    if operator not in operator_dict or not input_list or None in input_list:
        return str(0)
    if operator == StrConstant.OPERATOR_DIVISOR and str(input_list[1]) == "0":
        return str(0)
    new_input_list = []
    for i in input_list:
        new_input_list.append(Decimal(str(i)))
    input_list = new_input_list
    try:
        result = reduce(operator_dict.get(operator), input_list)
        result = result.quantize(Decimal('0.000'))
        return str(result)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError, ArithmeticError):
        return str(0)


def is_number(float_num: any) -> bool:
    """
    check whether s is number or not
    :param float_num: number to check
    :return: is number or not
    """
    try:
        float(float_num)
        return True
    except (ValueError, TypeError):
        return False
    finally:
        pass


def is_nonzero_number(float_num: any) -> bool:
    """
    check whether s is zero number or not
    :param float_num: number to check
    :return: is zero number or not
    """
    return is_number(float_num) and not NumberConstant.is_zero(float(float_num))


def clear_project_dirs(project_dir: str) -> None:
    """
    remove sqlite data and complete data
    project_dir : result path
    """
    if os.path.exists(PathManager.get_sql_dir(project_dir)):
        for file_name in os.listdir(PathManager.get_sql_dir(project_dir)):
            os.remove(os.path.join(PathManager.get_sql_dir(project_dir), file_name))
    for file_name in os.listdir(PathManager.get_data_dir(project_dir)):
        if file_name.endswith(Constant.COMPLETE_TAG):
            os.remove(os.path.join(PathManager.get_data_dir(project_dir), file_name))


def format_high_precision_for_csv(data: str) -> str:
    return data + '\t'
