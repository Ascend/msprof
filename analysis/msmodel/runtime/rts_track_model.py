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
from msconfig.config_manager import ConfigManager
from msmodel.interface.base_model import BaseModel


class RtsModel(BaseModel):
    """
    db operator for acl parser
    """

    TABLES_PATH = ConfigManager.TABLES

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return "RtsModel"

    def create_rts_db(self: any, data: list) -> None:
        """
        create rts_track.db
        :param data:
        :return:
        """
        self.init()
        self.check_db()
        self.create_table()
        self.insert_data_to_db(DBNameConstant.TABLE_TASK_TRACK, data)
        self.finalize()
