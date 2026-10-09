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
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.stars_constant import StarsConstant
from msmodel.stars.stars_chip_trans_model import StarsChipTransModel

NAMESPACE = 'msmodel.stars.stars_chip_trans_model'


class TestStarsChipTransModel(unittest.TestCase):

    def test_flush(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        data_dict = {StarsConstant.TYPE_STARS_PA: [], "01": []}
        with mock.patch(NAMESPACE + '.StarsChipTransModel.insert_data_to_db'):
            check = StarsChipTransModel('test', 'test', [])
            check.flush(data_dict)
