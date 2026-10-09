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
from profiling_bean.helper.model_with_q_bean import ModelWithQBean


class ModelWithQParser:
    """
    class for Model with Q
    """

    def __init__(self: any) -> None:
        self._data = []
        self._table_name = DBNameConstant.TABLE_MODEL_WITH_Q

    @property
    def data(self: any) -> list:
        """
        get data
        :return: data
        """
        return self._data

    @property
    def table_name(self: any) -> str:
        """
        get table_name
        :return: table_name
        """
        return self._table_name

    def read_binary_data(self: any, bean_data: any) -> None:
        """
        read model with q binary data and store them into list
        :param bean_data: binary data
        :return: None
        """
        model_with_q_bean = ModelWithQBean.decode(bean_data)
        if model_with_q_bean:
            self._data.append(
                [
                    model_with_q_bean.index_id,
                    model_with_q_bean.model_id,
                    model_with_q_bean.timestamp,
                    model_with_q_bean.tag_id,
                    model_with_q_bean.event_id,
                ]
            )
