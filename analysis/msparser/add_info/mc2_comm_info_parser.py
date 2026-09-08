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

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.platform.chip_manager import ChipManager
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from msparser.add_info.mc2_comm_info_bean import Mc2CommInfoBean
from profiling_bean.prof_enum.data_tag import DataTag
from msmodel.add_info.mc2_comm_info_model import Mc2CommInfoModel


class Mc2CommInfoParser(DataParser, MsMultiProcess):
    INVALID_STREAM_ID = 65535  # UINT16_MAX

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list
        self.project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._communication_info = []

    def reformat_data(self) -> list:
        reformat_data = []
        # 适配15 16 level0 device无streamId场景
        map_invalid_stream = ChipManager().is_chip_v6() and InfoConfReader().is_level0()
        appended_stream_ids = set()
        for data in self._communication_info:
            row = [
                data.group_name,
                data.rank_size,
                data.rank_id,
                data.usr_rank_id,
                data.stream_id,
                data.comm_stream_ids,
            ]
            reformat_data.append(row)
            # aicpuKfcStreamId 不变，单独补一条 comm stream=65535 的映射；同一个流只补一条
            if map_invalid_stream and data.stream_id not in appended_stream_ids:
                appended_stream_ids.add(data.stream_id)
                reformat_data.append(
                    [
                        data.group_name,
                        data.rank_size,
                        data.rank_id,
                        data.usr_rank_id,
                        data.stream_id,
                        str(self.INVALID_STREAM_ID),
                    ]
                )
        return reformat_data

    def ms_run(self: any) -> None:
        """
        parse and save ge fusion data
        :return:
        """
        if not self._file_list.get(DataTag.MC2_COMM_INFO, []):
            return
        logging.info("start parsing mc2 comm info, files: %s", str(self._file_list.get(DataTag.MC2_COMM_INFO)))
        self.parse()
        self.save()

    def parse(self: any) -> None:
        """
        parse mc2 comm info data
        """
        mc2_comm_info_files = self._file_list.get(DataTag.MC2_COMM_INFO, [])
        mc2_comm_info_files = self.group_aging_file(mc2_comm_info_files)
        for file_list in mc2_comm_info_files.values():
            self._communication_info += self.parse_bean_data(
                file_list,
                StructFmt.MC2_COMM_INFO_SIZE,
                Mc2CommInfoBean,
                check_func=self.check_magic_num,
            )

    def save(self: any) -> None:
        """
        save mc2 comm info parser data to db
        :return: None
        """
        if not self._communication_info:
            return
        with Mc2CommInfoModel(self._project_path) as _model:
            _model.flush(self.reformat_data())
