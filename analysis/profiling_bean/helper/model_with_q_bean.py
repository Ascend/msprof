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


class ModelWithQBean(StructDecoder):
    """
    step trace
    """

    def __init__(self: any, *args: any) -> None:
        data = args[0]
        self._timestamp = data[3]
        self._index_id = data[4]
        self._model_id = data[5]
        self._tag_id = data[6]
        self._event_id = data[8]

    @property
    def index_id(self: any) -> int:
        """
        get index id
        :return: index_id
        """
        return self._index_id

    @property
    def model_id(self: any) -> int:
        """
        get model id
        :return: model id
        """
        return self._model_id

    @property
    def timestamp(self: any) -> int:
        """
        get timestamp
        :return: timestamp
        """
        return self._timestamp

    @property
    def event_id(self: any) -> int:
        """
        get event id
        :return: event_id
        """
        return self._event_id

    @property
    def tag_id(self: any) -> int:
        """
        get tag id
        :return: tag id
        """
        return self._tag_id
