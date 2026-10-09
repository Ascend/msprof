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

from abc import ABCMeta
from abc import abstractmethod


class StepTraceTagHandler(metaclass=ABCMeta):
    """
    get model_id, index_id, FP, BP, reduce from step trace
    """

    @abstractmethod
    def receive_record(self: any, record: dict) -> None:
        """
        receive record of step trace
        :param record: contain model_id, tag_id, timestamp
        :return: void
        """

    @abstractmethod
    def get_data(self: any) -> dict:
        """
        return data of this handler
        :return: dict
        """
