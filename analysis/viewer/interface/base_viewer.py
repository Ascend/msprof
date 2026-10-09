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

from abc import abstractmethod

from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant


class BaseViewer:
    """
    class for get data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        # subclass should define it's list content
        self.model_list = {}
        self.configs = configs
        self.params = params

    def get_timeline_data(self: any) -> str:
        """
        get model list timeline data
        @return:timeline trace data
        """
        timeline_data = self.get_data_from_db()
        result = self.get_trace_timeline(timeline_data)
        return result

    def get_summary_data(self: any) -> tuple:
        """
        to get summary data
        """
        summary_data = self.get_data_from_db()
        return self.configs.get(StrConstant.CONFIG_HEADERS), summary_data, len(summary_data)

    def get_model_instance(self: any) -> any:
        """
        get model instance from list
        """
        model_class = self.model_list.get(self.params.get(StrConstant.PARAM_DATA_TYPE))
        return model_class(
            self.params.get(StrConstant.PARAM_RESULT_DIR),
            self.configs.get(StrConstant.CONFIG_DB),
            self.configs.get(StrConstant.CONFIG_TABLE),
        )

    def get_data_from_db(self: any) -> list:
        """
        get data from msmodel
        :return: []
        """
        model = self.get_model_instance()
        if not model or not model.check_db():
            return []

        if self.params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            data = model.get_timeline_data()
        else:
            data = model.get_summary_data()
        model.finalize()
        return data

    @abstractmethod
    def get_trace_timeline(self: any, data_list: list) -> list:
        """
        base method to get timeline data
        """
