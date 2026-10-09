# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.v5.v5_stars_bean import V5StarsBean
from msparser.v5.v5_stars_parser import V5StarsParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.v5.v5_stars_parser'

FORMAT_DATA_LIST = [
    [2, 0, 0, 'AI_CORE', 900324512280.0, 900329393820.0, 4881540.0, 1220001, 1, 530412, 0,
     382664, 244160, 415313, 555, 0, 0, 789, 44],
    [2, 0, 1, 'AI_CORE', 900329394020.0, 900332136660.0, 2742640.0, 685278, 1, 500064, 0,
     2552, 0, 682657, 676788, 0, 0, 7534, 4],
    [2, 0, 2, 'AI_CORE', 900332136860.0, 900396914500.0, 64777640.0, 16194028, 1, 4,
     15137216, 42922, 672, 3592381, 202059, 276320, 0, 38142, 13]
]


class TestV5StarsParser(unittest.TestCase):
    file_list = {DataTag.V5_STARS_PROFILE: ['v5_stars_profile.data.0.slice_0']}

    def test_ms_run_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.V5StarsParser.parse'), \
                mock.patch(NAMESPACE + '.V5StarsParser.save'):
            check = V5StarsParser(self.file_list, CONFIG)
            check.ms_run()

    def test_ms_run_when_parse_error_then_failed(self):
        with mock.patch(NAMESPACE + '.V5StarsParser.parse', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = V5StarsParser(self.file_list, CONFIG)
            check.ms_run()

    def test_parse_when_normal_then_pass(self):
        soc_profile_data = (
            0, 2, 256, 2048, 0, 1672395805, 243083, 1215030, 14, 0, 0,
            81920, 74466, 242432, 242432, 80640, 169728, 14433, 2562, 0, 0
        )
        struct_data = struct.pack("BBHHHIIIHBBIIIIIIIIII", *soc_profile_data)
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.V5StarsParser.get_file_path_and_check', return_value=True), \
                mock.patch('common_func.file_manager.check_path_valid', return_value=True), \
                mock.patch(NAMESPACE + '.logging.info', return_value=True):
            with mock.patch('os.path.getsize', return_value=64), \
                    mock.patch('builtins.open', mock.mock_open(read_data=struct_data)):
                check = V5StarsParser(self.file_list, CONFIG)
                check.parse()

    def test_save_when_no_data_then_return(self):
        check = V5StarsParser(self.file_list, CONFIG)
        check.save()

    def test_save_when_have_data_then_save(self):
        with mock.patch(NAMESPACE + ".V5StarsParser._reformat_data", return_value=FORMAT_DATA_LIST), \
                mock.patch('msmodel.v5.v5_stars_model.V5StarsModel.flush'):
            check = V5StarsParser(self.file_list, CONFIG)
            check.save()

    def test_reformat_data_when_get_data_list_then_return_format_data(self):
        bin_data = struct.pack('=BBHHHIIIHBBIIIIIIIIIIBBHHHIIIHBBIIIIIIIIIIBBHHHIIIHBBIIIIIIIIII',
                               0, 2, 0, 2048, 0, 2066552654, 244077, 1220001, 10, 0, 0, 530412, 0, 382664, 244160,
                               415313, 555, 0, 0, 789, 44,
                               0, 2, 0, 2048, 1, 2066796741, 137132, 685278, 10, 0, 0, 500064, 0, 2552, 0, 682657,
                               676788, 0, 0, 7534, 4,
                               0, 2, 0, 2048, 2, 2066933883, 3238882, 16194028, 10, 0, 0, 4, 15137216, 42922, 672,
                               3592381, 202059, 276320, 0, 38142, 13)
        data_list = [
            V5StarsBean.decode(bin_data[:64]),
            V5StarsBean.decode(bin_data[64:128]),
            V5StarsBean.decode(bin_data[128:])
        ]
        InfoConfReader()._info_json = {'pid': 1, "DeviceInfo": [{'hwts_frequency': 50}]}
        with mock.patch('msmodel.v5.v5_exeom_model' + '.V5ExeomViewModel.get_v5_host_data',
                        return_value={(2, 0): [(0, 1)], (2, 1): [(0, 1)], (2, 2): [(0, 1)]}):
            self.assertEqual(V5StarsParser(self.file_list, CONFIG)._reformat_data(data_list), FORMAT_DATA_LIST)

    def tearDown(self) -> None:
        info_reader = InfoConfReader()
        info_reader._info_json = {}
        info_reader._host_freq = None


if __name__ == '__main__':
    unittest.main()
