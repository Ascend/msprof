#!/usr/bin/env python
# coding=utf-8
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
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import json
import os

import constant.constant
from common_func.info_conf_reader import InfoConfReader
from constant.constant import UT_CONFIG_FILE_PATH


class CPUInfo:
    DEFAULT_CPU_INFO = {
        "Id": 0,
        "Name": "GenuineIntel",
        "Frequency": "100.000000",
        "Logical_CPU_Count": 1,
        "Type": "Intel(R) Xeon(R) Platinum 8476C",
    }

    def __init__(self, **kwargs: any):
        for key, value in self.DEFAULT_CPU_INFO.items():
            setattr(self, key, value)
        self.__dict__.update(kwargs)

    @property
    def cpu_info(self):
        return self.__dict__


class DeviceInfo:
    DEFAULT_DEVICE_INFO = {
        'id': 0,
        'env_type': 3,
        'ctrl_cpu_id': 'ARMv8_Cortex_A55',
        'ctrl_cpu_core_num': 1,
        'ctrl_cpu_endian_little': 1,
        'ts_cpu_core_num': 1,
        'ai_cpu_core_num': 7,
        'ai_core_num': 8,
        'ai_cpu_core_id': 1,
        'ai_core_id': 0,
        'aicpu_occupy_bitmap': 254,
        'ctrl_cpu': '0',
        'ai_cpu': '1, 2, 3, 4, 5, 6, 7',
        'aiv_num': 0,
        'hwts_frequency': '38.4',
        'aic_frequency': '1150',
        'aiv_frequency': '1000',
    }

    def __init__(self, **kwargs: any):
        for key, value in self.DEFAULT_DEVICE_INFO.items():
            setattr(self, key, value)
        self.__dict__.update(kwargs)

    @property
    def device_info(self):
        return self.__dict__


class InfoJson:
    DEFAULT_INFO_JSON = {
        "jobInfo": "NA",
        "devices": "0",
        "CPU": [CPUInfo().cpu_info],
        "DeviceInfo": [DeviceInfo().device_info],
        "platform_version": "0",
        "pid": "1000",
    }

    def __init__(self, **kwargs):
        for key, value in self.DEFAULT_INFO_JSON.items():
            setattr(self, key, value)
        self.__dict__.update(kwargs)

    def to_dict(self):
        return json.loads(json.dumps(self, default=lambda json_obj: json_obj.__dict__))

    def apply(self):
        """Load sample/start from the UT config dir, then set info.json in memory only."""
        InfoConfReader().load_info(UT_CONFIG_FILE_PATH)
        InfoConfReader()._info_json = self.to_dict()


class InfoJsonReaderManager:
    def __init__(self, info_json=InfoJson()):
        self._info_json = info_json

    @staticmethod
    def _get_info_json_path():
        return os.path.join(UT_CONFIG_FILE_PATH, "info.json")

    @staticmethod
    def _reload_info_json():
        InfoConfReader().load_info(UT_CONFIG_FILE_PATH)

    def process(self):
        self._update_info_json()
        self._reload_info_json()

    def _update_info_json(self):
        json_data = json.dumps(self._info_json, default=lambda json_obj: json_obj.__dict__)
        with os.fdopen(
            os.open(self._get_info_json_path(), constant.constant.WRITE_FLAGS, constant.constant.WRITE_MODES), 'w'
        ) as _json_file:
            json.dump(json.loads(json_data), _json_file)
