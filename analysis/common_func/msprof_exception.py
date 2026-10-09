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

from common_func.common import error


class ProfException(Exception):
    """
    The class for Profiling Exception
    """

    # error code for user:success
    PROF_NONE_ERROR = 0
    # error code for user: error
    PROF_INVALID_PARAM_ERROR = 1
    PROF_INVALID_PATH_ERROR = 2
    PROF_INVALID_CONNECT_ERROR = 3
    PROF_INVALID_EXECUTE_CMD_ERROR = 4
    PROF_INVALID_CONFIG_ERROR = 5
    PROF_INVALID_DISK_SHORT_ERROR = 6
    PROF_INVALID_JSON_ERROR = 7
    PROF_INVALID_DATA_ERROR = 8
    PROF_INVALID_STEP_TRACE_ERROR = 9
    PROF_SYSTEM_EXIT = 10
    PROF_CLUSTER_DIR_ERROR = 11
    PROF_CLUSTER_INVALID_DB = 12
    PROF_DB_RECORD_EXCEED_LIMIT = 13

    def __init__(self: any, code: int, message: str = '', callback: any = error) -> None:
        super().__init__(code, message)
        self.code = code
        self.message = message
        self.callback = callback

    def __str__(self):
        return self.message
