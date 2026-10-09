#!/usr/bin/python3
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
import os

from common_func.path_manager import PathManager
from common_func.singleton import singleton
from common_func.db_name_constant import DBNameConstant
from msmodel.compact_info.stream_expand_spec_model import StreamExpandSpecViewModel


@singleton
class StreamExpandSpecReader:
    """
    class used to read stream expand spec data from stream_expand_spec.db
    """

    def __init__(self: any) -> None:
        self._stream_expand_spec = 0
        self._is_load_stream_expand = False

    @property
    def is_stream_expand(self: any) -> bool:
        return self._stream_expand_spec == 1

    def load_stream_expand_spec(self: any, result_path: str) -> None:
        """
        load stream expand spec
        """
        if (
            not os.path.exists(PathManager.get_db_path(result_path, DBNameConstant.DB_STREAM_EXPAND_SPEC))
            or self._is_load_stream_expand
        ):
            return

        with StreamExpandSpecViewModel(result_path) as stream_expand_spec_model:
            data = stream_expand_spec_model.get_stream_expand_spec_data()
            if not data:
                return
            self._stream_expand_spec = data[0][0]
            self._is_load_stream_expand = True
