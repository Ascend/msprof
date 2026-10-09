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


from profiling_bean.struct_info.struct_decoder import StructDecoder


class NpuModuleMemDataBean(StructDecoder):
    """
    Npu module mem data bean for the data parsing by npu module mem parser
    """

    def __init__(self: any, *args) -> None:
        self._module_id, _, self._cpu_cycle_count, self._total_size = args[0][:4]

    @property
    def module_id(self: any) -> int:
        """
        :return: module_id
        """
        return self._module_id

    @property
    def cpu_cycle_count(self: any) -> int:
        """
        :return: cpu_cycle_count
        """
        return self._cpu_cycle_count

    @property
    def total_size(self: any) -> int:
        """
        :return: total_size
        """
        return self._total_size
