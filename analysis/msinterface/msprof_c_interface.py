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

import importlib
import logging
import os
import sys

from common_func.config_mgr import ConfigMgr
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_multi_process import run_in_subprocess
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from common_func.cpp_enable_scene import DeviceParseScene, ExportDBScene
from common_func.file_manager import check_so_valid

SO_DIR = os.path.join(os.path.dirname(__file__), "..", "lib64")


def _dump_cann_trace(project_path: str):
    sys.path.append(os.path.realpath(SO_DIR))
    logging.info("Data will be parsed by msprof_analysis.so!")
    msprof_analysis_module = importlib.import_module("msprof_analysis")
    msprof_analysis_module.parser.dump_cann_trace(project_path)


def _dump_device_data(device_path: str):
    sys.path.append(os.path.realpath(SO_DIR))
    logging.info("Device Data will be parsed by msprof_analysis.so!")
    msprof_analysis_module = importlib.import_module("msprof_analysis")
    msprof_analysis_module.parser.dump_device_data(os.path.dirname(device_path))


def _export_unified_db(project_path: str):
    sys.path.append(os.path.realpath(SO_DIR))
    logging.info("Data will be parsed by msprof_analysis.so!")
    msprof_analysis_module = importlib.import_module("msprof_analysis")
    msprof_analysis_module.parser.export_unified_db(project_path)


def _export_timeline(project_path: str, report_json_path: str):
    sys.path.append(os.path.realpath(SO_DIR))
    logging.info("Data will be export by msprof_analysis.so!")
    msprof_analysis_module = importlib.import_module("msprof_analysis")
    msprof_analysis_module.parser.export_timeline(project_path, report_json_path)


def _export_summary(project_path: str):
    sys.path.append(os.path.realpath(SO_DIR))
    logging.info("Summary will be export by msprof_analysis.so!")
    msprof_analysis_module = importlib.import_module("msprof_analysis")
    msprof_analysis_module.parser.export_summary(project_path)

def _export_platform(platform_uncore_trace: str, output_path: str):
    if not check_so_valid(os.path.join(SO_DIR, "platform_analysis.so")):
        logging.warning("There is no platform_analysis.so available!")
        return
    sys.path.append(os.path.realpath(SO_DIR))
    logging.info("Platform data will be exported by platform_analysis.so")
    platform_analysis_module = importlib.import_module("platform_analysis")
    platform_analysis_module.process_platform_data(platform_uncore_trace, output_path)


def dump_cann_trace(project_path: str):
    """
    调用host c化
    """
    run_in_subprocess(_dump_cann_trace, project_path)


def export_timeline(project_path: str, report_json_path: str):
    """
    调用viewer C化导出
    """
    run_in_subprocess(_export_timeline, project_path, report_json_path)


def export_summary(project_path: str):
    run_in_subprocess(_export_summary, project_path)


def dump_device_data(device_path: str) -> None:
    """
    调用device c化
    """
    if not ChipManager().is_chip_v4():
        logging.info("Do not support parsing by msprof_analysis.so!")
        return
    if ConfigMgr.is_ai_core_sample_based(device_path):
        logging.warning("Device Data in sample-based will not be parsed by msprof_analysis.so!")
        return
    if ConfigMgr.is_custom_pmu_scene(device_path):
        logging.warning("Device Data in custom pmu scene will not be parsed by msprof_analysis.so!")
        return
    all_export_flag = ProfilingScene().is_all_export() and InfoConfReader().is_all_export_version()
    if DeviceParseScene().is_cpp_enable() and all_export_flag:
        run_in_subprocess(_dump_device_data, device_path)
    else:
        logging.warning("Device Data will not be parsed by msprof_analysis.so!")
    return


def export_unified_db(project_path: str):
    """
    调用统一db
    """
    if not ExportDBScene().is_cpp_enable():
        logging.warning("Does not support exporting the msprof.db!")
        return
    run_in_subprocess(_export_unified_db, project_path)

def export_platform(platform_uncore_trace: str, output_path: str):
    run_in_subprocess(_export_platform, platform_uncore_trace, output_path)