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

import logging
from collections import OrderedDict

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.interface.view_model import ViewModel


class StarsSocView:
    """
    Model of viewer for stars soc
    """

    MATA_BW_LEVEL = "Mata Bw Level"
    L2_BUFFER_BW_LEVEL = "L2 Buffer Bw Level"

    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._model = ViewModel(self._project_path, DBNameConstant.DB_STARS_SOC, [DBNameConstant.TABLE_SOC_DATA])
        self._timeline_data = []
        self._pid = InfoConfReader().get_json_pid_data()
        self._tid = InfoConfReader().get_json_tid_data()

    def get_soc_data(self: any) -> list:
        """
        get stars soc data
        :return:
        """
        if not self._model.init() or not self._model.check_table():
            logging.error("Can not get stars soc data, please check the data parsing log.")
            return []
        sql = "select mata_bw_level,l2_buffer_bw_level,sys_time/{NS_TO_US} from {0}".format(
            DBNameConstant.TABLE_SOC_DATA, NS_TO_US=NumberConstant.NS_TO_US
        )

        return self._model.get_sql_data(sql)

    def get_timeline_data(self: any) -> list:
        """
        get timeline data of stars soc
        :return:
        """
        soc_data = self.get_soc_data()
        soc_res = []
        if not soc_data:
            logging.error("Failed to get stars soc data, please check the data parsing log.")
            return []
        self._timeline_data = TraceViewManager.metadata_event(
            [["process_name", self._pid, self._tid, "Stars Soc Info"]]
        )

        for _soc_data in soc_data:
            # _soc_data format: mata_bw_level,l2_buffer_bw_level,sys_time
            soc_res.extend(
                [
                    [
                        self.MATA_BW_LEVEL,
                        InfoConfReader().trans_into_local_time(raw_timestamp=_soc_data[2], use_us=True),
                        self._pid,
                        self._tid,
                        OrderedDict([(self.MATA_BW_LEVEL, _soc_data[0])]),
                    ],
                    [
                        self.L2_BUFFER_BW_LEVEL,
                        InfoConfReader().trans_into_local_time(raw_timestamp=_soc_data[2], use_us=True),
                        self._pid,
                        self._tid,
                        OrderedDict([(self.L2_BUFFER_BW_LEVEL, _soc_data[1])]),
                    ],
                ]
            )
        self._timeline_data.extend(
            TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, soc_res)
        )
        return self._timeline_data
