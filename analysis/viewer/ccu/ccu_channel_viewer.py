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

from abc import ABC

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import format_high_precision_for_csv
from msmodel.hardware.ccu_channel_model import CCUViewerChannelModel
from viewer.interface.base_viewer import BaseViewer


class CCUChannelViewer(BaseViewer, ABC):
    """
    class for get ccu channel data
    """

    PACKAGE_SIZE = 4 * Constant.KILOBYTE

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.model_list = {"ccu_channel": CCUViewerChannelModel}

    def get_summary_data(self: any) -> tuple:
        """
        to get summary data
        """
        summary_data = self.get_data_from_db()
        formatted_data = self.format_channel_summary_data(summary_data)
        return self.configs.get(StrConstant.CONFIG_HEADERS), formatted_data, len(formatted_data)

    def format_channel_summary_data(self, summary_data: list) -> list:
        return [
            (
                data.channel_id,
                format_high_precision_for_csv(InfoConfReader().trans_syscnt_into_local_time(data.timestamp)),
                round(self._trans_delay_to_bandwidth(data.min_bw), NumberConstant.ROUND_THREE_DECIMAL),
                round(self._trans_delay_to_bandwidth(data.max_bw), NumberConstant.ROUND_THREE_DECIMAL),
                round(self._trans_delay_to_bandwidth(data.avg_bw), NumberConstant.ROUND_THREE_DECIMAL),
            )
            for data in summary_data
        ]

    def _trans_delay_to_bandwidth(self, syscnt: int) -> float:
        delay = InfoConfReader().duration_from_syscnt(syscnt)
        if delay == 0:
            return 0
        return self.PACKAGE_SIZE / delay * Constant.BYTE_US_TO_MB_S
