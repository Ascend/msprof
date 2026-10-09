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

from common_func.db_name_constant import DBNameConstant
from msmodel.interface.base_model import BaseModel


class L2CacheCalculatorModel(BaseModel):
    """
    db operator for l2 cache calculator
    """

    RAW_DATA_EVENTS_INDEX = -1

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_L2CACHE, [DBNameConstant.TABLE_L2CACHE_SUMMARY])

    @staticmethod
    def split_events_data(l2_cache_ps_data: list) -> list:
        """
        split events str in l2_cache_ps_data like "a,b,c,..." to [a, b, c]
        """
        res_l2_cache_ps_data = []
        if not l2_cache_ps_data or len(l2_cache_ps_data[0]) <= 1:
            return []

        l2_cache_ps_data_no_events = []
        for data in l2_cache_ps_data:
            l2_cache_ps_data_no_events.append(list(data[: L2CacheCalculatorModel.RAW_DATA_EVENTS_INDEX]))

        l2_cache_ps_data_events = []
        for data in l2_cache_ps_data:
            l2_cache_ps_data_events.append(list(data[L2CacheCalculatorModel.RAW_DATA_EVENTS_INDEX].split(",")))

        for no_event, event in zip(l2_cache_ps_data_no_events, l2_cache_ps_data_events):
            no_event.extend(event)
            res_l2_cache_ps_data.append(no_event)
        return res_l2_cache_ps_data

    def flush(self: any, data_list: list) -> None:
        """
        insert data into database
        """
        self.insert_data_to_db(self.table_list[0], data_list)
