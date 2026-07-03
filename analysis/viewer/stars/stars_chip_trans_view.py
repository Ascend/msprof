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

import logging
from collections import OrderedDict
from collections import defaultdict

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.stars_constant import StarsConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.platform.chip_manager import ChipManager
from msmodel.interface.view_model import ViewModel


class StarsChipTransView:
    """
    Model of viewer for chip trans
    """

    PA_LINK_RX = "PA Link Rx"
    PA_LINK_TX = "PA Link Tx"
    PCIE_WRITE = "PCIE Write Bandwidth"
    PCIE_READ = "PCIE Read Bandwidth"
    PA_ID = "PA Link ID"
    PCIE_ID = "PCIE ID"

    TIMELINE_MAP = {
        StarsConstant.TYPE_STARS_PA: [PA_LINK_RX, PA_LINK_TX, PA_ID],
        StarsConstant.TYPE_STARS_PCIE: [PCIE_WRITE, PCIE_READ, PCIE_ID],
    }

    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._model = None
        self._timeline_data = []
        self._pid = InfoConfReader().get_json_pid_data()
        self._tid = InfoConfReader().get_json_tid_data()

    def get_timeline_data(self: any) -> list:
        """
        get timeline data of stars chip trans
        :return:
        """
        if ChipManager().is_chip_v6():
            self.get_pcie_data_v6()
        else:
            self.get_pa_data()
            self.get_pcie_data()
        if not self._timeline_data:
            logging.warning("Failed to get stars chip trans data, please check the data parsing log.")
            return []
        self._timeline_data.extend(
            TraceViewManager.metadata_event([["process_name", self._pid, self._tid, "Stars Chip Trans"]])
        )
        return self._timeline_data

    def get_pa_data(self: any) -> None:
        """
        get stars pa link data
        :return:
        """
        pa_model = ViewModel(
            self._project_path, DBNameConstant.DB_STARS_CHIP_TRANS, [DBNameConstant.TABLE_STARS_PA_LINK]
        )
        if not pa_model.init():
            logging.error("Can not get stars chip trans data, please check the data parsing log.")
            return
        if not pa_model.check_table():
            return
        sql = "select pa_link_id,pa_link_traffic_monit_rx,pa_link_traffic_monit_tx,sys_time/{NS_TO_US} from {0}".format(
            DBNameConstant.TABLE_STARS_PA_LINK, NS_TO_US=NumberConstant.NS_TO_US
        )
        pa_data = pa_model.get_sql_data(sql)
        if not pa_data:
            return
        self._timeline_data.extend(
            TraceViewManager.metadata_event([["thread_name", self._pid, StarsConstant.TYPE_STARS_PA, "PA Link"]])
        )
        self._format_data(pa_data, StarsConstant.TYPE_STARS_PA)

    def get_pcie_data(self: any) -> None:
        """
        get stars pcie data
        :return:
        """
        pcie_model = ViewModel(
            self._project_path, DBNameConstant.DB_STARS_CHIP_TRANS, [DBNameConstant.TABLE_STARS_PCIE]
        )
        pcie_data = []
        with pcie_model:
            sql = "select pcie_id,pcie_write_bandwidth,pcie_read_bandwidth,sys_time/{NS_TO_US} from {0}".format(
                DBNameConstant.TABLE_STARS_PCIE, NS_TO_US=NumberConstant.NS_TO_US
            )
            pcie_data = pcie_model.get_sql_data(sql)
        if not pcie_data:
            return
        self._timeline_data.extend(
            TraceViewManager.metadata_event([["thread_name", self._pid, StarsConstant.TYPE_STARS_PCIE, "PCIE"]])
        )
        self._format_data(pcie_data, StarsConstant.TYPE_STARS_PCIE)

    def get_pcie_data_v6(self: any) -> None:
        """
        get stars pcie data v6
        :return:
        """
        pcie_model = ViewModel(
            self._project_path, DBNameConstant.DB_STARS_CHIP_TRANS, [DBNameConstant.TABLE_STARS_PCIE_V6]
        )
        pcie_data = []
        with pcie_model:
            sql = "select die_id,pcie_write_bandwidth,pcie_read_bandwidth,sys_time/{NS_TO_US} from {0}".format(
                DBNameConstant.TABLE_STARS_PCIE_V6, NS_TO_US=NumberConstant.NS_TO_US
            )
            pcie_data = pcie_model.get_sql_data(sql)
        if not pcie_data:
            return
        pcie_data_dict = defaultdict(dict)
        for data in pcie_data:
            pcie_data_dict.setdefault(data[3], {}).update({data[0]: data[1:]})
        self._timeline_data.extend(
            TraceViewManager.metadata_event([["thread_name", self._pid, StarsConstant.TYPE_STARS_PCIE, "PCIE"]])
        )
        stars_cons = self.TIMELINE_MAP.get(StarsConstant.TYPE_STARS_PCIE)
        _res = []
        for key, data in pcie_data_dict.items():
            _res.extend(
                [
                    [
                        stars_cons[0],
                        InfoConfReader().trans_into_local_time(key, use_us=True),
                        self._pid,
                        StarsConstant.TYPE_STARS_PCIE,
                        OrderedDict([("U-Die" + str(_key), _val[0]) for _key, _val in data.items()]),
                    ],
                    [
                        stars_cons[1],
                        InfoConfReader().trans_into_local_time(key, use_us=True),
                        self._pid,
                        StarsConstant.TYPE_STARS_PCIE,
                        OrderedDict([("U-Die" + str(_key), _val[1]) for _key, _val in data.items()]),
                    ],
                ]
            )
        self._timeline_data.extend(
            TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, _res)
        )

    def _format_data(self: any, stars_data: list, stars_type: int) -> None:
        _res = []
        stars_cons = self.TIMELINE_MAP.get(stars_type)
        for _data in stars_data:
            # _data format: pcie_id,pcie_write_bandwidth,pcie_read_bandwidth,sys_time or
            #               pa_link_id,pa_link_traffic_monit_rx,pa_link_traffic_monit_tx,sys_time
            # stars_cons: pa rx/ pcie read name, pa tx/ pcie write name, id
            _res.extend(
                [
                    [
                        stars_cons[0],
                        InfoConfReader().trans_into_local_time(_data[3], use_us=True),
                        self._pid,
                        stars_type,
                        OrderedDict([(stars_cons[0], _data[1]), (stars_cons[2], _data[0])]),
                    ],
                    [
                        stars_cons[1],
                        InfoConfReader().trans_into_local_time(_data[3], use_us=True),
                        self._pid,
                        stars_type,
                        OrderedDict([(stars_cons[1], _data[2]), (stars_cons[2], _data[0])]),
                    ],
                ]
            )
        self._timeline_data.extend(
            TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, _res)
        )
