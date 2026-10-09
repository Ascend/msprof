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

import os
import stat

from profiling_bean.db_dto.step_trace_dto import IterationRange

INFO_JSON = {
    'DeviceInfo': [
        {
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
    ]
}
ITER_RANGE = IterationRange(0, 1, 1)
CONFIG = {'result_dir': 'test', 'device_id': '0', 'iter_id': ITER_RANGE, 'job_id': 'job_default', 'model_id': -1}
UT_CONFIG_FILE_PATH = os.path.join(os.path.dirname(__file__), "..", "msconfig")
WRITE_MODES = stat.S_IWUSR | stat.S_IRUSR | stat.S_IRGRP
WRITE_FLAGS = os.O_WRONLY | os.O_CREAT | os.O_TRUNC


def clear_dt_project(path):
    if os.path.exists(path):
        for root, dirs, files in os.walk(path, topdown=False):
            for name in files:
                os.remove(os.path.join(root, name))
            for name in dirs:
                os.rmdir(os.path.join(root, name))
        os.rmdir(path)
