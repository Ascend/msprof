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

import logging

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from msmodel.api.api_data_viewer_model import ApiDataViewModel


class ApiStatisticViewer:
    """
    Viewer for showing API(acl/hccl/model/node/runtime) data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._api_model = ApiDataViewModel(params)
        self._api_duration = {}

    @staticmethod
    def _api_statistic_reformat(data_dict: dict) -> list:
        reformat_result = []

        for api_data, duration in data_dict.items():
            api_name = api_data[0]
            level = api_data[1]
            sum_value = round(sum(duration), NumberConstant.ROUND_THREE_DECIMAL)
            count = len(duration)
            average_value = round(sum_value / count, NumberConstant.ROUND_THREE_DECIMAL)
            max_value = round(max(duration), NumberConstant.ROUND_THREE_DECIMAL)
            min_value = round(min(duration), NumberConstant.ROUND_THREE_DECIMAL)
            deviations = [round((x - average_value) ** 2, NumberConstant.ROUND_THREE_DECIMAL) for x in duration]
            variance = round(sum(deviations) / count, NumberConstant.ROUND_THREE_DECIMAL)
            reformat_result.append((level, api_name, sum_value, count, average_value, min_value, max_value, variance))
        return reformat_result

    def get_api_summary_data(self: any) -> list:
        """
        get summary data from api_event.db
        :return: summary data
        """
        if not self._api_model.init():
            logging.error("api data maybe parse failed, please check the data parsing log.")
            return MsvpConstant.EMPTY_LIST
        api_statistic_data = self._api_model.get_api_statistic_data()
        for api_name, duration, level in api_statistic_data:
            duration = InfoConfReader().get_host_duration(duration, NumberConstant.MICRO_SECOND)
            if (api_name, level) in self._api_duration:
                self._api_duration[(api_name, level)].append(duration)
            else:
                self._api_duration[(api_name, level)] = [duration]
        api_statistic_data = self._api_statistic_reformat(self._api_duration)

        if api_statistic_data:
            return api_statistic_data
        return MsvpConstant.EMPTY_LIST

    def get_api_statistic_data(self) -> tuple:
        """
        get api statistic data
        """
        data = self.get_api_summary_data()
        return self._configs.get(StrConstant.CONFIG_HEADERS), data, len(data)
