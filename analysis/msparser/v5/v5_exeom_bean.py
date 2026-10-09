# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

from msparser.add_info.add_info_bean import AddInfoBean


class V5ExeomBean(AddInfoBean):
    """
    V5 Exeom bean data for the data parsing.
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        filed = args[0]
        self._model_id = filed[6]
        self._model_name = filed[7]

    @property
    def graph_id(self: any) -> int:
        """
        v5 host data model_id
        :return: model_id
        """
        return self._model_id

    @property
    def model_name(self: any) -> str:
        """
        v5 host data model_name
        :return: model_name
        """
        return str(self._model_name)
