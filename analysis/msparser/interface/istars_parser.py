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

from abc import abstractmethod


class IStarsParser:
    """
    parser interface
    """

    MAX_DATA_LEN = 1000000000

    def __init__(self: any) -> None:
        self._model = None
        self._decoder = None
        self._data_list = []

    def handle(self: any, _: any, data: bytes) -> None:
        """
        Process a single piece of binary data, and the subclass can also overwrite it.
        :param _: sqe_type some parser need it
        :param data: binary data
        :return:
        """
        if len(self._data_list) >= self.MAX_DATA_LEN:
            self.flush()
        self._data_list.append(self._decoder.decode(data))

    @abstractmethod
    def preprocess_data(self: any) -> None:
        """
        Before saving to the database, subclasses can implement this method.
        Do another layer of preprocessing on the data in the buffer
        :return:result data list
        """

    def flush(self: any) -> None:
        """
        flush all buffer data to db
        :return: NA
        """
        if not self._data_list:
            return
        if self._model.init():
            self.preprocess_data()
            self._model.flush(self._data_list)
            self._model.finalize()
            self._data_list.clear()
