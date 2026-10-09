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


class CpuTimeInfo:
    """
    CPU Time Info
    """

    def __init__(self: any, line_info: str) -> None:
        fields = line_info.split()
        self._pid = int(fields[0])
        self._tid = int(fields[3])
        self._utime = int(fields[13])
        self._stime = int(fields[14])
        self._cpu_no = int(fields[38])

    @property
    def pid(self: any) -> int:
        """
        get pid
        :return: pid
        """
        return self._pid

    @property
    def tid(self: any) -> int:
        """
        get tid
        :return: tid
        """
        return self._tid

    @property
    def utime(self: any) -> int:
        """
        get utime
        :return: utime
        """
        return self._utime

    @property
    def stime(self: any) -> int:
        """
        get stime
        :return: stime
        """
        return self._stime

    @property
    def cpu_no(self: any) -> int:
        """
        get cpu_no
        :return: cpu_no
        """
        return self._cpu_no
