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
from common_func.platform.chip_manager import ChipManager
from msmodel.hardware.ccu_channel_model import CCUViewerChannelModel
from msmodel.hardware.ccu_channel_model import CCUViewerChannelV62Model
from profiling_bean.hardware.ccu_profile import CCU_V6_1_PROFILE
from profiling_bean.hardware.ccu_profile import CCU_V6_2_PROFILE
from viewer.interface.base_viewer import BaseViewer


class CCUChannelViewer(BaseViewer, ABC):
    """
    class for get ccu channel data
    """

    PACKAGE_SIZE = 4 * Constant.KILOBYTE

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        chip_id = ChipManager().chip_id
        if chip_id in CCU_V6_1_PROFILE.supported_chip_models:
            self.model_list = {"ccu_channel": CCUViewerChannelModel}
        elif chip_id in CCU_V6_2_PROFILE.supported_chip_models:
            self.model_list = {"ccu_channel_v62": CCUViewerChannelV62Model}

    def get_summary_data(self: any) -> tuple:
        """
        to get summary data
        """
        headers = self.configs.get(StrConstant.CONFIG_HEADERS)
        if self.params.get(StrConstant.PARAM_DATA_TYPE) != "ccu_channel" or "ccu_channel" not in self.model_list:
            return headers, [], 0
        summary_data = self.get_data_from_db()
        formatted_data = self.format_channel_summary_data(summary_data)
        return headers, formatted_data, len(formatted_data)

    def get_v62_summary_data(self: any) -> tuple:
        headers = self.configs.get(StrConstant.CONFIG_HEADERS)
        if (
            self.params.get(StrConstant.PARAM_DATA_TYPE) != "ccu_channel_v62"
            or "ccu_channel_v62" not in self.model_list
        ):
            return headers, [], 0
        summary_data = self.get_data_from_db()
        formatted_data = self.format_channel_v62_summary_data(summary_data)
        return headers, formatted_data, len(formatted_data)

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

    def format_channel_v62_summary_data(self, summary_data: list) -> list:
        return [
            (
                data.channel_id,
                data.die_id,
                format_high_precision_for_csv(InfoConfReader().trans_syscnt_into_local_time(int(data.syscnt))),
                data.avg_delay,
                data.max_delay,
            )
            for data in summary_data
        ]

    def _trans_delay_to_bandwidth(self, syscnt: int) -> float:
        delay = InfoConfReader().duration_from_syscnt(syscnt)
        if delay == 0:
            return 0
        return self.PACKAGE_SIZE / delay * Constant.BYTE_US_TO_MB_S
