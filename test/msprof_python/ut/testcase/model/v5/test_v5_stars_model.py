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
import unittest

from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.v5.v5_stars_model import V5StarsModel, V5StarsViewModel

NAMESPACE = 'msmodel.v5.v5_stars_model'

FORMAT_DATA_LIST = [
    [2, 0, 0, 'AI_CORE', 900324512280.0, 900329393820.0, 4881540.0, 1220001, 1, 530412, 0,
     382664, 244160, 414903, 333062, 0, 0, 222, 47],
    [2, 0, 1, 'AI_CORE', 900329394020.0, 900332136660.0, 2742640.0, 685278, 1, 500064, 0,
     2552, 0, 682657, 676788, 0, 0, 7534, 4],
    [2, 0, 2, 'AI_CORE', 900332136860.0, 900396914500.0, 64777640.0, 16194028, 1, 4,
     15137216, 42922, 672, 3592381, 202059, 276320, 0, 38142, 13]
]


class TestV5StarsModel(TestDirCRBaseModel):
    def test_flush_when_get_data_list_then_return_none(self):
        self.assertEqual(V5StarsModel('test').flush(FORMAT_DATA_LIST), None)

    def test_get_v5_data_within_time_range_when_different_range_then_get_different_result(self):

        with V5StarsModel(self.PROF_DEVICE_DIR) as model:
            model.flush(FORMAT_DATA_LIST)
            with V5StarsViewModel(self.PROF_DEVICE_DIR) as view_model:
                device_tasks = view_model.get_v5_data_within_time_range(float("-inf"), float("inf"))
                self.assertEqual(len(device_tasks), 3)

                device_tasks = view_model.get_v5_data_within_time_range(0, 500)
                self.assertEqual(len(device_tasks), 0)

                device_tasks = view_model.get_v5_data_within_time_range(900324512270, 900329393830)
                self.assertEqual(len(device_tasks), 1)

    def test_get_v5_pmu_details_should_return_data_when_is_normal(self):
        with V5StarsModel(self.PROF_DEVICE_DIR) as model:
            model.flush(FORMAT_DATA_LIST)
            with V5StarsViewModel(self.PROF_DEVICE_DIR) as view_model:
                res_data = view_model.get_v5_pmu_details()
                self.assertEqual(len(res_data), 3)


if __name__ == '__main__':
    unittest.main()
