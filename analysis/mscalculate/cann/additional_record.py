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


class AdditionalRecord:
    """
    This class is as an abstraction of additional information
    """

    _ID = 0

    def __init__(self, add_dto: any = None, timestamp: float = -1, struct_type: str = ""):
        self.dto = add_dto
        self.timestamp = timestamp
        self.id = self._ID
        self.struct_type = struct_type
        AdditionalRecord._ID += 1

    def __lt__(self, other):
        return self.timestamp > other.timestamp

    def __hash__(self):
        return hash(self.id)

    def __eq__(self, other):
        return self.id == self.id

    def __str__(self):
        return self.struct_type + "-" + str(self.id)
