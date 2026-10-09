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
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel


class QosModel(ParserModel):
    """
    QoS model class
    """

    def flush(self: any, data_list: list) -> None:
        """
        flush qos data to db
        :param data_list: qos data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_QOS_BW, data_list)


class QosViewModel(ViewModel):
    """
    QoS view model class
    """

    def get_timeline_data(self: any) -> list:
        """
        get qos bandwidth data
        :return: list
        """
        sql = "select timestamp, die_id, bw1, bw2, bw3, bw4, bw5, bw6, bw7, bw8, bw9, bw10 from {};".format(
            DBNameConstant.TABLE_QOS_BW
        )
        return DBManager.fetch_all_data(self.cur, sql)
