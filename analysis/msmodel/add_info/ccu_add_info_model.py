# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.ccu_model import insert_ccu_data_to_db
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.ccu.ccu_add_info_dto import OriginGroupInfoDto
from profiling_bean.db_dto.ccu.ccu_add_info_dto import OriginTaskInfoDto
from profiling_bean.db_dto.ccu.ccu_add_info_dto import OriginWaitSignalInfoDto


class CCUTaskInfoModel(ParserModel):
    """
    Model for CCU Task Info Parser
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CCU_ADD_INFO, [DBNameConstant.TABLE_CCU_TASK_INFO])

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_CCU_TASK_INFO) -> bool:
        """
        insert data to table
        :param data_list: CCU Task Info data
        :param table_name: table name
        :return:
        """
        return insert_ccu_data_to_db(self.conn, table_name, data_list)


class CCUWaitSignalInfoModel(ParserModel):
    """
    Model for CCU Wait Signal Info Parser
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CCU_ADD_INFO, [DBNameConstant.TABLE_CCU_WAIT_SIGNAL_INFO])

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_CCU_WAIT_SIGNAL_INFO) -> bool:
        """
        insert data to table
        :param data_list: ccu wait signal info data
        :param table_name: table name
        :return:
        """
        return insert_ccu_data_to_db(self.conn, table_name, data_list)


class CCUGroupInfoModel(ParserModel):
    """
    Model for CCU Group Info Parser
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CCU_ADD_INFO, [DBNameConstant.TABLE_CCU_GROUP_INFO])

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_CCU_GROUP_INFO) -> bool:
        """
        insert data to table
        :param data_list: ccu group info data
        :param table_name: table name
        :return:
        """
        return insert_ccu_data_to_db(self.conn, table_name, data_list)


class CCUViewerTaskInfoModel(ViewModel):
    """
    ccu viewer task info model class
    """

    def __init__(self: any, result_dir: str, *args, **kwargs) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CCU_ADD_INFO, [DBNameConstant.TABLE_CCU_TASK_INFO])

    def get_timeline_data(self: any) -> list:
        """
        get ccu task info data
        :return: list
        """
        sql = (
            "SELECT version, work_flow_mode, item_id, group_name, rank_id, rank_size, stream_id, task_id,"
            "die_id, mission_id, instr_id FROM {}".format(DBNameConstant.TABLE_CCU_TASK_INFO)
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=OriginTaskInfoDto)


class CCUViewerWaitSignalInfoModel(ViewModel):
    """
    ccu viewer wait signal info model class
    """

    def __init__(self: any, result_dir: str, *args, **kwargs) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CCU_ADD_INFO, [DBNameConstant.TABLE_CCU_WAIT_SIGNAL_INFO])

    def get_timeline_data(self: any) -> list:
        """
        get viewer wait signal info data
        :return: list
        """
        sql = (
            "SELECT version, item_id, group_name, rank_id, work_flow_mode, rank_size, stream_id, task_id,"
            "die_id, instr_id, mission_id,"
            "cke_id, mask, channel_id, remote_rank_id FROM {}".format(DBNameConstant.TABLE_CCU_WAIT_SIGNAL_INFO)
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=OriginWaitSignalInfoDto)


class CCUViewerGroupInfoModel(ViewModel):
    """
    ccu viewer group info model class
    """

    def __init__(self: any, result_dir: str, *args, **kwargs) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CCU_ADD_INFO, [DBNameConstant.TABLE_CCU_GROUP_INFO])

    def get_timeline_data(self: any) -> list:
        """
        get viewer group info data
        :return: list
        """
        sql = (
            "SELECT version, item_id, group_name, rank_id, work_flow_mode, rank_size, stream_id, task_id,"
            "die_id, instr_id, mission_id, reduce_op_type, input_data_type, output_data_type, data_size,"
            "channel_id, remote_rank_id FROM {}".format(DBNameConstant.TABLE_CCU_GROUP_INFO)
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=OriginGroupInfoDto)
