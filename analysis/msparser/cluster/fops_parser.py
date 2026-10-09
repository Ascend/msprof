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

# pylint: disable=duplicate-code

import json
import os

from common_func.common import error
from common_func.common import print_info
from common_func.common import warn
from common_func.config_mgr import ConfigMgr
from common_func.data_check_manager import DataCheckManager
from common_func.db_manager import ClassRowType
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import check_path_valid
from common_func.file_manager import FdOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_query_data import QueryArgumentCheck
from common_func.path_manager import PathManager
from profiling_bean.db_dto.cluster_rank_dto import ClusterRankDto
from profiling_bean.db_dto.fops_dto import FopsDto


class FopsParser:
    """
    class used to calculate fops data in cluster
    """

    FILE_NAME = os.path.basename(__file__)
    MAX_TYPE_NUM = 19
    QUERY_FILE_NAME = 'query'
    BMS_TO_GS = 1000.0 / 1000 / 1000 / 1000
    BYT_TO_M = 1.0 / 1000 / 1000

    def __init__(self: any, params: dict) -> None:
        self.collection_path = params.get('collection_path')
        self.data_type = params.get('data_type')
        self.model_id = params.get('model_id')
        self.iter_id = params.get('iteration_id')
        self.rank_id = params.get('npu_id')
        self.sample_config = None

    def get_fops_data(self: any) -> list:
        """
        get data from database
        :return: fops data list
        """
        conn, cur = DBManager().check_connect_db_path(
            PathManager.get_db_path(self.collection_path, DBNameConstant.DB_AICORE_OP_SUMMARY)
        )
        if not all(
            [
                conn,
                cur,
                DBManager.judge_table_exist(cur, DBNameConstant.TABLE_SUMMARY_METRICS),
                DBManager.judge_table_exist(cur, DBNameConstant.TABLE_SUMMARY_GE),
            ]
        ):
            DBManager.destroy_db_connect(conn, cur)
            return []
        sql = (
            "select {0}.cube_fops, {0}.vector_fops, {0}.cube_fops + {0}.vector_fops as total_fops, "
            "{0}.stream_id, {0}.task_id, {1}.op_type, {0}.total_time "
            "from {0} join {1} on {0}.stream_id={1}.stream_id and {0}.task_id={1}.task_id".format(
                DBNameConstant.TABLE_SUMMARY_METRICS, DBNameConstant.TABLE_SUMMARY_GE
            )
        )

        cur.row_factory = ClassRowType.class_row(FopsDto)
        fops_data = DBManager.fetch_all_data(cur, sql)
        DBManager.destroy_db_connect(conn, cur)
        return fops_data

    def calculate(self: any) -> None:
        """
        calculate data and data storage
        :return: None
        """
        if not self.check_id_valid():
            warn(self.FILE_NAME, "Parameter settings are incorrect, please check model_id, id and iteration_id.")
            return
        self._query_data()

    def check_id_valid(self: any) -> bool:
        rank_conn, rank_cur = DBManager.check_connect_db_path(
            PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)
        )
        trace_conn, trace_cur = DBManager.check_connect_db_path(
            PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_STEP_TRACE)
        )
        rank_sql = 'select * from {} where rank_id=?'.format(DBNameConstant.TABLE_CLUSTER_RANK)
        rank_cur.row_factory = ClassRowType.class_row(ClusterRankDto)
        rank_data = DBManager.fetch_all_data(rank_cur, rank_sql, (self.rank_id,))
        if not rank_data:
            DBManager.destroy_db_connect(rank_conn, rank_cur)
            DBManager.destroy_db_connect(trace_conn, trace_cur)
            return False
        trace_sql = 'select * from {} where model_id=? and iteration_id=?'.format(
            DBNameConstant.TABLE_CLUSTER_STEP_TRACE.format(rank_data[0].rank_id)
        )
        trace_data = DBManager.fetch_all_data(trace_cur, trace_sql, (self.model_id, self.iter_id))
        DBManager.destroy_db_connect(rank_conn, rank_cur)
        DBManager.destroy_db_connect(trace_conn, trace_cur)
        if not trace_data:
            return False
        self.collection_path = os.path.join(self.collection_path, rank_data[0].dir_name)
        return True

    def query_fops_data(self: any) -> None:
        """
        query cluster data
        :return: None
        """
        if self.sample_config.get("ai_core_metrics", '') != "ArithmeticUtilization":
            warn(
                self.FILE_NAME,
                "Query fops data failed, --aic_metrics: This parameter can only be set to ArithmeticUtilization. ",
            )
            return
        fops_data = self.get_fops_data()
        if not fops_data:
            error(
                self.FILE_NAME,
                "Query data failed, maybe fops data does not exist or export command has not run "
                "successfully yet, please check your data or run export command",
            )
            return
        json_data = self.calculate_fops_data(fops_data)
        self.storage_data(json_data)

    def storage_data(self: any, json_data: list) -> None:
        """
        save data into file
        :return: None
        """
        print_info(self.FILE_NAME, "Fops data query complete, start to storage data into json file")
        file_name = 'fops_{0}_{1}_{2}.json'.format(self.rank_id, self.model_id, self.iter_id)
        file_path = self.get_cluster_path(file_name)
        if os.path.exists(file_path):
            os.remove(file_path)
        try:
            with FdOpen(file_path) as _file:
                _file.write(json.dumps(json_data))
        except (OSError, SystemError, RuntimeError, TypeError):
            error(
                self.FILE_NAME,
                "Storing data failed, you may not have the permission to write files in the current path.",
            )
        else:
            print_info(self.FILE_NAME, "The data has stored successfully, file path: {}".format(file_path))

    def get_cluster_path(self: any, file_name: str) -> str:
        query_path = os.path.realpath(os.path.join(self.collection_path, '..', '..', self.QUERY_FILE_NAME))
        if not os.path.exists(query_path):
            try:
                os.makedirs(query_path, mode=NumberConstant.DIR_AUTHORITY)
            except OSError:
                error(
                    self.FILE_NAME,
                    "Storing data failed, you may not have the permission to write files in the current path.",
                )
        return os.path.realpath(os.path.join(query_path, file_name))

    def calculate_fops_data(self: any, data_list: list) -> list:
        """
        calculate fops data
        :return: json data list
        """
        op_type_dict = {}
        total_fops = 0
        total_times = 0
        for data in data_list:
            op_type_dict.setdefault(data.op_type, []).append(data.total_fops)
            total_fops += data.total_fops
            total_times += data.total_time
        if not all([total_fops, total_times, op_type_dict]):
            return []
        sorted_data = sorted(zip(op_type_dict.keys(), op_type_dict.values()), key=lambda x: sum(x[1]), reverse=True)
        res_list = [
            {
                'total_fops_info': {
                    "total_fops": round(total_fops * self.BYT_TO_M, NumberConstant.DECIMAL_ACCURACY),
                    "total_time": round(total_times, NumberConstant.DECIMAL_ACCURACY),
                    "total_fops_speed": round(
                        total_fops / total_times * self.BMS_TO_GS, NumberConstant.DECIMAL_ACCURACY
                    ),
                    "total_op_count": len(data_list),
                    "total_fops_avg": round(
                        total_fops / len(data_list) * self.BYT_TO_M, NumberConstant.DECIMAL_ACCURACY
                    ),
                }
            }
        ]
        other_fops_ratio, other_op_count, other_fops = 0, 0, 0
        detail_list = []
        for index, data in enumerate(sorted_data):
            op_type = data[0]
            op_fops = data[1]
            if index < self.MAX_TYPE_NUM:
                detail_list.append(
                    {
                        op_type: {
                            'fops_ratio': float(
                                round(100 * sum(op_fops) / total_fops, NumberConstant.ROUND_TWO_DECIMAL)
                            ),
                            'op_count': len(op_fops),
                            'fops': round(sum(op_fops) * self.BYT_TO_M, NumberConstant.DECIMAL_ACCURACY),
                        }
                    }
                )
            else:
                other_fops_ratio += float(round(100 * sum(op_fops) / total_fops, NumberConstant.ROUND_TWO_DECIMAL))
                other_op_count += len(op_fops)
                other_fops += sum(op_fops)
        if other_op_count:
            detail_list.append(
                {
                    'other': {
                        'fops_ratio': other_fops_ratio,
                        'op_count': other_op_count,
                        'fops': round(other_fops * self.BYT_TO_M, NumberConstant.DECIMAL_ACCURACY),
                    }
                }
            )
        res_list.append({'details': detail_list})
        return res_list

    def process(self: any) -> None:
        """
        entrance for calculating fops data
        :return: None or dict
        """
        QueryArgumentCheck.check_arguments_valid(self.rank_id, self.model_id, self.iter_id)
        if list(filter(lambda x: x is None, [self.rank_id, self.model_id, self.iter_id])):
            warn(self.FILE_NAME, "To query fops data,  id, model-id and iteration-id are required")
            return
        self.calculate()

    def _query_data(self):
        check_path_valid(self.collection_path, False)
        if DataCheckManager.contain_info_json_data(self.collection_path):
            InfoConfReader().load_info(self.collection_path)
            self.sample_config = ConfigMgr.read_sample_config(self.collection_path)
            self.query_fops_data()
        else:
            warn(self.FILE_NAME, 'Invalid parsing dir("%s"), there is no PROF file in this path' % self.collection_path)
