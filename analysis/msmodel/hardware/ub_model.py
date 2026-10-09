#!/usr/bin/python3
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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.ub_dto import UBDto


class UBModel(ParserModel):
    """
    UB model class
    """

    def flush(self: any, data_list: list) -> None:
        """
        flush UB data to db
        :param data_list: UB data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_UB_BW, data_list)


class UBViewModel(ViewModel):
    """
    UB view model class
    """

    def get_timeline_data(self: any) -> list:
        """
        get UB bandwidth data
        :return: list
        """
        sql = self.get_sql()
        return DBManager.fetch_all_data(self.cur, sql, dto_class=UBDto)

    def get_summary_data(self: any) -> list:
        """
        get UB bandwidth data
        :return: list
        """
        sql = self.get_sql()
        return DBManager.fetch_all_data(self.cur, sql, dto_class=UBDto)

    def get_sql(self):
        return (
            "select device_id, port_id, time_stamp, udma_rx_bind, udma_tx_bind, rx_port_band_width, "
            "rx_packet_rate, rx_bytes, rx_packets, rx_errors, rx_dropped, tx_port_band_width, tx_packet_rate, "
            "tx_bytes, tx_packets, tx_errors, tx_dropped from {};".format(DBNameConstant.TABLE_UB_BW)
        )
