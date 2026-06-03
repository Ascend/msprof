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

import os


class MsvpConstant:
    """
    msvp constant
    """

    CONFIG_PATH = os.path.join(os.path.dirname(__file__), "..", "msconfig")
    # msvp return empty info
    MSVP_EMPTY_DATA = ([], [], 0)
    EMPTY_DICT = {}
    EMPTY_LIST = []
    EMPTY_TUPLE = ()

    @property
    def msvp_empty_data(self: any) -> tuple:
        """
        default empty data struct
        :return:
        """
        return self.MSVP_EMPTY_DATA

    @property
    def empty_dict(self: any) -> dict:
        """
        empty dict
        :return:
        """
        return self.EMPTY_DICT
