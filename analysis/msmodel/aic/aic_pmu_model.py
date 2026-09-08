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
from common_func.path_manager import PathManager
from mscalculate.aic.aic_utils import AicPmuUtils
from msmodel.interface.parser_model import ParserModel
from viewer.calculate_rts_data import create_metric_table
from viewer.calculate_rts_data import get_metrics_from_sample_config


class AicPmuModel(ParserModel):
    """
    ffts pmu model.
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_METRICS_SUMMARY, DBNameConstant.TABLE_METRIC_SUMMARY)

    def create_table(self: any) -> None:
        """
        create aic and aiv table by sample.json
        :return:
        """
        self.clear()
        aic_profiling_events = get_metrics_from_sample_config(self.result_dir)
        column_list = AicPmuUtils.remove_unused_column(aic_profiling_events)
        create_metric_table(self.conn, column_list, DBNameConstant.TABLE_METRIC_SUMMARY)

    def flush(self: any, data_list: list) -> None:
        """
        insert data into database
        :param data_list: ffts pmu data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_METRIC_SUMMARY, data_list)

    def clear(self: any) -> None:
        """
        clear ai core metric table
        :return: None
        """
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_METRICS_SUMMARY)
        if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_METRIC_SUMMARY):
            DBManager.drop_table(self.conn, DBNameConstant.TABLE_METRIC_SUMMARY)


class V5AicPmuModel(AicPmuModel):
    """
    v5 pmu model.
    """

    def create_table(self: any) -> None:
        """
        create aic and aiv table by sample.json
        :return:
        """
        self.clear()
        aic_profiling_events = get_metrics_from_sample_config(self.result_dir)
        column_list = AicPmuUtils.remove_unused_column(aic_profiling_events)
        create_metric_table(self.conn, column_list, DBNameConstant.TABLE_METRIC_SUMMARY)


class FftsV1PmuModel(AicPmuModel):
    """
    7/8/11(V1.1.x) ffts 非mix场景的MetricSummary模型。
    在AIC表基础上于表尾追加end_time列，保存每算子context PMU的wall-clock结束时间，
    供C++ unified task-pmu读取作为算子的时间（与chip5/chip6 python侧写end_time语义一致）。
    """

    def create_table(self: any) -> None:
        """
        create aic metric table and append end_time column at the tail.
        :return:
        """
        super().create_table()
        sql = "ALTER TABLE {0} ADD COLUMN end_time INT".format(DBNameConstant.TABLE_METRIC_SUMMARY)
        DBManager.execute_sql(self.conn, sql)
