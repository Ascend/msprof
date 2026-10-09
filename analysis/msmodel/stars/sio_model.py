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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from msmodel.interface.parser_model import ParserModel


class SioModel(ParserModel):
    """
    sio model
    """

    def flush(self: any, data: list) -> None:
        """
        insert data to database
        时间戳单位：ns
        带宽单位：MB/s
        """
        self.insert_data_to_db(DBNameConstant.TABLE_SIO, data)

    def get_timeline_data(self: any) -> list:
        """
        get sio timeline data
        :return: list
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_SIO):
            return []
        sql = (
            "select acc_id, req_rx, rsp_rx, snp_rx, dat_rx, req_tx, rsp_tx, snp_tx, dat_tx, "
            "timestamp/{NS_TO_US} as timestamp from {}".format(
                DBNameConstant.TABLE_SIO, NS_TO_US=NumberConstant.NS_TO_US
            )
        )
        return DBManager.fetch_all_data(self.cur, sql)
